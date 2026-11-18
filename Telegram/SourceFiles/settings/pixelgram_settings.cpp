#include "settings/pixelgram_settings.h"

#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "storage/localstorage.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace PixelGram {
namespace {

Config globalConfig;
bool loaded = false;

QString ConfigFilePath() {
	return cWorkingDir() + u"tdata/pixelgram_settings.json"_q;
}

} // namespace

Config &CurrentConfig() {
	if (!loaded) {
		LoadConfig();
	}
	return globalConfig;
}

void LoadConfig() {
	loaded = true;
	QFile file(ConfigFilePath());
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto doc = QJsonDocument::fromJson(file.readAll());
	if (!doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	if (obj.contains("antiDelete")) {
		globalConfig.antiDelete = obj["antiDelete"].toBool(true);
	}
	if (obj.contains("ghostMode")) {
		globalConfig.ghostMode = obj["ghostMode"].toBool(true);
	}
	if (obj.contains("hideTyping")) {
		globalConfig.hideTyping = obj["hideTyping"].toBool(true);
	}
	if (obj.contains("bypassProtection")) {
		globalConfig.bypassProtection = obj["bypassProtection"].toBool(true);
	}
	if (obj.contains("blurMediaBackground")) {
		globalConfig.blurMediaBackground = obj["blurMediaBackground"].toBool(true);
	}
}

void SaveConfig() {
	QJsonObject obj;
	obj["antiDelete"] = globalConfig.antiDelete;
	obj["ghostMode"] = globalConfig.ghostMode;
	obj["hideTyping"] = globalConfig.hideTyping;
	obj["bypassProtection"] = globalConfig.bypassProtection;
	obj["blurMediaBackground"] = globalConfig.blurMediaBackground;
	obj["m3SeedColor"] = QString("0x%1").arg(globalConfig.m3SeedColor, 8, 16, QChar('0'));
	obj["m3SchemeVariant"] = globalConfig.m3SchemeVariant;
	obj["m3ContrastLevel"] = globalConfig.m3ContrastLevel;

	QFile file(ConfigFilePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
	}
}

void ShowSettingsBox(Window::SessionController *controller) {
	if (!controller) {
		return;
	}
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(u"PixelGram Settings"_q));

		const auto addToggle = [&](
				const QString &title,
				const QString &desc,
				bool initial,
				Fn<void(bool)> onChange) {
			const auto checkbox = box->addRow(object_ptr<Ui::Checkbox>(
				box,
				title,
				initial,
				st::defaultBoxCheckbox));
			checkbox->checkedChanges(
			) | rpl::on_next([=](bool checked) {
				onChange(checked);
				SaveConfig();
			}, box->lifetime());

			if (!desc.isEmpty()) {
				box->addRow(object_ptr<Ui::FlatLabel>(
					box,
					rpl::single(desc),
					st::boxDividerLabel),
					QMargins(st::boxRowPadding.left() + 24, 2, st::boxRowPadding.right(), 12));
			}
		};

		addToggle(
			u"Размытие фона (Blur background)"_q,
			u"Размытие содержимого приложения при открытии фото и видео."_q,
			CurrentConfig().blurMediaBackground,
			[](bool checked) { CurrentConfig().blurMediaBackground = checked; });

		addToggle(
			u"Анти-удаление (Anti-Delete)"_q,
			u"Сохранять удаленные сообщения собеседником в чате."_q,
			CurrentConfig().antiDelete,
			[](bool checked) { CurrentConfig().antiDelete = checked; });

		addToggle(
			u"Режим невидимки (Ghost Mode)"_q,
			u"Не отправлять статус прочтения сообщений."_q,
			CurrentConfig().ghostMode,
			[](bool checked) { CurrentConfig().ghostMode = checked; });

		addToggle(
			u"Скрыть набор текста (Hide Typing)"_q,
			u"Не отправлять статус 'печатает...' в диалогах."_q,
			CurrentConfig().hideTyping,
			[](bool checked) { CurrentConfig().hideTyping = checked; });

		addToggle(
			u"Обход защиты (Bypass Protection)"_q,
			u"Разрешить копирование и скриншоты в защищенных чатах."_q,
			CurrentConfig().bypassProtection,
			[](bool checked) { CurrentConfig().bypassProtection = checked; });

		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

} // namespace PixelGram
