/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/version.h"
#include "window/main_window.h"
#include "styles/style_layers.h"
#include "settings/sections/settings_pixelgram.h"
#include "core/pixelgram_link_guard.h"
#include "core/pixelgram_ws_proxy.h"
#include "ui/basic_click_handlers.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/layers/box_content.h"
#include "ui/layers/generic_box.h"

#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_main.h"
#include "data/data_session.h"
#include "main/main_session.h"
#include "data/data_chat_filters.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "settings/pixelgram_settings.h"
#include "ui/style/style_core_font.h"
#include "ui/pixelgram_avatar_shapes.h"
#include "ui/pixelgram_m3_scheme.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/discrete_sliders.h"
#include "ui/widgets/labels.h"
#include "ui/painter.h"
#include "window/window_session_controller.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "styles/style_menu_icons.h"

#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>

namespace Settings {
namespace {

using namespace Builder;
using namespace PixelGram;

// ------------------ Helper: Full-width Native Toggle Row ------------------
not_null<Ui::SettingsButton*> AddToggle(
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<QString> text,
		bool checked,
		Fn<void(bool)> callback) {
	const auto button = AddButtonWithIcon(container, std::move(text), st::settingsButton);
	button->setAllowTextLines(2);
	auto padding = button->st().padding;
	padding.setRight(70 + 16 + 12);
	button->setPaddingOverride(padding);
	const auto [toggle, checkView] = AddSeparatedToggle(button, st::settingsButton, checked);
	toggle->clicks() | rpl::on_next([=] {
		const auto val = !checkView->checked();
		checkView->setChecked(val, anim::type::normal);
		callback(val);
	}, button->lifetime());
	return button;
}

// ------------------ Top Header Widget ------------------
class PixelGramHeaderWidget : public Ui::RpWidget {
public:
	PixelGramHeaderWidget(QWidget *parent) : RpWidget(parent) {
		resize(parent->width(), style::ConvertScale(185));
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		PainterHighQualityEnabler hq(p);

		const auto &roles = CurrentM3Roles();
		const auto iconSize = style::ConvertScale(84);
		const auto iconTop = style::ConvertScale(14);
		const auto iconRect = QRect((width() - iconSize) / 2, iconTop, iconSize, iconSize);

		// Draw PixelGram Logo
		const auto &logo = Window::Logo();
		if (!logo.isNull()) {
			p.drawImage(iconRect, logo);
		}

		// Title: PixelGram Desktop v1.1 (v{AppVersionStr})
		const auto titleTop = iconTop + iconSize + style::ConvertScale(14);
		p.setFont(st::boxTitleFont);
		p.setPen(roles.onSurface.isValid() ? roles.onSurface : st::windowFg->c);
		const auto versionStr = QString("PixelGram Desktop v1.1 (v") + QString::fromLatin1(AppVersionStr) + QString(")");
		p.drawText(QRect(0, titleTop, width(), style::ConvertScale(26)), Qt::AlignCenter, versionStr);

		// Subtitle
		const auto subtitleTop = titleTop + style::ConvertScale(28);
		p.setFont(st::normalFont->f);
		p.setPen(roles.onSurfaceVariant.isValid() ? roles.onSurfaceVariant : st::windowSubTextFg->c);
		p.drawText(
			QRect(style::ConvertScale(24), subtitleTop, width() - style::ConvertScale(48), style::ConvertScale(42)),
			Qt::AlignCenter | Qt::TextWordWrap,
			u"Telegram Desktop fork powered by Material 3 Expressive and privacy-focused features."_q);
	}
};

// ------------------ Live Avatar Shape Preview Widget ------------------
class LiveAvatarPreview : public Ui::RpWidget {
public:
	LiveAvatarPreview(QWidget *parent) : RpWidget(parent) {
		resize(parent->width(), style::ConvertScale(88));
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		PainterHighQualityEnabler hq(p);

		const auto size = style::ConvertScale(64);
		const auto rect = QRect((width() - size) / 2, (height() - size) / 2, size, size);
		const auto &cfg = CurrentConfig();
		const auto path = AvatarShapePath(QRectF(rect), cfg.avatarShape, cfg.avatarCornerRadius);

		p.save();
		p.setClipPath(path);
		const auto &roles = CurrentM3Roles();
		QLinearGradient grad(rect.topLeft(), rect.bottomRight());
		grad.setColorAt(0.0, roles.primary.isValid() ? roles.primary : QColor(0x67, 0x50, 0xA4));
		grad.setColorAt(1.0, roles.tertiary.isValid() ? roles.tertiary : QColor(0x7D, 0x52, 0x60));
		p.fillRect(rect, grad);

		p.setFont(st::semiboldFont->f);
		p.setPen(QColor(0xFF, 0xFF, 0xFF));
		p.drawText(rect, Qt::AlignCenter, u"PG"_q);
		p.restore();
	}
};

// ------------------ Live Message Bubble Preview Widget ------------------
class LiveBubblePreview : public Ui::RpWidget {
public:
	LiveBubblePreview(QWidget *parent) : RpWidget(parent) {
		resize(parent->width(), 104);
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		PainterHighQualityEnabler hq(p);

		const auto &cfg = CurrentConfig();
		const auto radius = (cfg.messageBubbleRadius > 0) ? cfg.messageBubbleRadius : 16;
		const auto tailRadius = cfg.removeMessageTail ? radius : 2;

		const auto &roles = CurrentM3Roles();
		const auto inBg = roles.surfaceContainerHigh.isValid() ? roles.surfaceContainerHigh : QColor(0x2B, 0x29, 0x30);
		const auto inFg = roles.onSurface.isValid() ? roles.onSurface : QColor(0xE6, 0xE0, 0xE9);
		const auto outBg = roles.primary.isValid() ? roles.primary : QColor(0x67, 0x50, 0xA4);
		const auto outFg = roles.onPrimary.isValid() ? roles.onPrimary : QColor(0xFF, 0xFF, 0xFF);

		// Incoming bubble (left)
		{
			const QRect inRect(24, 10, 210, 36);
			QPainterPath inPath;
			if (cfg.removeMessageTail) {
				inPath.addRoundedRect(inRect, radius, radius);
			} else {
				inPath.moveTo(inRect.left() + radius, inRect.top());
				inPath.lineTo(inRect.right() - radius, inRect.top());
				inPath.quadTo(inRect.right(), inRect.top(), inRect.right(), inRect.top() + radius);
				inPath.lineTo(inRect.right(), inRect.bottom() - radius);
				inPath.quadTo(inRect.right(), inRect.bottom(), inRect.right() - radius, inRect.bottom());
				inPath.lineTo(inRect.left() + tailRadius, inRect.bottom());
				inPath.quadTo(inRect.left(), inRect.bottom(), inRect.left(), inRect.bottom() - tailRadius);
				inPath.lineTo(inRect.left(), inRect.top() + radius);
				inPath.quadTo(inRect.left(), inRect.top(), inRect.left() + radius, inRect.top());
			}

			p.fillPath(inPath, inBg);
			p.setPen(inFg);
			p.setFont(st::normalFont->f);
			p.drawText(inRect.adjusted(12, 0, -12, 0), Qt::AlignLeft | Qt::AlignVCenter, u"PixelGram Incoming"_q);
		}

		// Outgoing bubble (right)
		{
			const QRect outRect(width() - 234, 56, 210, 36);
			QPainterPath outPath;
			if (cfg.removeMessageTail) {
				outPath.addRoundedRect(outRect, radius, radius);
			} else {
				outPath.moveTo(outRect.left() + radius, outRect.top());
				outPath.lineTo(outRect.right() - radius, outRect.top());
				outPath.quadTo(outRect.right(), outRect.top(), outRect.right(), outRect.top() + radius);
				outPath.lineTo(outRect.right(), outRect.bottom() - tailRadius);
				outPath.quadTo(outRect.right(), outRect.bottom(), outRect.right() - tailRadius, outRect.bottom());
				outPath.lineTo(outRect.left() + radius, outRect.bottom());
				outPath.quadTo(outRect.left(), outRect.bottom(), outRect.left(), outRect.bottom() - radius);
				outPath.lineTo(outRect.left(), outRect.top() + radius);
				outPath.quadTo(outRect.left(), outRect.top(), outRect.left() + radius, outRect.top());
			}

			p.fillPath(outPath, outBg);
			p.setPen(outFg);
			p.setFont(st::normalFont->f);
			p.drawText(outRect.adjusted(12, 0, -12, 0), Qt::AlignLeft | Qt::AlignVCenter, u"PixelGram Outgoing"_q);
		}
	}
};

// ============================================================================
// 1. Root Section: PixelGram
// ============================================================================
class PixelGramSection final : public Section<PixelGramSection> {
public:
	PixelGramSection(QWidget *parent, not_null<Window::SessionController*> controller)
	: Section(parent, controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"PixelGram Desktop"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

		content->add(object_ptr<PixelGramHeaderWidget>(content));
		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		// 1. Ghost Mode with live count label
		const auto ghostLabel = rpl::single(QString::number(CurrentConfig().activeGhostCount()) + u"/5"_q);
		const auto ghostButton = AddButtonWithLabel(
			content,
			rpl::single(u"Невидимка (Ghost Mode)"_q),
			ghostLabel,
			st::settingsButton,
			{ &st::menuIconStealth });
		ghostButton->setClickedCallback([=] {
			controller()->showSettings(PixelGramGhostId());
		});

		// 2. Appearance
		const auto appearanceButton = AddButtonWithIcon(
			content,
			rpl::single(u"Внешний вид (Appearance)"_q),
			st::settingsButton,
			{ &st::menuIconChatBubble });
		appearanceButton->setClickedCallback([=] {
			controller()->showSettings(PixelGramAppearanceId());
		});

		// 3. Chats
		const auto chatsButton = AddButtonWithIcon(
			content,
			rpl::single(u"Чаты (Chats)"_q),
			st::settingsButton,
			{ &st::menuIconChatBubble });
		chatsButton->setClickedCallback([=] {
			controller()->showSettings(PixelGramChatsId());
		});

		// 4. Other
		const auto otherButton = AddButtonWithIcon(
			content,
			rpl::single(u"Другое (Other)"_q),
			st::settingsButton,
			{ &st::menuIconManage });
		otherButton->setClickedCallback([=] {
			controller()->showSettings(PixelGramOtherId());
		});

		// 5. Link Guard
		const auto linkGuardButton = AddButtonWithIcon(
			content,
			rpl::single(u"Link Guard (Защита ссылок)"_q),
			st::settingsButton,
			{ &st::menuIconManage });
		linkGuardButton->setClickedCallback([=] {
			controller()->showSettings(PixelGramLinkGuardId());
		});

		Ui::AddSkip(content, 16);
		Ui::ResizeFitChild(this, content);
	}
};

// ============================================================================
// 2. Subpage: Ghost Mode
// ============================================================================
class PixelGramGhostSection final : public Section<PixelGramGhostSection> {
public:
	PixelGramGhostSection(QWidget *parent, not_null<Window::SessionController*> controller)
	: Section(parent, controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"Невидимка"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
		auto &cfg = CurrentConfig();

		Ui::AddSubsectionTitle(content, rpl::single(u"Режим инкогнито"_q));

		AddToggle(content, rpl::single(u"Не передавать статус «В сети»"_q), cfg.dontSendOnline, [](bool val) {
			CurrentConfig().dontSendOnline = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Скрывать статус набора текста"_q), cfg.dontSendTyping, [](bool val) {
			CurrentConfig().dontSendTyping = val;
			CurrentConfig().hideTyping = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Не отмечать сообщения прочитанными"_q), cfg.dontReadMessages, [](bool val) {
			CurrentConfig().dontReadMessages = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Не отмечать истории просмотренными"_q), cfg.dontReadStories, [](bool val) {
			CurrentConfig().dontReadStories = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Отмечать прочитанным при отправке сообщения"_q), cfg.markReadAfterAction, [](bool val) {
			CurrentConfig().markReadAfterAction = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Автоматически переходить в офлайн"_q), cfg.goOfflineAutomatically, [](bool val) {
			CurrentConfig().goOfflineAutomatically = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 8);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Истории"_q));
		AddToggle(content, rpl::single(u"Предупреждать перед открытием историй"_q), cfg.storyGhostAlert, [](bool val) {
			CurrentConfig().storyGhostAlert = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 16);
		Ui::ResizeFitChild(this, content);
	}
};

// ============================================================================
// 3. Subpage: Appearance
// ============================================================================
class PixelGramAppearanceSection final : public Section<PixelGramAppearanceSection> {
public:
	PixelGramAppearanceSection(QWidget *parent, not_null<Window::SessionController*> controller)
	: Section(parent, controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"Внешний вид"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
		auto &cfg = CurrentConfig();

		Ui::AddSubsectionTitle(content, rpl::single(u"Форма аватарок Material 3"_q));

		const auto preview = content->add(object_ptr<LiveAvatarPreview>(content));
		Ui::AddSkip(content, 6);

		const auto shapeGroup = std::make_shared<Ui::RadioenumGroup<AvatarShape>>(cfg.avatarShape);
		const auto addRadio = [&](AvatarShape val, const QString &label) {
			content->add(
				object_ptr<Ui::Radioenum<AvatarShape>>(content, shapeGroup, val, label, st::defaultBoxCheckbox),
				style::margins(16, 4, 16, 4));
		};

		addRadio(AvatarShape::Circle, u"Круг (Circle)"_q);
		addRadio(AvatarShape::Squircle, u"Сквиркл (Squircle)"_q);
		addRadio(AvatarShape::Cookie, u"Печенье (Cookie M3)"_q);
		addRadio(AvatarShape::Sunflower, u"Цветок (Sunflower M3)"_q);
		addRadio(AvatarShape::Clover, u"Клевер (Clover M3)"_q);
		addRadio(AvatarShape::Hexagon, u"Шестиугольник (Hexagon M3)"_q);
		addRadio(AvatarShape::Custom, u"Свой радиус (Кастом)"_q);

		Ui::AddSkip(content, 6);
		content->add(object_ptr<Ui::FlatLabel>(
			content,
			rpl::single(u"Радиус углов (только для своего радиуса)"_q),
			st::boxDividerLabel),
			style::margins(16, 6, 16, 4));

		const auto radiusSlider = content->add(
			object_ptr<Ui::SettingsSlider>(content, st::settingsSlider),
			style::margins(16, 4, 16, 4));
		radiusSlider->addSection("10%");
		radiusSlider->addSection("20%");
		radiusSlider->addSection("30%");
		radiusSlider->addSection("40%");
		radiusSlider->addSection("50%");
		radiusSlider->setActiveSectionFast(std::clamp((cfg.avatarCornerRadius - 10) / 10, 0, 4));

		const auto singleCornerCheckbox = content->add(object_ptr<Ui::Checkbox>(
			content,
			u"Скруглять только один угол"_q,
			cfg.singleCornerRadius,
			st::defaultBoxCheckbox),
			style::margins(16, 6, 16, 8));

		const auto updateControlsState = [=] {
			const auto isCustom = (CurrentConfig().avatarShape == AvatarShape::Custom);
			radiusSlider->setDisabled(!isCustom);
			singleCornerCheckbox->setDisabled(!isCustom);
		};
		updateControlsState();

		shapeGroup->setChangedCallback([=](AvatarShape val) {
			CurrentConfig().avatarShape = val;
			SaveConfig();
			updateControlsState();
			preview->update();
			IncrementAvatarShapeVersion();
		});

		radiusSlider->sectionActivated() | rpl::on_next([=](int section) {
			CurrentConfig().avatarCornerRadius = (section + 1) * 10;
			SaveConfig();
			preview->update();
			IncrementAvatarShapeVersion();
		}, content->lifetime());

		singleCornerCheckbox->checkedChanges() | rpl::on_next([=](bool checked) {
			CurrentConfig().singleCornerRadius = checked;
			SaveConfig();
			preview->update();
			IncrementAvatarShapeVersion();
		}, content->lifetime());

		Ui::AddSkip(content, 8);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Интерфейс"_q));
		AddToggle(content, rpl::single(u"Отключить кастомные фоны чатов"_q), cfg.disableCustomBackgrounds, [](bool val) {
			CurrentConfig().disableCustomBackgrounds = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Скрывать эмодзи-статусы пользователей"_q), cfg.hidePremiumStatuses, [](bool val) {
			CurrentConfig().hidePremiumStatuses = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Моноширинный шрифт сообщений"_q), !cfg.monospaceFont.isEmpty(), [](bool val) {
			CurrentConfig().monospaceFont = val ? u"Roboto Mono"_q : QString();
			style::SetCustomMonospaceFont(CurrentConfig().monospaceFont);
			SaveConfig();
		});

		Ui::AddSkip(content, 8);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Папки и системный трей"_q));
		AddToggle(content, rpl::single(u"Скрыть вкладку «Все чаты»"_q), cfg.hideAllChatsTab, [=](bool val) {
			CurrentConfig().hideAllChatsTab = val;
			SaveConfig();
			controller()->session().data().chatsFilters().notifyChanged();
		});

		AddToggle(content, rpl::single(u"Монохромная иконка в трее"_q), cfg.trayGhostMode, [](bool val) {
			CurrentConfig().trayGhostMode = val;
			SaveConfig();
			Core::App().settings().setTrayIconMonochrome(val);
			Core::App().saveSettingsDelayed();
		});

		Ui::AddSkip(content, 16);
		Ui::ResizeFitChild(this, content);
	}
};

// ============================================================================
// 4. Subpage: Chats
// ============================================================================
class PixelGramChatsSection final : public Section<PixelGramChatsSection> {
public:
	PixelGramChatsSection(QWidget *parent, not_null<Window::SessionController*> controller)
	: Section(parent, controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"Чаты"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
		auto &cfg = CurrentConfig();

		Ui::AddSubsectionTitle(content, rpl::single(u"Пузыри сообщений Material 3"_q));

		const auto bubblePreview = content->add(object_ptr<LiveBubblePreview>(content));
		Ui::AddSkip(content, 6);

		content->add(object_ptr<Ui::FlatLabel>(
			content,
			rpl::single(u"Радиус скругления пузырей (px)"_q),
			st::boxDividerLabel),
			style::margins(16, 6, 16, 4));

		const auto bubbleRadiusSlider = content->add(
			object_ptr<Ui::SettingsSlider>(content, st::settingsSlider),
			style::margins(16, 4, 16, 8));
		bubbleRadiusSlider->addSection("8px");
		bubbleRadiusSlider->addSection("12px");
		bubbleRadiusSlider->addSection("16px");
		bubbleRadiusSlider->addSection("20px");
		bubbleRadiusSlider->addSection("24px");
		const auto radiusIndex = std::clamp((cfg.messageBubbleRadius - 8) / 4, 0, 4);
		bubbleRadiusSlider->setActiveSectionFast(radiusIndex);

		bubbleRadiusSlider->sectionActivated() | rpl::on_next([=](int section) {
			CurrentConfig().messageBubbleRadius = 8 + section * 4;
			SaveConfig();
			bubblePreview->update();
		}, content->lifetime());

		AddToggle(content, rpl::single(u"Отключить хвостик у сообщений"_q), cfg.removeMessageTail, [=](bool val) {
			CurrentConfig().removeMessageTail = val;
			SaveConfig();
			bubblePreview->update();
		});

		AddToggle(content, rpl::single(u"Полупрозрачность удалённых сообщений"_q), cfg.translucentDeletedMessages, [](bool val) {
			CurrentConfig().translucentDeletedMessages = val;
			SaveConfig();
		});

		const auto markText = [](AntiDeleteMark m) {
			switch (m) {
			case AntiDeleteMark::Text: return u"Текст «deleted»"_q;
			case AntiDeleteMark::Emoji: return u"Эмодзи «🗑️»"_q;
			case AntiDeleteMark::Hidden: return u"Скрыть метку"_q;
			}
			return u"Текст «deleted»"_q;
		};
		const auto markVal = std::make_shared<rpl::variable<QString>>(markText(cfg.antiDeleteMark));
		const auto markBtn = AddButtonWithLabel(
			content,
			rpl::single(u"Метка удалённых сообщений"_q),
			markVal->value(),
			st::settingsButton);
		markBtn->setClickedCallback([=] {
			auto &current = CurrentConfig().antiDeleteMark;
			current = (current == AntiDeleteMark::Text)
				? AntiDeleteMark::Emoji
				: (current == AntiDeleteMark::Emoji)
				? AntiDeleteMark::Hidden
				: AntiDeleteMark::Text;
			SaveConfig();
			*markVal = markText(current);
		});

		AddToggle(content, rpl::single(u"Скрыть стрелку пересылки рядом с сообщением"_q), cfg.hideSideShareButton, [](bool val) {
			CurrentConfig().hideSideShareButton = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Отключить цветные цитаты и ответы"_q), cfg.disableColorfulReplies, [](bool val) {
			CurrentConfig().disableColorfulReplies = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 8);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Контекстное меню сообщений"_q));

		const auto addMenuCycle = [&](const QString &title, ContextMenuDisplay &valRef) {
			const auto cycleVal = [=](ContextMenuDisplay curr) {
				return (curr == ContextMenuDisplay::Shown)
					? ContextMenuDisplay::Extended
					: (curr == ContextMenuDisplay::Extended)
					? ContextMenuDisplay::Hidden
					: ContextMenuDisplay::Shown;
			};
			const auto textForVal = [](ContextMenuDisplay v) {
				return (v == ContextMenuDisplay::Shown)
					? u"Всегда"_q
					: (v == ContextMenuDisplay::Extended)
					? u"Shift + Ctrl"_q
					: u"Скрыто"_q;
			};

			const auto btn = AddButtonWithLabel(
				content,
				rpl::single(title),
				rpl::single(textForVal(valRef)),
				st::settingsButton);

			btn->setClickedCallback([=, &valRef] {
				valRef = cycleVal(valRef);
				SaveConfig();
			});
		};

		addMenuCycle(u"Снимок сообщения"_q, cfg.menuMessageShot);
		addMenuCycle(u"Список реакций"_q, cfg.menuReactions);
		addMenuCycle(u"Просмотры сообщения"_q, cfg.menuViews);
		addMenuCycle(u"Скрыть локально"_q, cfg.menuHideLocally);
		addMenuCycle(u"Сообщения автора"_q, cfg.menuUserMessages);

		Ui::AddSkip(content, 8);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Строка ввода сообщения"_q));

		AddToggle(content, rpl::single(u"Отображать кнопку подарка"_q), cfg.showGiftButton, [](bool val) {
			CurrentConfig().showGiftButton = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Отображать кнопку записи голоса"_q), cfg.showVoiceButton, [](bool val) {
			CurrentConfig().showVoiceButton = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Звуки интерфейса (UI Sound Effects)"_q), cfg.uiSounds, [](bool val) {
			CurrentConfig().uiSounds = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Анимация морфинга кнопки записи (Material 3)"_q), cfg.dynamicVoiceMorph, [](bool val) {
			CurrentConfig().dynamicVoiceMorph = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Анимация набора и стирания текста (Fluid Typing & Erase)"_q), cfg.typingAnimation, [](bool val) {
			CurrentConfig().typingAnimation = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Анимированные касания (AnimTou - Fluid Touch Ripples)"_q), cfg.animTouRipples, [](bool val) {
			CurrentConfig().animTouRipples = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 16);
		Ui::ResizeFitChild(this, content);
	}
};

// ============================================================================
// 5. Subpage: Other
// ============================================================================
class PixelGramOtherSection final : public Section<PixelGramOtherSection> {
public:
	PixelGramOtherSection(QWidget *parent, not_null<Window::SessionController*> controller)
	: Section(parent, controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"Другое"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
		auto &cfg = CurrentConfig();

		Ui::AddSubsectionTitle(content, rpl::single(u"Расширенные функции"_q));

		AddToggle(content, rpl::single(u"Локальный Telegram Premium"_q), cfg.localPremium, [](bool val) {
			CurrentConfig().localPremium = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Отключить рекламу (Disable Ads)"_q), cfg.disableAds, [](bool val) {
			CurrentConfig().disableAds = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Анти-удаление сообщений"_q), cfg.antiDelete, [](bool val) {
			CurrentConfig().antiDelete = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Обход защиты от копирования"_q), cfg.bypassProtection, [](bool val) {
			CurrentConfig().bypassProtection = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Размытие фона медиавьювера"_q), cfg.blurMediaBackground, [](bool val) {
			CurrentConfig().blurMediaBackground = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Отображать Peer ID в профиле"_q), cfg.showPeerId, [](bool val) {
			CurrentConfig().showPeerId = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Подтверждение отправки"_q));

		AddToggle(content, rpl::single(u"Подтверждать отправку стикеров"_q), cfg.stickerConfirmation, [](bool val) {
			CurrentConfig().stickerConfirmation = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Подтверждать отправку голосовых сообщений"_q), cfg.voiceConfirmation, [](bool val) {
			CurrentConfig().voiceConfirmation = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Подтверждать отправку видеосообщений (кружков)"_q), cfg.roundConfirmation, [](bool val) {
			CurrentConfig().roundConfirmation = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Мини-приложения (Webview)"_q));

		AddToggle(content, rpl::single(u"Маскировать Webview под Android"_q), cfg.spoofWebviewAsAndroid, [](bool val) {
			CurrentConfig().spoofWebviewAsAndroid = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Увеличенная высота окна Webview"_q), cfg.increaseWebviewHeight, [](bool val) {
			CurrentConfig().increaseWebviewHeight = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Увеличенная ширина окна Webview"_q), cfg.increaseWebviewWidth, [](bool val) {
			CurrentConfig().increaseWebviewWidth = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		Ui::AddSubsectionTitle(content, rpl::single(u"Локальный прокси (WS-Proxy)"_q));

		AddToggle(content, rpl::single(u"Встроенный TG WS-Proxy"_q), cfg.wsProxyEnabled, [](bool val) {
			CurrentConfig().wsProxyEnabled = val;
			SaveConfig();
			if (val) {
				PixelGram::WsProxy::Start();
			} else {
				PixelGram::WsProxy::Stop();
			}
		});

		AddToggle(content, rpl::single(u"Отключить TLS для CF-прокси"_q), cfg.wsProxyDisableTls, [](bool val) {
			CurrentConfig().wsProxyDisableTls = val;
			SaveConfig();
			if (CurrentConfig().wsProxyEnabled) {
				PixelGram::WsProxy::Restart();
			}
		});

		AddButtonWithLabel(
			content,
			rpl::single(u"Статус"_q),
			PixelGram::WsProxy::StatusValue(),
			st::settingsButton,
			{});

		AddButtonWithLabel(
			content,
			rpl::single(u"Тип прокси"_q),
			rpl::single(u"MTProto"_q),
			st::settingsButton,
			{});

		auto portValue = content->lifetime().make_state<rpl::variable<QString>>(
			QString::number(cfg.wsProxyPort));

		const auto portBtn = AddButtonWithLabel(
			content,
			rpl::single(u"Адрес и порт"_q),
			portValue->value() | rpl::map([](const QString &p) { return u"127.0.0.1:"_q + p; }),
			st::settingsButton,
			{});

		portBtn->setClickedCallback([=] {
			controller()->show(Box([=](not_null<Ui::GenericBox*> box) {
				box->setTitle(rpl::single(u"Порт локального прокси"_q));
				const auto input = box->addRow(object_ptr<Ui::InputField>(
					box,
					st::defaultInputField,
					rpl::single(u"Порт (1-65535)"_q),
					QString::number(CurrentConfig().wsProxyPort)));
				box->addButton(rpl::single(u"Сохранить"_q), [=] {
					const auto text = input->getLastText().trimmed();
					bool ok = false;
					int port = text.toInt(&ok);
					if (ok && port > 0 && port < 65536) {
						CurrentConfig().wsProxyPort = port;
						SaveConfig();
						*portValue = QString::number(port);
						if (CurrentConfig().wsProxyEnabled) {
							PixelGram::WsProxy::Restart();
						}
						controller()->showToast(u"Порт изменён на %1"_q.arg(port));
					}
					box->closeBox();
				});
				box->addButton(rpl::single(u"Отмена"_q), [=] { box->closeBox(); });
			}));
		});

		const auto fullSecret = PixelGram::WsProxy::FullSecret();
		const auto shortSecret = (fullSecret.size() > 16)
			? (fullSecret.left(8) + u"..."_q + fullSecret.right(8))
			: fullSecret;
		const auto secretBtn = AddButtonWithLabel(
			content,
			rpl::single(u"Секрет"_q),
			rpl::single(shortSecret),
			st::settingsButton,
			{});
		secretBtn->setClickedCallback([=] {
			QGuiApplication::clipboard()->setText(fullSecret);
			controller()->showToast(u"Секрет скопирован в буфер"_q);
		});

		const auto applyBtn = AddButtonWithIcon(
			content,
			rpl::single(u"Применить прокси в Telegram"_q),
			st::settingsButton);
		applyBtn->setClickedCallback([=] {
			UrlClickHandler::Open(PixelGram::WsProxy::ProxyLink());
		});

		const auto copyLinkBtn = AddButtonWithIcon(
			content,
			rpl::single(u"Скопировать ссылку tg://proxy"_q),
			st::settingsButton);
		copyLinkBtn->setClickedCallback([=] {
			const auto link = PixelGram::WsProxy::ProxyLink();
			QGuiApplication::clipboard()->setText(link);
			controller()->showToast(u"Ссылка tg://proxy скопирована"_q);
		});

		Ui::AddSkip(content, 16);
		Ui::ResizeFitChild(this, content);
	}
};

// Register build metadata for router navigation and back-stack
const auto kPixelGramMeta = BuildHelper({
	.id = PixelGramSection::Id(),
	.parentId = MainId(),
	.title = &tr::lng_menu_settings,
	.icon = &st::menuIconManage,
}, [](SectionBuilder &builder) {});

const auto kGhostMeta = BuildHelper({
	.id = PixelGramGhostSection::Id(),
	.parentId = PixelGramSection::Id(),
	.title = &tr::lng_settings_section_privacy,
	.icon = &st::menuIconStealth,
}, [](SectionBuilder &builder) {});

const auto kAppearanceMeta = BuildHelper({
	.id = PixelGramAppearanceSection::Id(),
	.parentId = PixelGramSection::Id(),
	.title = &tr::lng_settings_section_chat_settings,
	.icon = &st::menuIconChatBubble,
}, [](SectionBuilder &builder) {});

const auto kChatsMeta = BuildHelper({
	.id = PixelGramChatsSection::Id(),
	.parentId = PixelGramSection::Id(),
	.title = &tr::lng_settings_section_chat_settings,
	.icon = &st::menuIconChatBubble,
}, [](SectionBuilder &builder) {});


// ============================================================================
// 6. Section: Link Guard
// ============================================================================
class PixelGramLinkGuardSection final : public Section<PixelGramLinkGuardSection> {
public:
	PixelGramLinkGuardSection(QWidget *parent, not_null<Window::SessionController*> controller)
	: Section(parent, controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"Link Guard (Защита ссылок)"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
		auto &cfg = CurrentConfig();

		// 1. Checks
		Ui::AddSubsectionTitle(content, rpl::single(u"Проверка ссылок"_q));

		AddToggle(content, rpl::single(u"Проверять перед открытием"_q), cfg.linkGuardCheckEnabled, [](bool val) {
			CurrentConfig().linkGuardCheckEnabled = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Показывать разбор: Всегда (иначе только при подозрении)"_q), (cfg.linkGuardShowMode == 1), [](bool val) {
			CurrentConfig().linkGuardShowMode = val ? 1 : 0;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Разворачивать короткие ссылки"_q), cfg.linkGuardExpandShort, [](bool val) {
			CurrentConfig().linkGuardExpandShort = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Узнавать возраст домена (RDAP)"_q), cfg.linkGuardCheckAge, [](bool val) {
			CurrentConfig().linkGuardCheckAge = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		// 2. Anti-tracker
		Ui::AddSubsectionTitle(content, rpl::single(u"Антитрекер"_q));

		AddToggle(content, rpl::single(u"Чистить метки при переходе"_q), cfg.linkGuardCleanOnOpen, [](bool val) {
			CurrentConfig().linkGuardCleanOnOpen = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Чистить метки в моих сообщениях"_q), cfg.linkGuardCleanOutgoing, [](bool val) {
			CurrentConfig().linkGuardCleanOutgoing = val;
			SaveConfig();
		});

		AddToggle(content, rpl::single(u"Агрессивная чистка (ref, si, spm, from)"_q), cfg.linkGuardAggressive, [](bool val) {
			CurrentConfig().linkGuardAggressive = val;
			SaveConfig();
		});

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		// 3. Database
		Ui::AddSubsectionTitle(content, rpl::single(u"База мошеннических сайтов"_q));

		AddToggle(content, rpl::single(u"Проверять по базе мошеннических сайтов"_q), cfg.linkGuardUseDatabase, [](bool val) {
			CurrentConfig().linkGuardUseDatabase = val;
			SaveConfig();
		});

		const auto dbStatusBtn = AddButtonWithLabel(
			content,
			rpl::single(u"Обновить базу"_q),
			rpl::single(PixelGram::LinkGuard::DatabaseStatus(false)),
			st::settingsButton,
			{ &st::menuIconManage });
		dbStatusBtn->setClickedCallback([=] {
			controller()->showToast(u"Проверяю обновления базы…"_q);
			PixelGram::LinkGuard::RefreshDatabase(true, [=](bool success, int count) {
				if (success) {
					controller()->showToast(u"База обновлена (%1 записей)"_q.arg(count));
				} else {
					controller()->showToast(u"Не удалось обновить базу"_q);
				}
			});
		});

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		// 4. Whitelist
		Ui::AddSubsectionTitle(content, rpl::single(u"Исключения (Доверенные домены)"_q));

		const auto addDomainBtn = AddButtonWithIcon(
			content,
			rpl::single(u"+ Добавить домен в исключения"_q),
			st::settingsButton,
			{ &st::menuIconManage });
		addDomainBtn->setClickedCallback([=] {
			controller()->show(Box([=](not_null<Ui::GenericBox*> box) {
				box->setTitle(rpl::single(u"Новый домен"_q));
				const auto input = box->addRow(object_ptr<Ui::InputField>(
					box,
					st::defaultInputField,
					rpl::single(u"example.com"_q)));
				box->addButton(rpl::single(u"Добавить"_q), [=] {
					const auto dom = input->getLastText().trimmed();
					if (!dom.isEmpty()) {
						PixelGram::LinkGuard::AddWhitelistDomain(dom);
						controller()->showToast(u"%1 добавлен в исключения"_q.arg(dom));
					}
					box->closeBox();
				});
				box->addButton(rpl::single(u"Отмена"_q), [=] { box->closeBox(); });
			}));
		});

		for (const auto &domain : cfg.linkGuardWhitelist) {
			const auto btn = AddButtonWithLabel(
				content,
				rpl::single(domain),
				rpl::single(u"Удалить"_q),
				st::settingsButton,
				{ &st::menuIconDelete });
			btn->setClickedCallback([=] {
				PixelGram::LinkGuard::RemoveWhitelistDomain(domain);
				controller()->showToast(u"%1 удалён из исключений"_q.arg(domain));
			});
		}

		Ui::AddSkip(content, 12);
		Ui::AddDivider(content);
		Ui::AddSkip(content, 8);

		// 5. Statistics
		Ui::AddSubsectionTitle(content, rpl::single(u"Статистика"_q));

		AddButtonWithLabel(
			content,
			rpl::single(u"Вырезано меток слежения"_q),
			rpl::single(QString::number(cfg.linkGuardStatsCleaned)),
			st::settingsButton,
			{});

		AddButtonWithLabel(
			content,
			rpl::single(u"Предупреждений показано"_q),
			rpl::single(QString::number(cfg.linkGuardStatsWarned)),
			st::settingsButton,
			{});

		const auto resetStatsBtn = AddButtonWithIcon(
			content,
			rpl::single(u"Сбросить счётчики"_q),
			st::settingsButton,
			{ &st::menuIconDelete });
		resetStatsBtn->setClickedCallback([=] {
			PixelGram::LinkGuard::ResetStats();
			controller()->showToast(u"Счётчики сброшены"_q);
		});

		Ui::AddSkip(content, 16);
		Ui::ResizeFitChild(this, content);
	}
};

const auto kOtherMeta = BuildHelper({
	.id = PixelGramOtherSection::Id(),
	.parentId = PixelGramSection::Id(),
	.title = &tr::lng_settings_advanced,
	.icon = &st::menuIconManage,
}, [](SectionBuilder &builder) {});

const auto kLinkGuardMeta = BuildHelper({
	.id = PixelGramLinkGuardSection::Id(),
	.parentId = PixelGramSection::Id(),
	.title = &tr::lng_settings_section_privacy,
	.icon = &st::menuIconManage,
}, [](SectionBuilder &builder) {});

} // namespace

Type PixelGramId() {
	return PixelGramSection::Id();
}

Type PixelGramGhostId() {
	return PixelGramGhostSection::Id();
}

Type PixelGramAppearanceId() {
	return PixelGramAppearanceSection::Id();
}

Type PixelGramChatsId() {
	return PixelGramChatsSection::Id();
}

Type PixelGramOtherId() {
	return PixelGramOtherSection::Id();
}

Type PixelGramLinkGuardId() {
	return PixelGramLinkGuardSection::Id();
}

} // namespace Settings
