#include "apiwrap.h"
#include "settings/pixelgram_settings.h"
#include "ui/pixelgram_avatar_shapes.h"

#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/discrete_sliders.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "window/window_session_controller.h"
#include "main/main_session.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "styles/style_chat.h"
#include "styles/style_settings.h"
#include "storage/localstorage.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>

namespace PixelGram {
namespace {

Config globalConfig;
bool loaded = false;

QString ConfigFilePath() {
	return cWorkingDir() + u"tdata/pixelgram_settings.json"_q;
}

class AvatarPreviewWidget : public Ui::RpWidget {
public:
	AvatarPreviewWidget(QWidget *parent) : RpWidget(parent) {
		resize(st::boxWideWidth, 96);
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setRenderHint(QPainter::Antialiasing);

		const auto size = 72;
		const auto rect = QRect((width() - size) / 2, (height() - size) / 2, size, size);
		const auto &cfg = CurrentConfig();
		const auto path = AvatarShapePath(QRectF(rect), cfg.avatarShape, cfg.avatarCornerRadius);

		p.save();
		p.setClipPath(path);
		QLinearGradient grad(rect.topLeft(), rect.bottomRight());
		grad.setColorAt(0.0, QColor(0x33, 0x90, 0xec));
		grad.setColorAt(1.0, QColor(0x7a, 0xa5, 0xeb));
		p.fillRect(rect, grad);

		p.setFont(st::msgServiceFont->f);
		p.setPen(QColor(0xff, 0xff, 0xff));
		p.drawText(rect, Qt::AlignCenter, "PG");
		p.restore();
	}
};

class BubblePreviewWidget : public Ui::RpWidget {
public:
	BubblePreviewWidget(QWidget *parent) : RpWidget(parent) {
		resize(st::boxWideWidth, 112);
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setRenderHint(QPainter::Antialiasing);

		const auto &cfg = CurrentConfig();
		const auto radius = (cfg.messageBubbleRadius > 0)
			? cfg.messageBubbleRadius
			: 16;
		const auto tailRadius = cfg.removeMessageTail ? radius : 2;

		// Incoming bubble (left)
		{
			QRect inRect(16, 10, 190, 40);
			QPainterPath inPath;
			inPath.moveTo(inRect.left() + radius, inRect.top());
			inPath.lineTo(inRect.right() - radius, inRect.top());
			inPath.quadTo(inRect.right(), inRect.top(), inRect.right(), inRect.top() + radius);
			inPath.lineTo(inRect.right(), inRect.bottom() - radius);
			inPath.quadTo(inRect.right(), inRect.bottom(), inRect.right() - radius, inRect.bottom());
			inPath.lineTo(inRect.left() + tailRadius, inRect.bottom());
			inPath.quadTo(inRect.left(), inRect.bottom(), inRect.left(), inRect.bottom() - tailRadius);
			inPath.lineTo(inRect.left(), inRect.top() + radius);
			inPath.quadTo(inRect.left(), inRect.top(), inRect.left() + radius, inRect.top());

			p.fillPath(inPath, QColor(0x28, 0x2e, 0x33));
			p.setPen(QColor(0xff, 0xff, 0xff));
			p.drawText(inRect.adjusted(12, 10, -12, -10), Qt::AlignLeft | Qt::AlignVCenter, "PixelGram Preview");
		}

		// Outgoing bubble (right)
		{
			QRect outRect(width() - 206, 58, 190, 40);
			QPainterPath outPath;
			outPath.moveTo(outRect.left() + radius, outRect.top());
			outPath.lineTo(outRect.right() - radius, outRect.top());
			outPath.quadTo(outRect.right(), outRect.top(), outRect.right(), outRect.top() + radius);
			outPath.lineTo(outRect.right(), outRect.bottom() - tailRadius);
			outPath.quadTo(outRect.right(), outRect.bottom(), outRect.right() - tailRadius, outRect.bottom());
			outPath.lineTo(outRect.left() + radius, outRect.bottom());
			outPath.quadTo(outRect.left(), outRect.bottom(), outRect.left() + radius, outRect.bottom());
			outPath.lineTo(outRect.left(), outRect.top() + radius);
			outPath.quadTo(outRect.left(), outRect.top(), outRect.left() + radius, outRect.top());

			const auto accent = QColor::fromRgb(cfg.m3SeedColor ? cfg.m3SeedColor : 0x7aa5eb);
			p.fillPath(outPath, accent);
			p.setPen(QColor(0x00, 0x00, 0x00));
			p.drawText(outRect.adjusted(12, 10, -12, -10), Qt::AlignLeft | Qt::AlignVCenter, "Material 3 Bubbles!");
		}
	}
};

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

	// Base
	if (obj.contains("antiDelete")) globalConfig.antiDelete = obj["antiDelete"].toBool(true);
	if (obj.contains("bypassProtection")) globalConfig.bypassProtection = obj["bypassProtection"].toBool(true);
	if (obj.contains("blurMediaBackground")) globalConfig.blurMediaBackground = obj["blurMediaBackground"].toBool(true);

	// Ghost
	if (obj.contains("ghostMode")) globalConfig.ghostMode = obj["ghostMode"].toBool(false);
	if (obj.contains("dontReadMessages")) globalConfig.dontReadMessages = obj["dontReadMessages"].toBool(false);
	if (obj.contains("dontReadStories")) globalConfig.dontReadStories = obj["dontReadStories"].toBool(false);
	if (obj.contains("dontSendOnline")) globalConfig.dontSendOnline = obj["dontSendOnline"].toBool(false);
	if (obj.contains("dontSendTyping")) globalConfig.dontSendTyping = obj["dontSendTyping"].toBool(false);
	if (obj.contains("hideTyping")) globalConfig.hideTyping = obj["hideTyping"].toBool(false);
	if (obj.contains("goOfflineAutomatically")) globalConfig.goOfflineAutomatically = obj["goOfflineAutomatically"].toBool(false);
	if (obj.contains("storyGhostAlert")) globalConfig.storyGhostAlert = obj["storyGhostAlert"].toBool(true);

	// Other
	if (obj.contains("localPremium")) globalConfig.localPremium = obj["localPremium"].toBool(false);
	if (obj.contains("disableAds")) globalConfig.disableAds = obj["disableAds"].toBool(true);

	// Appearance
	if (obj.contains("avatarShape")) globalConfig.avatarShape = static_cast<AvatarShape>(obj["avatarShape"].toInt(0));
	if (obj.contains("avatarCornerRadius")) globalConfig.avatarCornerRadius = obj["avatarCornerRadius"].toInt(50);
	if (obj.contains("singleCornerRadius")) globalConfig.singleCornerRadius = obj["singleCornerRadius"].toBool(false);
	if (obj.contains("disableCustomBackgrounds")) globalConfig.disableCustomBackgrounds = obj["disableCustomBackgrounds"].toBool(false);
	if (obj.contains("hidePremiumStatuses")) globalConfig.hidePremiumStatuses = obj["hidePremiumStatuses"].toBool(false);
	if (obj.contains("monospaceFont")) globalConfig.monospaceFont = obj["monospaceFont"].toString();
	if (obj.contains("hideFolderCounters")) globalConfig.hideFolderCounters = obj["hideFolderCounters"].toBool(false);
	if (obj.contains("hideAllChatsTab")) globalConfig.hideAllChatsTab = obj["hideAllChatsTab"].toBool(false);
	if (obj.contains("trayGhostMode")) globalConfig.trayGhostMode = obj["trayGhostMode"].toBool(false);
	if (obj.contains("trayStreamerMode")) globalConfig.trayStreamerMode = obj["trayStreamerMode"].toBool(false);
	if (obj.contains("drawerMyProfile")) globalConfig.drawerMyProfile = obj["drawerMyProfile"].toBool(true);
	if (obj.contains("drawerNewGroup")) globalConfig.drawerNewGroup = obj["drawerNewGroup"].toBool(true);
	if (obj.contains("drawerNewChannel")) globalConfig.drawerNewChannel = obj["drawerNewChannel"].toBool(true);
	if (obj.contains("drawerContacts")) globalConfig.drawerContacts = obj["drawerContacts"].toBool(true);
	if (obj.contains("drawerCalls")) globalConfig.drawerCalls = obj["drawerCalls"].toBool(true);
	if (obj.contains("drawerSavedMessages")) globalConfig.drawerSavedMessages = obj["drawerSavedMessages"].toBool(true);
	if (obj.contains("drawerNightMode")) globalConfig.drawerNightMode = obj["drawerNightMode"].toBool(true);
	if (obj.contains("drawerGhostMode")) globalConfig.drawerGhostMode = obj["drawerGhostMode"].toBool(false);
	if (obj.contains("drawerStreamerMode")) globalConfig.drawerStreamerMode = obj["drawerStreamerMode"].toBool(false);

	// Chats
	if (obj.contains("showOnlyAddedStickers")) globalConfig.showOnlyAddedStickers = obj["showOnlyAddedStickers"].toBool(false);
	if (obj.contains("unlimitedRecentStickers")) globalConfig.unlimitedRecentStickers = obj["unlimitedRecentStickers"].toBool(false);
	if (obj.contains("hideReactionsBar")) globalConfig.hideReactionsBar = obj["hideReactionsBar"].toBool(false);
	if (obj.contains("hideReactionsUnderMessage")) globalConfig.hideReactionsUnderMessage = obj["hideReactionsUnderMessage"].toBool(false);
	if (obj.contains("hideReactionsAnimations")) globalConfig.hideReactionsAnimations = obj["hideReactionsAnimations"].toBool(false);
	if (obj.contains("channelBottomAction")) globalConfig.channelBottomAction = static_cast<ChannelBottomAction>(obj["channelBottomAction"].toInt(0));
	if (obj.contains("quickAdminShortcuts")) globalConfig.quickAdminShortcuts = obj["quickAdminShortcuts"].toBool(false);
	if (obj.contains("disableGreetingSticker")) globalConfig.disableGreetingSticker = obj["disableGreetingSticker"].toBool(false);
	if (obj.contains("menuMessageShot")) globalConfig.menuMessageShot = static_cast<ContextMenuDisplay>(obj["menuMessageShot"].toInt(0));
	if (obj.contains("replaceWithIcons")) globalConfig.replaceWithIcons = obj["replaceWithIcons"].toBool(false);
	if (obj.contains("removeMessageTail")) globalConfig.removeMessageTail = obj["removeMessageTail"].toBool(false);
	if (obj.contains("hideSideShareButton")) globalConfig.hideSideShareButton = obj["hideSideShareButton"].toBool(false);
	if (obj.contains("disableColorfulReplies")) globalConfig.disableColorfulReplies = obj["disableColorfulReplies"].toBool(false);
	if (obj.contains("translucentDeletedMessages")) globalConfig.translucentDeletedMessages = obj["translucentDeletedMessages"].toBool(true);
	if (obj.contains("messageBubbleRadius")) globalConfig.messageBubbleRadius = obj["messageBubbleRadius"].toInt(16);
	if (obj.contains("wideMessagesMultiplier")) globalConfig.wideMessagesMultiplier = obj["wideMessagesMultiplier"].toDouble(1.0);
	if (obj.contains("menuReactions")) globalConfig.menuReactions = static_cast<ContextMenuDisplay>(obj["menuReactions"].toInt(0));
	if (obj.contains("menuViews")) globalConfig.menuViews = static_cast<ContextMenuDisplay>(obj["menuViews"].toInt(0));
	if (obj.contains("menuHideLocally")) globalConfig.menuHideLocally = static_cast<ContextMenuDisplay>(obj["menuHideLocally"].toInt(0));
	if (obj.contains("menuUserMessages")) globalConfig.menuUserMessages = static_cast<ContextMenuDisplay>(obj["menuUserMessages"].toInt(0));
	if (obj.contains("menuDetails")) globalConfig.menuDetails = static_cast<ContextMenuDisplay>(obj["menuDetails"].toInt(0));
	if (obj.contains("menuRepeat")) globalConfig.menuRepeat = static_cast<ContextMenuDisplay>(obj["menuRepeat"].toInt(0));
	if (obj.contains("showAttachButton")) globalConfig.showAttachButton = obj["showAttachButton"].toBool(true);
	if (obj.contains("showCommandsButton")) globalConfig.showCommandsButton = obj["showCommandsButton"].toBool(true);
	if (obj.contains("showTtlButton")) globalConfig.showTtlButton = obj["showTtlButton"].toBool(true);
	if (obj.contains("showEmojiButton")) globalConfig.showEmojiButton = obj["showEmojiButton"].toBool(true);
	if (obj.contains("showVoiceButton")) globalConfig.showVoiceButton = obj["showVoiceButton"].toBool(true);
	if (obj.contains("showGiftButton")) globalConfig.showGiftButton = obj["showGiftButton"].toBool(true);
	if (obj.contains("openAttachOnHover")) globalConfig.openAttachOnHover = obj["openAttachOnHover"].toBool(false);
	if (obj.contains("openEmojiOnHover")) globalConfig.openEmojiOnHover = obj["openEmojiOnHover"].toBool(false);

	// M3
	if (obj.contains("m3SeedColor")) globalConfig.m3SeedColor = obj["m3SeedColor"].toString("0x00000000").toUInt(nullptr, 16);
	if (obj.contains("m3SchemeVariant")) globalConfig.m3SchemeVariant = obj["m3SchemeVariant"].toInt(0);
	if (obj.contains("m3ContrastLevel")) globalConfig.m3ContrastLevel = obj["m3ContrastLevel"].toDouble(0.0);
}

void SaveConfig() {
	QJsonObject obj;
	// Base
	obj["antiDelete"] = globalConfig.antiDelete;
	obj["bypassProtection"] = globalConfig.bypassProtection;
	obj["blurMediaBackground"] = globalConfig.blurMediaBackground;

	// Ghost
	obj["ghostMode"] = globalConfig.ghostMode;
	obj["dontReadMessages"] = globalConfig.dontReadMessages;
	obj["dontReadStories"] = globalConfig.dontReadStories;
	obj["dontSendOnline"] = globalConfig.dontSendOnline;
	obj["dontSendTyping"] = globalConfig.dontSendTyping;
	obj["hideTyping"] = globalConfig.hideTyping;
	obj["goOfflineAutomatically"] = globalConfig.goOfflineAutomatically;
	obj["storyGhostAlert"] = globalConfig.storyGhostAlert;

	// Other
	obj["localPremium"] = globalConfig.localPremium;
	obj["disableAds"] = globalConfig.disableAds;

	// Appearance
	obj["avatarShape"] = static_cast<int>(globalConfig.avatarShape);
	obj["avatarCornerRadius"] = globalConfig.avatarCornerRadius;
	obj["singleCornerRadius"] = globalConfig.singleCornerRadius;
	obj["disableCustomBackgrounds"] = globalConfig.disableCustomBackgrounds;
	obj["hidePremiumStatuses"] = globalConfig.hidePremiumStatuses;
	obj["monospaceFont"] = globalConfig.monospaceFont;
	obj["hideFolderCounters"] = globalConfig.hideFolderCounters;
	obj["hideAllChatsTab"] = globalConfig.hideAllChatsTab;
	obj["trayGhostMode"] = globalConfig.trayGhostMode;
	obj["trayStreamerMode"] = globalConfig.trayStreamerMode;
	obj["drawerMyProfile"] = globalConfig.drawerMyProfile;
	obj["drawerNewGroup"] = globalConfig.drawerNewGroup;
	obj["drawerNewChannel"] = globalConfig.drawerNewChannel;
	obj["drawerContacts"] = globalConfig.drawerContacts;
	obj["drawerCalls"] = globalConfig.drawerCalls;
	obj["drawerSavedMessages"] = globalConfig.drawerSavedMessages;
	obj["drawerNightMode"] = globalConfig.drawerNightMode;
	obj["drawerGhostMode"] = globalConfig.drawerGhostMode;
	obj["drawerStreamerMode"] = globalConfig.drawerStreamerMode;

	// Chats
	obj["showOnlyAddedStickers"] = globalConfig.showOnlyAddedStickers;
	obj["unlimitedRecentStickers"] = globalConfig.unlimitedRecentStickers;
	obj["hideReactionsBar"] = globalConfig.hideReactionsBar;
	obj["hideReactionsUnderMessage"] = globalConfig.hideReactionsUnderMessage;
	obj["hideReactionsAnimations"] = globalConfig.hideReactionsAnimations;
	obj["channelBottomAction"] = static_cast<int>(globalConfig.channelBottomAction);
	obj["quickAdminShortcuts"] = globalConfig.quickAdminShortcuts;
	obj["disableGreetingSticker"] = globalConfig.disableGreetingSticker;
	obj["menuMessageShot"] = static_cast<int>(globalConfig.menuMessageShot);
	obj["replaceWithIcons"] = globalConfig.replaceWithIcons;
	obj["removeMessageTail"] = globalConfig.removeMessageTail;
	obj["hideSideShareButton"] = globalConfig.hideSideShareButton;
	obj["disableColorfulReplies"] = globalConfig.disableColorfulReplies;
	obj["translucentDeletedMessages"] = globalConfig.translucentDeletedMessages;
	obj["messageBubbleRadius"] = globalConfig.messageBubbleRadius;
	obj["wideMessagesMultiplier"] = globalConfig.wideMessagesMultiplier;
	obj["menuReactions"] = static_cast<int>(globalConfig.menuReactions);
	obj["menuViews"] = static_cast<int>(globalConfig.menuViews);
	obj["menuHideLocally"] = static_cast<int>(globalConfig.menuHideLocally);
	obj["menuUserMessages"] = static_cast<int>(globalConfig.menuUserMessages);
	obj["menuDetails"] = static_cast<int>(globalConfig.menuDetails);
	obj["menuRepeat"] = static_cast<int>(globalConfig.menuRepeat);
	obj["showAttachButton"] = globalConfig.showAttachButton;
	obj["showCommandsButton"] = globalConfig.showCommandsButton;
	obj["showTtlButton"] = globalConfig.showTtlButton;
	obj["showEmojiButton"] = globalConfig.showEmojiButton;
	obj["showVoiceButton"] = globalConfig.showVoiceButton;
	obj["showGiftButton"] = globalConfig.showGiftButton;
	obj["openAttachOnHover"] = globalConfig.openAttachOnHover;
	obj["openEmojiOnHover"] = globalConfig.openEmojiOnHover;

	// M3
	obj["m3SeedColor"] = QString("0x%1").arg(globalConfig.m3SeedColor, 8, 16, QChar('0'));
	obj["m3SchemeVariant"] = globalConfig.m3SchemeVariant;
	obj["m3ContrastLevel"] = globalConfig.m3ContrastLevel;

	QFile file(ConfigFilePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
	}
}

void CheckGoOffline(not_null<Main::Session*> session) {
	if (CurrentConfig().goOfflineAutomatically) {
		session->api().request(MTPaccount_UpdateStatus(
			MTP_bool(true)
		)).send();
	}
}

bool MaybeShowStoryGhostAlert(
		not_null<Window::SessionController*> controller,
		Fn<void()> callback) {
	if (!CurrentConfig().storyGhostAlert || CurrentConfig().dontReadStories) {
		return false;
	}
	controller->show(Ui::MakeConfirmBox({
		.text = rpl::single(QString::fromUtf8("Внимание: режим невидимки для историй выключен!\n\nАвтор истории увидит ваш просмотр. Включить невидимку перед открытием?")),
		.confirmed = [=](Fn<void()> close) {
			CurrentConfig().dontReadStories = true;
			SaveConfig();
			close();
			callback();
		},
		.cancelled = [=](Fn<void()> close) {
			close();
			callback();
		},
		.confirmText = rpl::single(QString::fromUtf8("Включить невидимку")),
		.cancelText = rpl::single(QString::fromUtf8("Открыть как обычно")),
	}));
	return true;
}

void ShowSettingsBox(Window::SessionController *controller) {
	if (!controller) {
		return;
	}
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(QString::fromUtf8("Настройки PixelGram")));

		// Category slider (tabs)
		const auto tabsSlider = box->addRow(object_ptr<Ui::SettingsSlider>(box, st::settingsSlider));
		tabsSlider->addSection(QString::fromUtf8("Невидимка"));
		tabsSlider->addSection(QString::fromUtf8("Внешний вид"));
		tabsSlider->addSection(QString::fromUtf8("Чаты"));
		tabsSlider->addSection(QString::fromUtf8("Другое"));

		// Containers for 4 tabs
		const auto tabGhost = box->addRow(object_ptr<Ui::VerticalLayout>(box));
		const auto tabAppearance = box->addRow(object_ptr<Ui::VerticalLayout>(box));
		const auto tabChats = box->addRow(object_ptr<Ui::VerticalLayout>(box));
		const auto tabOther = box->addRow(object_ptr<Ui::VerticalLayout>(box));

		tabGhost->setVisible(true);
		tabAppearance->setVisible(false);
		tabChats->setVisible(false);
		tabOther->setVisible(false);

		tabsSlider->sectionActivated() | rpl::on_next([=](int section) {
			tabGhost->setVisible(section == 0);
			tabAppearance->setVisible(section == 1);
			tabChats->setVisible(section == 2);
			tabOther->setVisible(section == 3);
		}, box->lifetime());

		const auto addToggle = [&](
				not_null<Ui::VerticalLayout*> parent,
				const QString &title,
				const QString &desc,
				bool initial,
				Fn<void(bool)> onChange) {
			const auto checkbox = parent->add(object_ptr<Ui::Checkbox>(
				parent,
				title,
				initial,
				st::defaultBoxCheckbox),
				st::boxRowPadding);
			checkbox->checkedChanges() | rpl::on_next([=](bool checked) {
				onChange(checked);
				SaveConfig();
			}, box->lifetime());

			if (!desc.isEmpty()) {
				parent->add(object_ptr<Ui::FlatLabel>(
					parent,
					rpl::single(desc),
					st::boxDividerLabel),
					QMargins(st::boxRowPadding.left() + 24, 2, st::boxRowPadding.right(), 10));
			}
			return checkbox;
		};

		// ------------------ TAB 1: GHOST MODE ------------------
		const auto masterGhost = tabGhost->add(object_ptr<Ui::Checkbox>(
			tabGhost,
			QString::fromUtf8("Режим невидимки (%1/5)").arg(CurrentConfig().activeGhostCount()),
			CurrentConfig().ghostMode,
			st::defaultBoxCheckbox),
			st::boxRowPadding);

		const auto addGhostSub = [&](const QString &title, bool initial, Fn<void(bool)> onChange) {
			const auto cb = tabGhost->add(object_ptr<Ui::Checkbox>(
				tabGhost,
				title,
				initial,
				st::defaultBoxCheckbox),
				QMargins(st::boxRowPadding.left() + 20, 4, st::boxRowPadding.right(), 4));
			cb->checkedChanges() | rpl::on_next([=](bool checked) {
				onChange(checked);
				SaveConfig();
				masterGhost->setChecked(CurrentConfig().activeGhostCount() > 0);
				masterGhost->setText(QString::fromUtf8("Режим невидимки (%1/5)").arg(CurrentConfig().activeGhostCount()));
			}, box->lifetime());
			return cb;
		};

		const auto cbReadMsgs = addGhostSub(QString::fromUtf8("Не читать сообщения"), CurrentConfig().dontReadMessages, [](bool c) { CurrentConfig().dontReadMessages = c; });
		const auto cbReadStories = addGhostSub(QString::fromUtf8("Не читать истории"), CurrentConfig().dontReadStories, [](bool c) { CurrentConfig().dontReadStories = c; });
		const auto cbOnline = addGhostSub(QString::fromUtf8("Скрыть статус 'В сети'"), CurrentConfig().dontSendOnline, [](bool c) { CurrentConfig().dontSendOnline = c; });
		const auto cbTyping = addGhostSub(QString::fromUtf8("Скрыть статус набора текста"), CurrentConfig().dontSendTyping, [](bool c) { CurrentConfig().dontSendTyping = c; CurrentConfig().hideTyping = c; });
		const auto cbAutoOffline = addGhostSub(QString::fromUtf8("Скрывать 'В сети' после отправки"), CurrentConfig().goOfflineAutomatically, [](bool c) { CurrentConfig().goOfflineAutomatically = c; });

		masterGhost->checkedChanges() | rpl::on_next([=](bool checked) {
			CurrentConfig().setMasterGhostMode(checked);
			SaveConfig();
			cbReadMsgs->setChecked(checked);
			cbReadStories->setChecked(checked);
			cbOnline->setChecked(checked);
			cbTyping->setChecked(checked);
			cbAutoOffline->setChecked(checked);
			masterGhost->setText(QString::fromUtf8("Режим невидимки (%1/5)").arg(CurrentConfig().activeGhostCount()));
		}, box->lifetime());

		addToggle(
			tabGhost,
			QString::fromUtf8("Предупреждение перед просмотром историй"),
			QString::fromUtf8("Предупреждать перед открытием историй, если режим невидимки выключен."),
			CurrentConfig().storyGhostAlert,
			[](bool c) { CurrentConfig().storyGhostAlert = c; });

		// ------------------ TAB 2: APPEARANCE ------------------
		const auto avatarPreview = tabAppearance->add(object_ptr<AvatarPreviewWidget>(tabAppearance));

		tabAppearance->add(object_ptr<Ui::FlatLabel>(
			tabAppearance,
			rpl::single(QString::fromUtf8("Форма аватарок")),
			st::boxTitle),
			st::boxRowPadding);

		const auto shapeGroup = std::make_shared<Ui::RadiobuttonGroup>(static_cast<int>(CurrentConfig().avatarShape));

		const auto addShapeOption = [&](AvatarShape shape, const QString &label) {
			tabAppearance->add(object_ptr<Ui::Radiobutton>(
				tabAppearance,
				shapeGroup,
				static_cast<int>(shape),
				label,
				st::defaultBoxCheckbox),
				QMargins(st::boxRowPadding.left() + 8, 2, st::boxRowPadding.right(), 2));
		};

		addShapeOption(AvatarShape::Custom, QString::fromUtf8("Обычная / Закругленный квадрат"));
		addShapeOption(AvatarShape::Circle, QString::fromUtf8("Круг"));
		addShapeOption(AvatarShape::Squircle, QString::fromUtf8("Сквиркл"));
		addShapeOption(AvatarShape::Cookie, QString::fromUtf8("Печенье"));
		addShapeOption(AvatarShape::Sunflower, QString::fromUtf8("Подсолнух"));
		addShapeOption(AvatarShape::Clover, QString::fromUtf8("Клевер"));
		addShapeOption(AvatarShape::Hexagon, QString::fromUtf8("Шестиугольник"));

		tabAppearance->add(object_ptr<Ui::FlatLabel>(
			tabAppearance,
			rpl::single(QString::fromUtf8("Радиус скругления (для обычной формы)")),
			st::boxDividerLabel),
			st::boxRowPadding);

		const auto radiusSlider = tabAppearance->add(
			object_ptr<Ui::SettingsSlider>(tabAppearance, st::settingsSlider),
			st::boxRowPadding);
		radiusSlider->addSection("10%");
		radiusSlider->addSection("20%");
		radiusSlider->addSection("30%");
		radiusSlider->addSection("40%");
		radiusSlider->addSection("50%");
		const auto initialRadiusIndex = std::clamp((CurrentConfig().avatarCornerRadius - 10) / 10, 0, 4);
		radiusSlider->setActiveSectionFast(initialRadiusIndex);

		const auto singleCornerCheckbox = tabAppearance->add(object_ptr<Ui::Checkbox>(
			tabAppearance,
			QString::fromUtf8("Скругление только одного угла"),
			CurrentConfig().singleCornerRadius,
			st::defaultBoxCheckbox),
			st::boxRowPadding);

		const auto updateShapeControlsState = [=] {
			const auto isCustom = (CurrentConfig().avatarShape == AvatarShape::Custom);
			radiusSlider->setDisabled(!isCustom);
			singleCornerCheckbox->setDisabled(!isCustom);
		};
		updateShapeControlsState();

		shapeGroup->setChangedCallback([=](int val) {
			CurrentConfig().avatarShape = static_cast<AvatarShape>(val);
			SaveConfig();
			updateShapeControlsState();
			avatarPreview->update();
			IncrementAvatarShapeVersion();
		});

		radiusSlider->sectionActivated() | rpl::on_next([=](int section) {
			CurrentConfig().avatarCornerRadius = (section + 1) * 10;
			SaveConfig();
			avatarPreview->update();
			IncrementAvatarShapeVersion();
		}, box->lifetime());

		singleCornerCheckbox->checkedChanges() | rpl::on_next([=](bool checked) {
			CurrentConfig().singleCornerRadius = checked;
			SaveConfig();
			avatarPreview->update();
			IncrementAvatarShapeVersion();
		}, box->lifetime());

		addToggle(tabAppearance, QString::fromUtf8("Отключить кастомные фоны каналов"), "", CurrentConfig().disableCustomBackgrounds, [](bool c) { CurrentConfig().disableCustomBackgrounds = c; });
		addToggle(tabAppearance, QString::fromUtf8("Скрыть эмодзи-статусы Premium"), "", CurrentConfig().hidePremiumStatuses, [](bool c) { CurrentConfig().hidePremiumStatuses = c; });
		addToggle(tabAppearance, QString::fromUtf8("Моноширинный шрифт в сообщениях"), "", !CurrentConfig().monospaceFont.isEmpty(), [](bool c) {
			CurrentConfig().monospaceFont = c ? "Consolas" : "";
		});
		addToggle(tabAppearance, QString::fromUtf8("Скрыть счетчики на папках"), "", CurrentConfig().hideFolderCounters, [](bool c) { CurrentConfig().hideFolderCounters = c; });
		addToggle(tabAppearance, QString::fromUtf8("Скрыть вкладку 'Все чаты'"), "", CurrentConfig().hideAllChatsTab, [](bool c) { CurrentConfig().hideAllChatsTab = c; });
		addToggle(tabAppearance, QString::fromUtf8("Невидимка в трее"), "", CurrentConfig().trayGhostMode, [](bool c) { CurrentConfig().trayGhostMode = c; });
		addToggle(tabAppearance, QString::fromUtf8("Режим стримера в трее"), "", CurrentConfig().trayStreamerMode, [](bool c) { CurrentConfig().trayStreamerMode = c; });

		// ------------------ TAB 3: CHATS ------------------
		const auto bubblePreview = tabChats->add(object_ptr<BubblePreviewWidget>(tabChats));

		tabChats->add(object_ptr<Ui::FlatLabel>(
			tabChats,
			rpl::single(QString::fromUtf8("Радиус пузыря сообщений")),
			st::boxDividerLabel),
			st::boxRowPadding);

		const auto bubbleRadiusSlider = tabChats->add(
			object_ptr<Ui::SettingsSlider>(tabChats, st::settingsSlider),
			st::boxRowPadding);
		bubbleRadiusSlider->addSection("8px");
		bubbleRadiusSlider->addSection("12px");
		bubbleRadiusSlider->addSection("16px");
		bubbleRadiusSlider->addSection("20px");
		bubbleRadiusSlider->addSection("24px");
		const auto initialBubbleIndex = std::clamp((CurrentConfig().messageBubbleRadius - 8) / 4, 0, 4);
		bubbleRadiusSlider->setActiveSectionFast(initialBubbleIndex);
		bubbleRadiusSlider->sectionActivated() | rpl::on_next([=](int section) {
			CurrentConfig().messageBubbleRadius = 8 + section * 4;
			SaveConfig();
			bubblePreview->update();
		}, box->lifetime());

		addToggle(tabChats, QString::fromUtf8("Убрать хвостик сообщений"), "", CurrentConfig().removeMessageTail, [=](bool c) {
			CurrentConfig().removeMessageTail = c;
			bubblePreview->update();
		});

		tabChats->add(object_ptr<Ui::FlatLabel>(
			tabChats,
			rpl::single(QString::fromUtf8("Ширина сообщений")),
			st::boxDividerLabel),
			st::boxRowPadding);

		const auto wideSlider = tabChats->add(
			object_ptr<Ui::SettingsSlider>(tabChats, st::settingsSlider),
			st::boxRowPadding);
		wideSlider->addSection("1.0x");
		wideSlider->addSection("1.2x");
		wideSlider->addSection("1.4x");
		wideSlider->addSection("1.6x");
		const auto initialWideIndex = std::clamp(int(std::round((CurrentConfig().wideMessagesMultiplier - 1.0) / 0.2)), 0, 3);
		wideSlider->setActiveSectionFast(initialWideIndex);
		wideSlider->sectionActivated() | rpl::on_next([=](int section) {
			CurrentConfig().wideMessagesMultiplier = 1.0 + section * 0.2;
			SaveConfig();
		}, box->lifetime());

		addToggle(tabChats, QString::fromUtf8("Полупрозрачность удаленных сообщений"), QString::fromUtf8("Отображать сообщения, спасенные Anti-Delete, с прозрачностью ~65%."), CurrentConfig().translucentDeletedMessages, [](bool c) { CurrentConfig().translucentDeletedMessages = c; });
		addToggle(tabChats, QString::fromUtf8("Скрыть боковую стрелку пересылки"), "", CurrentConfig().hideSideShareButton, [](bool c) { CurrentConfig().hideSideShareButton = c; });
		addToggle(tabChats, QString::fromUtf8("Отключить цветные ответы"), QString::fromUtf8("Использовать нейтральный цвет для всех цитат и ответов вместо цветов авторов."), CurrentConfig().disableColorfulReplies, [](bool c) { CurrentConfig().disableColorfulReplies = c; });

		tabChats->add(object_ptr<Ui::FlatLabel>(
			tabChats,
			rpl::single(QString::fromUtf8("Контекстное меню сообщений")),
			st::boxTitle),
			st::boxRowPadding);

		const auto addContextItemFilter = [&](const QString &title, ContextMenuDisplay &val) {
			tabChats->add(object_ptr<Ui::FlatLabel>(
				tabChats,
				rpl::single(title),
				st::boxDividerLabel),
				st::boxRowPadding);
			const auto slider = tabChats->add(
				object_ptr<Ui::SettingsSlider>(tabChats, st::settingsSlider),
				st::boxRowPadding);
			slider->addSection(QString::fromUtf8("Всегда"));
			slider->addSection(QString::fromUtf8("Shift / Ctrl"));
			slider->addSection(QString::fromUtf8("Скрыто"));
			slider->setActiveSectionFast(static_cast<int>(val));
			slider->sectionActivated() | rpl::on_next([=, &val](int section) {
				val = static_cast<ContextMenuDisplay>(section);
				SaveConfig();
			}, box->lifetime());
		};

		addContextItemFilter(QString::fromUtf8("Детали (Details)"), CurrentConfig().menuDetails);
		addContextItemFilter(QString::fromUtf8("Сообщения пользователя"), CurrentConfig().menuUserMessages);
		addContextItemFilter(QString::fromUtf8("Повторить (Repeat)"), CurrentConfig().menuRepeat);
		addContextItemFilter(QString::fromUtf8("Скриншот сообщения (Shot)"), CurrentConfig().menuMessageShot);
		addContextItemFilter(QString::fromUtf8("Скрыть локально"), CurrentConfig().menuHideLocally);

		// ------------------ TAB 4: OTHER ------------------
		addToggle(tabOther, QString::fromUtf8("Локальный Telegram Premium"), QString::fromUtf8("Клиентская разблокировка премиум-функций."), CurrentConfig().localPremium, [](bool c) { CurrentConfig().localPremium = c; });
		addToggle(tabOther, QString::fromUtf8("Отключить рекламу (Disable Ads)"), QString::fromUtf8("Полное отключение спонсированных сообщений и каналов."), CurrentConfig().disableAds, [](bool c) { CurrentConfig().disableAds = c; });
		addToggle(tabOther, QString::fromUtf8("Анти-удаление сообщений (Anti-Delete)"), QString::fromUtf8("Сохранять удаленные сообщения собеседников в чатах."), CurrentConfig().antiDelete, [](bool c) { CurrentConfig().antiDelete = c; });
		addToggle(tabOther, QString::fromUtf8("Обход защиты от копирования"), QString::fromUtf8("Разрешить копирование текста и медиа из защищенных каналов."), CurrentConfig().bypassProtection, [](bool c) { CurrentConfig().bypassProtection = c; });
		addToggle(tabOther, QString::fromUtf8("Размытие фона медиавьювера"), QString::fromUtf8("Плавное размытие содержимого чата при открытии полноэкранного медиа."), CurrentConfig().blurMediaBackground, [](bool c) { CurrentConfig().blurMediaBackground = c; });

		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

} // namespace PixelGram
