#include "apiwrap.h"
#include "settings/pixelgram_settings.h"

#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "main/main_session.h"
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

	// Base
	if (obj.contains("antiDelete")) globalConfig.antiDelete = obj["antiDelete"].toBool(true);
	if (obj.contains("bypassProtection")) globalConfig.bypassProtection = obj["bypassProtection"].toBool(true);
	if (obj.contains("blurMediaBackground")) globalConfig.blurMediaBackground = obj["blurMediaBackground"].toBool(true);

	// Ghost Mode
	if (obj.contains("ghostMode")) globalConfig.ghostMode = obj["ghostMode"].toBool(true);
	if (obj.contains("dontReadMessages")) globalConfig.dontReadMessages = obj["dontReadMessages"].toBool(globalConfig.ghostMode);
	if (obj.contains("dontReadStories")) globalConfig.dontReadStories = obj["dontReadStories"].toBool(globalConfig.ghostMode);
	if (obj.contains("dontSendOnline")) globalConfig.dontSendOnline = obj["dontSendOnline"].toBool(globalConfig.ghostMode);
	if (obj.contains("dontSendTyping")) globalConfig.dontSendTyping = obj["dontSendTyping"].toBool(true);
	if (obj.contains("hideTyping")) globalConfig.hideTyping = obj["hideTyping"].toBool(globalConfig.dontSendTyping);
	if (obj.contains("goOfflineAutomatically")) globalConfig.goOfflineAutomatically = obj["goOfflineAutomatically"].toBool(true);
	if (obj.contains("storyGhostAlert")) globalConfig.storyGhostAlert = obj["storyGhostAlert"].toBool(true);

	// Other
	if (obj.contains("localPremium")) globalConfig.localPremium = obj["localPremium"].toBool(false);
	if (obj.contains("disableAds")) globalConfig.disableAds = obj["disableAds"].toBool(true);

	// Appearance
	if (obj.contains("avatarShape")) globalConfig.avatarShape = static_cast<AvatarShape>(obj["avatarShape"].toInt(static_cast<int>(AvatarShape::Circle)));
	if (obj.contains("avatarCornerRadius")) globalConfig.avatarCornerRadius = obj["avatarCornerRadius"].toInt(50);
	if (obj.contains("singleCornerRadius")) globalConfig.singleCornerRadius = obj["singleCornerRadius"].toBool(false);
	if (obj.contains("disableCustomBackgrounds")) globalConfig.disableCustomBackgrounds = obj["disableCustomBackgrounds"].toBool(false);
	if (obj.contains("hidePremiumStatuses")) globalConfig.hidePremiumStatuses = obj["hidePremiumStatuses"].toBool(false);
	if (obj.contains("monospaceFont")) globalConfig.monospaceFont = obj["monospaceFont"].toString();
	if (obj.contains("hideFolderCounters")) globalConfig.hideFolderCounters = obj["hideFolderCounters"].toBool(false);
	if (obj.contains("hideAllChatsTab")) globalConfig.hideAllChatsTab = obj["hideAllChatsTab"].toBool(false);
	if (obj.contains("trayGhostMode")) globalConfig.trayGhostMode = obj["trayGhostMode"].toBool(true);
	if (obj.contains("trayStreamerMode")) globalConfig.trayStreamerMode = obj["trayStreamerMode"].toBool(false);
	if (obj.contains("drawerMyProfile")) globalConfig.drawerMyProfile = obj["drawerMyProfile"].toBool(true);
	if (obj.contains("drawerNewGroup")) globalConfig.drawerNewGroup = obj["drawerNewGroup"].toBool(true);
	if (obj.contains("drawerNewChannel")) globalConfig.drawerNewChannel = obj["drawerNewChannel"].toBool(true);
	if (obj.contains("drawerContacts")) globalConfig.drawerContacts = obj["drawerContacts"].toBool(true);
	if (obj.contains("drawerCalls")) globalConfig.drawerCalls = obj["drawerCalls"].toBool(true);
	if (obj.contains("drawerSavedMessages")) globalConfig.drawerSavedMessages = obj["drawerSavedMessages"].toBool(true);
	if (obj.contains("drawerNightMode")) globalConfig.drawerNightMode = obj["drawerNightMode"].toBool(true);
	if (obj.contains("drawerGhostMode")) globalConfig.drawerGhostMode = obj["drawerGhostMode"].toBool(true);
	if (obj.contains("drawerStreamerMode")) globalConfig.drawerStreamerMode = obj["drawerStreamerMode"].toBool(true);

	// Chats
	if (obj.contains("showOnlyAddedStickers")) globalConfig.showOnlyAddedStickers = obj["showOnlyAddedStickers"].toBool(false);
	if (obj.contains("unlimitedRecentStickers")) globalConfig.unlimitedRecentStickers = obj["unlimitedRecentStickers"].toBool(true);
	if (obj.contains("hideReactionsBar")) globalConfig.hideReactionsBar = obj["hideReactionsBar"].toBool(false);
	if (obj.contains("hideReactionsUnderMessage")) globalConfig.hideReactionsUnderMessage = obj["hideReactionsUnderMessage"].toBool(false);
	if (obj.contains("hideReactionsAnimations")) globalConfig.hideReactionsAnimations = obj["hideReactionsAnimations"].toBool(false);
	if (obj.contains("channelBottomAction")) globalConfig.channelBottomAction = static_cast<ChannelBottomAction>(obj["channelBottomAction"].toInt(0));
	if (obj.contains("quickAdminShortcuts")) globalConfig.quickAdminShortcuts = obj["quickAdminShortcuts"].toBool(false);
	if (obj.contains("disableGreetingSticker")) globalConfig.disableGreetingSticker = obj["disableGreetingSticker"].toBool(true);
	if (obj.contains("showMessageShot")) globalConfig.showMessageShot = obj["showMessageShot"].toBool(true);
	if (obj.contains("replaceWithIcons")) globalConfig.replaceWithIcons = obj["replaceWithIcons"].toBool(false);
	if (obj.contains("removeMessageTail")) globalConfig.removeMessageTail = obj["removeMessageTail"].toBool(true);
	if (obj.contains("hideSideShareButton")) globalConfig.hideSideShareButton = obj["hideSideShareButton"].toBool(false);
	if (obj.contains("disableColorfulReplies")) globalConfig.disableColorfulReplies = obj["disableColorfulReplies"].toBool(false);
	if (obj.contains("translucentDeletedMessages")) globalConfig.translucentDeletedMessages = obj["translucentDeletedMessages"].toBool(true);
	if (obj.contains("messageBubbleRadius")) globalConfig.messageBubbleRadius = obj["messageBubbleRadius"].toInt(16);
	if (obj.contains("wideMessagesMultiplier")) globalConfig.wideMessagesMultiplier = obj["wideMessagesMultiplier"].toDouble(1.0);
	if (obj.contains("menuReactions")) globalConfig.menuReactions = static_cast<ContextMenuDisplay>(obj["menuReactions"].toInt(0));
	if (obj.contains("menuViews")) globalConfig.menuViews = static_cast<ContextMenuDisplay>(obj["menuViews"].toInt(0));
	if (obj.contains("menuHideLocally")) globalConfig.menuHideLocally = static_cast<ContextMenuDisplay>(obj["menuHideLocally"].toInt(1));
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

	// Theme M3
	if (obj.contains("m3SeedColor")) {
		const auto hex = obj["m3SeedColor"].toString();
		if (!hex.isEmpty()) {
			bool ok = false;
			const auto val = hex.toUInt(&ok, 16);
			if (ok) globalConfig.m3SeedColor = val;
		}
	}
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
	obj["showMessageShot"] = globalConfig.showMessageShot;
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
			QString::fromUtf8("Размытие фона (Blur background)"),
			QString::fromUtf8("Размытие содержимого чата при открытии медиа в полный экран."),
			CurrentConfig().blurMediaBackground,
			[](bool checked) { CurrentConfig().blurMediaBackground = checked; });

		addToggle(
			QString::fromUtf8("Анти-удаление (Anti-Delete)"),
			QString::fromUtf8("Сохранять удалённые сообщения собеседников в чатах."),
			CurrentConfig().antiDelete,
			[](bool checked) { CurrentConfig().antiDelete = checked; });

		addToggle(
			QString::fromUtf8("Невидимка (Ghost Mode)"),
			QString::fromUtf8("Не отправлять статусы прочтения сообщений и историй."),
			CurrentConfig().ghostMode,
			[](bool checked) {
				CurrentConfig().setMasterGhostMode(checked);
			});

		addToggle(
			QString::fromUtf8("Скрыть статус набора (Hide Typing)"),
			QString::fromUtf8("Не отправлять статус 'печатает...' в чаты."),
			CurrentConfig().dontSendTyping,
			[](bool checked) {
				CurrentConfig().dontSendTyping = checked;
				CurrentConfig().hideTyping = checked;
			});

		addToggle(
			QString::fromUtf8("Обход защиты (Bypass Protection)"),
			QString::fromUtf8("Разрешить копирование и пересылку из защищённых чатов."),
			CurrentConfig().bypassProtection,
			[](bool checked) { CurrentConfig().bypassProtection = checked; });

		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

} // namespace PixelGram