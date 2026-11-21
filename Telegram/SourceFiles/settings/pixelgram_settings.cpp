/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/pixelgram_settings.h"
#include "base/random.h"
#include "apiwrap.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_session.h"
#include "data/data_chat_filters.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "ui/pixelgram_avatar_shapes.h"
#include "ui/pixelgram_m3_scheme.h"
#include "ui/style/style_core_font.h"

#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/discrete_sliders.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "window/window_session_controller.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "styles/style_chat.h"
#include "styles/style_settings.h"
#include "storage/localstorage.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "mainwindow.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>
#include <QtGui/QMouseEvent>

namespace PixelGram {
namespace {

Config globalConfig;
bool loaded = false;

QString ConfigFilePath() {
	return cWorkingDir() + u"tdata/pixelgram_settings.json"_q;
}

// ------------------ M3 Card Widget ------------------
class CardWidget : public Ui::VerticalLayout {
public:
	CardWidget(QWidget *parent) : Ui::VerticalLayout(parent) {
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		const auto &roles = CurrentM3Roles();
		p.setPen(Qt::NoPen);
		p.setBrush(roles.surfaceContainerLow);
		p.drawRoundedRect(rect(), 16., 16.);
	}
};

// ------------------ M3 Segmented Pill Tabs ------------------
class M3SegmentedTabs : public Ui::RpWidget {
public:
	M3SegmentedTabs(QWidget *parent, const std::vector<QString> &tabs)
	: RpWidget(parent)
	, _tabs(tabs) {
		resize(460, 44);
		setMouseTracking(true);
	}

	void setChangedCallback(Fn<void(int)> callback) {
		_callback = std::move(callback);
	}

	void setActiveTab(int index) {
		if (index >= 0 && index < int(_tabs.size()) && _active != index) {
			_active = index;
			update();
		}
	}

	[[nodiscard]] int activeTab() const {
		return _active;
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::TextAntialiasing);

		const auto count = int(_tabs.size());
		if (count <= 0) return;

		const auto marginH = 16;
		const auto h = 36;
		const auto y = (height() - h) / 2;
		const auto totalW = width() - marginH * 2;
		const auto tabW = float(totalW) / count;

		const auto &roles = CurrentM3Roles();

		// Track
		p.setPen(Qt::NoPen);
		p.setBrush(roles.surfaceContainer);
		p.drawRoundedRect(QRectF(marginH, y, totalW, h), h / 2.0, h / 2.0);

		p.setFont(st::semiboldFont->f);

		for (int i = 0; i < count; ++i) {
			const auto tabRect = QRectF(marginH + i * tabW, y, tabW, h);
			if (i == _active) {
				p.setPen(Qt::NoPen);
				p.setBrush(roles.secondaryContainer);
				p.drawRoundedRect(tabRect.adjusted(2, 2, -2, -2), (h - 4) / 2.0, (h - 4) / 2.0);
				p.setPen(roles.onSecondaryContainer);
			} else {
				if (i == _hovered) {
					p.setPen(Qt::NoPen);
					auto hoverBg = roles.surfaceContainerHighest;
					hoverBg.setAlpha(120);
					p.setBrush(hoverBg);
					p.drawRoundedRect(tabRect.adjusted(2, 2, -2, -2), (h - 4) / 2.0, (h - 4) / 2.0);
				}
				p.setPen(roles.onSurfaceVariant);
			}
			p.drawText(tabRect, Qt::AlignCenter, _tabs[i]);
		}
	}

	void mousePressEvent(QMouseEvent *e) override {
		const auto count = int(_tabs.size());
		if (count <= 0) return;
		const auto marginH = 16;
		const auto totalW = width() - marginH * 2;
		const auto tabW = float(totalW) / count;
		const auto clickX = e->pos().x() - marginH;
		if (clickX >= 0 && clickX < totalW) {
			const auto idx = std::clamp(int(clickX / tabW), 0, count - 1);
			if (idx != _active) {
				_active = idx;
				update();
				if (_callback) {
					_callback(_active);
				}
			}
		}
	}

	void mouseMoveEvent(QMouseEvent *e) override {
		const auto count = int(_tabs.size());
		if (count <= 0) return;
		const auto marginH = 16;
		const auto totalW = width() - marginH * 2;
		const auto tabW = float(totalW) / count;
		const auto clickX = e->pos().x() - marginH;
		int newHovered = -1;
		if (clickX >= 0 && clickX < totalW) {
			newHovered = std::clamp(int(clickX / tabW), 0, count - 1);
		}
		if (newHovered != _hovered) {
			_hovered = newHovered;
			update();
		}
	}

	void leaveEventHook(QEvent *e) override {
		_hovered = -1;
		update();
	}

private:
	std::vector<QString> _tabs;
	int _active = 0;
	int _hovered = -1;
	Fn<void(int)> _callback;
};

// ------------------ Avatar Preview Widget ------------------
class AvatarPreviewWidget : public Ui::RpWidget {
public:
	AvatarPreviewWidget(QWidget *parent) : RpWidget(parent) {
		resize(428, 92);
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setRenderHint(QPainter::Antialiasing);

		const auto size = 68;
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

// ------------------ Bubble Preview Widget ------------------
class BubblePreviewWidget : public Ui::RpWidget {
public:
	BubblePreviewWidget(QWidget *parent) : RpWidget(parent) {
		resize(428, 108);
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
			QRect inRect(16, 10, 200, 38);
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
			p.setFont(st::normalFont->f);
			p.drawText(inRect.adjusted(14, 0, -14, 0), Qt::AlignLeft | Qt::AlignVCenter, "PixelGram Incoming");
		}

		// Outgoing bubble (right)
		{
			QRect outRect(width() - 216, 56, 200, 38);
			QPainterPath outPath;
			outPath.moveTo(outRect.left() + radius, outRect.top());
			outPath.lineTo(outRect.right() - radius, outRect.top());
			outPath.quadTo(outRect.right(), outRect.top(), outRect.right(), outRect.top() + radius);
			outPath.lineTo(outRect.right(), outRect.bottom() - tailRadius);
			outPath.quadTo(outRect.right(), outRect.bottom(), outRect.right() - tailRadius, outRect.bottom());
			outPath.lineTo(outRect.left() + radius, outRect.bottom());
			outPath.quadTo(outRect.left(), outRect.bottom(), outRect.left(), outRect.bottom() - radius);
			outPath.lineTo(outRect.left(), outRect.top() + radius);
			outPath.quadTo(outRect.left(), outRect.top(), outRect.left() + radius, outRect.top());

			const auto &roles = CurrentM3Roles();
			p.fillPath(outPath, roles.primary);
			p.setPen(roles.onPrimary);
			p.setFont(st::normalFont->f);
			p.drawText(outRect.adjusted(14, 0, -14, 0), Qt::AlignLeft | Qt::AlignVCenter, "PixelGram Outgoing");
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

void SaveConfig() {
	QJsonObject root;
	const auto &cfg = CurrentConfig();

	// Base
	root["antiDelete"] = cfg.antiDelete;
	root["antiDeleteMark"] = static_cast<int>(cfg.antiDeleteMark);
	root["bypassProtection"] = cfg.bypassProtection;
	root["blurMediaBackground"] = cfg.blurMediaBackground;

	// Ghost Mode
	root["ghostMode"] = cfg.ghostMode;
	root["dontReadMessages"] = cfg.dontReadMessages;
	root["dontReadStories"] = cfg.dontReadStories;
	root["dontSendOnline"] = cfg.dontSendOnline;
	root["dontSendTyping"] = cfg.dontSendTyping;
	root["hideTyping"] = cfg.hideTyping;
	root["goOfflineAutomatically"] = cfg.goOfflineAutomatically;
	root["storyGhostAlert"] = cfg.storyGhostAlert;

	// Other
	root["localPremium"] = cfg.localPremium;
	root["disableAds"] = cfg.disableAds;
	root["localColorIndex"] = cfg.localColorIndex;
	root["localBackgroundEmojiId"] = qint64(cfg.localBackgroundEmojiId);
	root["localProfileColorIndex"] = cfg.localProfileColorIndex;
	root["localProfileBackgroundEmojiId"] = qint64(cfg.localProfileBackgroundEmojiId);
	root["localEmojiStatusId"] = qint64(cfg.localEmojiStatusId);

	// Appearance & M3
	root["avatarShape"] = static_cast<int>(cfg.avatarShape);
	root["avatarCornerRadius"] = cfg.avatarCornerRadius;
	root["singleCornerRadius"] = cfg.singleCornerRadius;
	root["disableCustomBackgrounds"] = cfg.disableCustomBackgrounds;
	root["hidePremiumStatuses"] = cfg.hidePremiumStatuses;
	root["monospaceFont"] = cfg.monospaceFont;
	root["hideAllChatsTab"] = cfg.hideAllChatsTab;
	root["trayGhostMode"] = cfg.trayGhostMode;
	root["monochromeTrayIcon"] = cfg.trayGhostMode;
	root["trayStreamerMode"] = cfg.trayStreamerMode;
	root["streamerModeActive"] = cfg.streamerModeActive;
	root["drawerMyProfile"] = cfg.drawerMyProfile;
	root["drawerNewGroup"] = cfg.drawerNewGroup;
	root["drawerNewChannel"] = cfg.drawerNewChannel;
	root["drawerContacts"] = cfg.drawerContacts;
	root["drawerCalls"] = cfg.drawerCalls;
	root["drawerSavedMessages"] = cfg.drawerSavedMessages;
	root["drawerNightMode"] = cfg.drawerNightMode;
	root["drawerGhostMode"] = cfg.drawerGhostMode;
	root["drawerStreamerMode"] = cfg.drawerStreamerMode;

	// Chats
	root["showOnlyAddedStickers"] = cfg.showOnlyAddedStickers;
	root["unlimitedRecentStickers"] = cfg.unlimitedRecentStickers;
	root["hideReactionsBar"] = cfg.hideReactionsBar;
	root["hideReactionsUnderMessage"] = cfg.hideReactionsUnderMessage;
	root["hideReactionsAnimations"] = cfg.hideReactionsAnimations;
	root["channelBottomAction"] = static_cast<int>(cfg.channelBottomAction);
	root["quickAdminShortcuts"] = cfg.quickAdminShortcuts;
	root["disableGreetingSticker"] = cfg.disableGreetingSticker;
	root["menuMessageShot"] = static_cast<int>(cfg.menuMessageShot);
	root["replaceWithIcons"] = cfg.replaceWithIcons;
	root["removeMessageTail"] = cfg.removeMessageTail;
	root["hideSideShareButton"] = cfg.hideSideShareButton;
	root["disableColorfulReplies"] = cfg.disableColorfulReplies;
	root["translucentDeletedMessages"] = cfg.translucentDeletedMessages;
	root["messageBubbleRadius"] = cfg.messageBubbleRadius;
	root["menuReactions"] = static_cast<int>(cfg.menuReactions);
	root["menuViews"] = static_cast<int>(cfg.menuViews);
	root["menuHideLocally"] = static_cast<int>(cfg.menuHideLocally);
	root["menuUserMessages"] = static_cast<int>(cfg.menuUserMessages);
	root["menuDetails"] = static_cast<int>(cfg.menuDetails);
	root["menuRepeat"] = static_cast<int>(cfg.menuRepeat);
	root["showAttachButton"] = cfg.showAttachButton;
	root["showCommandsButton"] = cfg.showCommandsButton;
	root["showTtlButton"] = cfg.showTtlButton;
	root["showEmojiButton"] = cfg.showEmojiButton;
	root["showVoiceButton"] = cfg.showVoiceButton;
	root["showGiftButton"] = cfg.showGiftButton;
	root["openAttachOnHover"] = cfg.openAttachOnHover;
	root["openEmojiOnHover"] = cfg.openEmojiOnHover;

	// Webview
	root["spoofWebviewAsAndroid"] = cfg.spoofWebviewAsAndroid;
	root["increaseWebviewHeight"] = cfg.increaseWebviewHeight;
	root["increaseWebviewWidth"] = cfg.increaseWebviewWidth;

	// Ghost Extensions
	root["markReadAfterAction"] = cfg.markReadAfterAction;

	// Profile & UI
	root["showPeerId"] = cfg.showPeerId;

	// Confirmations
	root["stickerConfirmation"] = cfg.stickerConfirmation;
	root["voiceConfirmation"] = cfg.voiceConfirmation;
	root["roundConfirmation"] = cfg.roundConfirmation;

	// Link Guard
	root["linkGuardCheckEnabled"] = cfg.linkGuardCheckEnabled;
	root["linkGuardShowMode"] = cfg.linkGuardShowMode;
	root["linkGuardExpandShort"] = cfg.linkGuardExpandShort;
	root["linkGuardCheckAge"] = cfg.linkGuardCheckAge;
	root["linkGuardCleanOnOpen"] = cfg.linkGuardCleanOnOpen;
	root["linkGuardCleanOutgoing"] = cfg.linkGuardCleanOutgoing;
	root["linkGuardAggressive"] = cfg.linkGuardAggressive;
	root["linkGuardUseDatabase"] = cfg.linkGuardUseDatabase;
	{
		QJsonArray wl;
		for (const auto &d : cfg.linkGuardWhitelist) wl.append(d);
		root["linkGuardWhitelist"] = wl;
	}
	root["linkGuardStatsCleaned"] = static_cast<double>(cfg.linkGuardStatsCleaned);
	root["linkGuardStatsWarned"] = static_cast<double>(cfg.linkGuardStatsWarned);
	root["linkGuardDbBuilt"] = cfg.linkGuardDbBuilt;
	root["linkGuardDbEntries"] = cfg.linkGuardDbEntries;
	root["linkGuardDbCheckedAt"] = static_cast<double>(cfg.linkGuardDbCheckedAt);

	root["m3SeedColor"] = static_cast<double>(cfg.m3SeedColor);
	root["m3SchemeVariant"] = cfg.m3SchemeVariant;
	root["m3ContrastLevel"] = cfg.m3ContrastLevel;

	root["wsProxyEnabled"] = cfg.wsProxyEnabled;
	root["wsProxyPort"] = cfg.wsProxyPort;
	root["wsProxySecret"] = cfg.wsProxySecret;
	root["wsProxyDisableTls"] = cfg.wsProxyDisableTls;

	QFile file(ConfigFilePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	}
}

QString GenerateRandomSecret() {
	auto bytes = std::array<uint8_t, 16>();
	base::RandomFill(bytes.data(), bytes.size());
	return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(bytes.data()), int(bytes.size())).toHex().toLower());
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
	const auto root = doc.object();
	auto &cfg = globalConfig;

	if (root.contains("antiDelete")) cfg.antiDelete = root["antiDelete"].toBool();
	if (root.contains("antiDeleteMark")) cfg.antiDeleteMark = static_cast<AntiDeleteMark>(root["antiDeleteMark"].toInt());
	if (root.contains("bypassProtection")) cfg.bypassProtection = root["bypassProtection"].toBool();
	if (root.contains("blurMediaBackground")) cfg.blurMediaBackground = root["blurMediaBackground"].toBool();

	if (root.contains("ghostMode")) cfg.ghostMode = root["ghostMode"].toBool();
	if (root.contains("dontReadMessages")) cfg.dontReadMessages = root["dontReadMessages"].toBool();
	if (root.contains("dontReadStories")) cfg.dontReadStories = root["dontReadStories"].toBool();
	if (root.contains("dontSendOnline")) cfg.dontSendOnline = root["dontSendOnline"].toBool();
	if (root.contains("dontSendTyping")) cfg.dontSendTyping = root["dontSendTyping"].toBool();
	if (root.contains("hideTyping")) cfg.hideTyping = root["hideTyping"].toBool();
	if (root.contains("goOfflineAutomatically")) cfg.goOfflineAutomatically = root["goOfflineAutomatically"].toBool();
	if (root.contains("storyGhostAlert")) cfg.storyGhostAlert = root["storyGhostAlert"].toBool();

	if (root.contains("localPremium")) cfg.localPremium = root["localPremium"].toBool();
	if (root.contains("disableAds")) cfg.disableAds = root["disableAds"].toBool();
	if (root.contains("localColorIndex")) cfg.localColorIndex = root["localColorIndex"].toInt();
	if (root.contains("localBackgroundEmojiId")) cfg.localBackgroundEmojiId = uint64(root["localBackgroundEmojiId"].toVariant().toLongLong());
	if (root.contains("localProfileColorIndex")) cfg.localProfileColorIndex = root["localProfileColorIndex"].toInt();
	if (root.contains("localProfileBackgroundEmojiId")) cfg.localProfileBackgroundEmojiId = uint64(root["localProfileBackgroundEmojiId"].toVariant().toLongLong());
	if (root.contains("localEmojiStatusId")) cfg.localEmojiStatusId = uint64(root["localEmojiStatusId"].toVariant().toLongLong());

	if (root.contains("avatarShape")) cfg.avatarShape = static_cast<AvatarShape>(root["avatarShape"].toInt());
	if (root.contains("avatarCornerRadius")) cfg.avatarCornerRadius = root["avatarCornerRadius"].toInt();
	if (root.contains("singleCornerRadius")) cfg.singleCornerRadius = root["singleCornerRadius"].toBool();
	if (root.contains("disableCustomBackgrounds")) cfg.disableCustomBackgrounds = root["disableCustomBackgrounds"].toBool();
	if (root.contains("hidePremiumStatuses")) cfg.hidePremiumStatuses = root["hidePremiumStatuses"].toBool();
	if (root.contains("monospaceFont")) {
		cfg.monospaceFont = root["monospaceFont"].toString();
		if (!cfg.monospaceFont.isEmpty()) {
			style::SetCustomMonospaceFont(cfg.monospaceFont);
		}
	}
	if (root.contains("hideAllChatsTab")) cfg.hideAllChatsTab = root["hideAllChatsTab"].toBool();
	if (root.contains("trayGhostMode")) cfg.trayGhostMode = root["trayGhostMode"].toBool();
	if (root.contains("monochromeTrayIcon")) cfg.trayGhostMode = root["monochromeTrayIcon"].toBool();
	Core::App().settings().setTrayIconMonochrome(cfg.trayGhostMode);
	if (root.contains("trayStreamerMode")) cfg.trayStreamerMode = root["trayStreamerMode"].toBool();
	if (root.contains("streamerModeActive")) cfg.streamerModeActive = root["streamerModeActive"].toBool();
	if (root.contains("drawerMyProfile")) cfg.drawerMyProfile = root["drawerMyProfile"].toBool();
	if (root.contains("drawerNewGroup")) cfg.drawerNewGroup = root["drawerNewGroup"].toBool();
	if (root.contains("drawerNewChannel")) cfg.drawerNewChannel = root["drawerNewChannel"].toBool();
	if (root.contains("drawerContacts")) cfg.drawerContacts = root["drawerContacts"].toBool();
	if (root.contains("drawerCalls")) cfg.drawerCalls = root["drawerCalls"].toBool();
	if (root.contains("drawerSavedMessages")) cfg.drawerSavedMessages = root["drawerSavedMessages"].toBool();
	if (root.contains("drawerNightMode")) cfg.drawerNightMode = root["drawerNightMode"].toBool();
	if (root.contains("drawerGhostMode")) cfg.drawerGhostMode = root["drawerGhostMode"].toBool();
	if (root.contains("drawerStreamerMode")) cfg.drawerStreamerMode = root["drawerStreamerMode"].toBool();

	if (root.contains("showOnlyAddedStickers")) cfg.showOnlyAddedStickers = root["showOnlyAddedStickers"].toBool();
	if (root.contains("unlimitedRecentStickers")) cfg.unlimitedRecentStickers = root["unlimitedRecentStickers"].toBool();
	if (root.contains("hideReactionsBar")) cfg.hideReactionsBar = root["hideReactionsBar"].toBool();
	if (root.contains("hideReactionsUnderMessage")) cfg.hideReactionsUnderMessage = root["hideReactionsUnderMessage"].toBool();
	if (root.contains("hideReactionsAnimations")) cfg.hideReactionsAnimations = root["hideReactionsAnimations"].toBool();
	if (root.contains("channelBottomAction")) cfg.channelBottomAction = static_cast<ChannelBottomAction>(root["channelBottomAction"].toInt());
	if (root.contains("quickAdminShortcuts")) cfg.quickAdminShortcuts = root["quickAdminShortcuts"].toBool();
	if (root.contains("disableGreetingSticker")) cfg.disableGreetingSticker = root["disableGreetingSticker"].toBool();
	if (root.contains("menuMessageShot")) cfg.menuMessageShot = static_cast<ContextMenuDisplay>(root["menuMessageShot"].toInt());
	if (root.contains("replaceWithIcons")) cfg.replaceWithIcons = root["replaceWithIcons"].toBool();
	if (root.contains("removeMessageTail")) cfg.removeMessageTail = root["removeMessageTail"].toBool();
	if (root.contains("hideSideShareButton")) cfg.hideSideShareButton = root["hideSideShareButton"].toBool();
	if (root.contains("disableColorfulReplies")) cfg.disableColorfulReplies = root["disableColorfulReplies"].toBool();
	if (root.contains("translucentDeletedMessages")) cfg.translucentDeletedMessages = root["translucentDeletedMessages"].toBool();
	if (root.contains("messageBubbleRadius")) cfg.messageBubbleRadius = root["messageBubbleRadius"].toInt();
	if (root.contains("menuReactions")) cfg.menuReactions = static_cast<ContextMenuDisplay>(root["menuReactions"].toInt());
	if (root.contains("menuViews")) cfg.menuViews = static_cast<ContextMenuDisplay>(root["menuViews"].toInt());
	if (root.contains("menuHideLocally")) cfg.menuHideLocally = static_cast<ContextMenuDisplay>(root["menuHideLocally"].toInt());
	if (root.contains("menuUserMessages")) cfg.menuUserMessages = static_cast<ContextMenuDisplay>(root["menuUserMessages"].toInt());
	if (root.contains("menuDetails")) cfg.menuDetails = static_cast<ContextMenuDisplay>(root["menuDetails"].toInt());
	if (root.contains("menuRepeat")) cfg.menuRepeat = static_cast<ContextMenuDisplay>(root["menuRepeat"].toInt());
	if (root.contains("showAttachButton")) cfg.showAttachButton = root["showAttachButton"].toBool();
	if (root.contains("showCommandsButton")) cfg.showCommandsButton = root["showCommandsButton"].toBool();
	if (root.contains("showTtlButton")) cfg.showTtlButton = root["showTtlButton"].toBool();
	if (root.contains("showEmojiButton")) cfg.showEmojiButton = root["showEmojiButton"].toBool();
	if (root.contains("showVoiceButton")) cfg.showVoiceButton = root["showVoiceButton"].toBool();
	if (root.contains("showGiftButton")) cfg.showGiftButton = root["showGiftButton"].toBool();
	if (root.contains("openAttachOnHover")) cfg.openAttachOnHover = root["openAttachOnHover"].toBool();
	if (root.contains("openEmojiOnHover")) cfg.openEmojiOnHover = root["openEmojiOnHover"].toBool();

	// Webview
	if (root.contains("spoofWebviewAsAndroid")) cfg.spoofWebviewAsAndroid = root["spoofWebviewAsAndroid"].toBool();
	if (root.contains("increaseWebviewHeight")) cfg.increaseWebviewHeight = root["increaseWebviewHeight"].toBool();
	if (root.contains("increaseWebviewWidth")) cfg.increaseWebviewWidth = root["increaseWebviewWidth"].toBool();

	// Ghost Extensions
	if (root.contains("markReadAfterAction")) cfg.markReadAfterAction = root["markReadAfterAction"].toBool();

	// Profile & UI
	if (root.contains("showPeerId")) cfg.showPeerId = root["showPeerId"].toBool();

	// Confirmations
	if (root.contains("stickerConfirmation")) cfg.stickerConfirmation = root["stickerConfirmation"].toBool();
	if (root.contains("voiceConfirmation")) cfg.voiceConfirmation = root["voiceConfirmation"].toBool();
	if (root.contains("roundConfirmation")) cfg.roundConfirmation = root["roundConfirmation"].toBool();

	// Link Guard
	if (root.contains("linkGuardCheckEnabled")) cfg.linkGuardCheckEnabled = root["linkGuardCheckEnabled"].toBool();
	if (root.contains("linkGuardShowMode")) cfg.linkGuardShowMode = root["linkGuardShowMode"].toInt();
	if (root.contains("linkGuardExpandShort")) cfg.linkGuardExpandShort = root["linkGuardExpandShort"].toBool();
	if (root.contains("linkGuardCheckAge")) cfg.linkGuardCheckAge = root["linkGuardCheckAge"].toBool();
	if (root.contains("linkGuardCleanOnOpen")) cfg.linkGuardCleanOnOpen = root["linkGuardCleanOnOpen"].toBool();
	if (root.contains("linkGuardCleanOutgoing")) cfg.linkGuardCleanOutgoing = root["linkGuardCleanOutgoing"].toBool();
	if (root.contains("linkGuardAggressive")) cfg.linkGuardAggressive = root["linkGuardAggressive"].toBool();
	if (root.contains("linkGuardUseDatabase")) cfg.linkGuardUseDatabase = root["linkGuardUseDatabase"].toBool();
	if (root.contains("linkGuardWhitelist")) {
		cfg.linkGuardWhitelist.clear();
		for (const auto &val : root["linkGuardWhitelist"].toArray()) {
			cfg.linkGuardWhitelist.push_back(val.toString());
		}
	}
	if (root.contains("linkGuardStatsCleaned")) cfg.linkGuardStatsCleaned = static_cast<int64>(root["linkGuardStatsCleaned"].toDouble());
	if (root.contains("linkGuardStatsWarned")) cfg.linkGuardStatsWarned = static_cast<int64>(root["linkGuardStatsWarned"].toDouble());
	if (root.contains("linkGuardDbBuilt")) cfg.linkGuardDbBuilt = root["linkGuardDbBuilt"].toString();
	if (root.contains("linkGuardDbEntries")) cfg.linkGuardDbEntries = root["linkGuardDbEntries"].toInt();
	if (root.contains("linkGuardDbCheckedAt")) cfg.linkGuardDbCheckedAt = static_cast<uint64>(root["linkGuardDbCheckedAt"].toDouble());

	if (root.contains("m3SeedColor")) cfg.m3SeedColor = static_cast<uint32>(root["m3SeedColor"].toDouble());
	if (root.contains("m3SchemeVariant")) cfg.m3SchemeVariant = root["m3SchemeVariant"].toInt();
	if (root.contains("m3ContrastLevel")) cfg.m3ContrastLevel = root["m3ContrastLevel"].toDouble();

	if (root.contains("wsProxyEnabled")) cfg.wsProxyEnabled = root["wsProxyEnabled"].toBool();
	if (root.contains("wsProxyDisableTls")) cfg.wsProxyDisableTls = root["wsProxyDisableTls"].toBool();
	if (root.contains("wsProxyPort")) {
		const auto p = root["wsProxyPort"].toInt();
		cfg.wsProxyPort = (p == 443 || p <= 0) ? 1443 : p;
	}
	if (root.contains("wsProxySecret")) {
		cfg.wsProxySecret = root["wsProxySecret"].toString().trimmed().toLower();
	}
	const auto isBadSecret = (cfg.wsProxySecret.size() != 32)
		|| (cfg.wsProxySecret == u"00000000000000000000000000000001"_q)
		|| (cfg.wsProxySecret == u"00000000000000000000000000000000"_q);
	if (isBadSecret) {
		cfg.wsProxySecret = GenerateRandomSecret();
		SaveConfig();
	}

	if (!cfg.monospaceFont.isEmpty()) {
		style::SetCustomMonospaceFont(cfg.monospaceFont);
	}
}

void CheckGoOffline(not_null<Main::Session*> session) {
	if (!CurrentConfig().goOfflineAutomatically) {
		return;
	}
	session->api().request(MTPaccount_UpdateStatus(
		MTP_bool(true) // offline = true
	)).send();
}

bool MaybeShowStoryGhostAlert(
		not_null<Window::SessionController*> controller,
		Fn<void()> callback) {
	if (!CurrentConfig().storyGhostAlert || CurrentConfig().dontReadStories) {
		return false;
	}
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(400);
		box->setTitle(rpl::single(QString::fromUtf8("Предупреждение")));
		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			rpl::single(QString::fromUtf8(
				"Внимание: Невидимка для историй сейчас выключена!\n\n"
				"Автор увидит ваш просмотр. Включить невидимку перед открытием?")),
			st::boxLabel),
			st::boxRowPadding);

		box->addButton(rpl::single(QString::fromUtf8("Включить невидимку")), [=] {
			CurrentConfig().dontReadStories = true;
			SaveConfig();
			box->closeBox();
			callback();
		});

		box->addButton(rpl::single(QString::fromUtf8("Открыть")), [=] {
			box->closeBox();
			callback();
		});
	}));
	return true;
}

void ShowSettingsBox(Window::SessionController *controller) {
	if (!controller) {
		return;
	}

	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(460);
		box->setTitle(rpl::single(QString::fromUtf8("Настройки PixelGram")));

		// Pill segmented tabs at the top
		const auto tabsBar = box->addRow(
			object_ptr<M3SegmentedTabs>(
				box,
				std::vector<QString>{
					QString::fromUtf8("Невидимка"),
					QString::fromUtf8("Внешний вид"),
					QString::fromUtf8("Чаты"),
					QString::fromUtf8("Другое"),
				}),
			style::margins(0, 4, 0, 8));

		// Single content container
		const auto container = box->addRow(object_ptr<Ui::VerticalLayout>(box));

		const auto addToggle = [=](
				not_null<Ui::VerticalLayout*> parent,
				const QString &text,
				const QString &subtext,
				bool initial,
				Fn<void(bool)> onChange) {
			const auto wrap = parent->add(
				object_ptr<Ui::VerticalLayout>(parent),
				style::margins(14, 6, 14, 6));

			const auto checkbox = wrap->add(object_ptr<Ui::Checkbox>(
				wrap,
				text,
				initial,
				st::defaultBoxCheckbox));

			if (!subtext.isEmpty()) {
				const auto label = wrap->add(
					object_ptr<Ui::FlatLabel>(wrap, rpl::single(subtext), st::boxDividerLabel),
					style::margins(32, 2, 14, 2));
				label->setAttribute(Qt::WA_TransparentForMouseEvents);
			}

			checkbox->checkedChanges() | rpl::on_next([=](bool checked) {
				onChange(checked);
				SaveConfig();
			}, box->lifetime());

			return checkbox;
		};

		const auto buildTab = [=](int tabIndex) {
			container->clear();

			if (tabIndex == 0) {
				// ------------------ TAB 0: GHOST MODE ------------------
				const auto card1 = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 4, 16, 4));

				card1->add(object_ptr<Ui::FlatLabel>(
					card1,
					rpl::single(QString::fromUtf8("Режим невидимки")),
					st::boxTitle),
					style::margins(14, 10, 14, 4));

				addToggle(
					card1,
					QString::fromUtf8("Режим невидимки (Master)"),
					QString::fromUtf8("Включает или выключает все функции невидимки сразу."),
					CurrentConfig().ghostMode,
					[=](bool checked) {
						CurrentConfig().setMasterGhostMode(checked);
					});

				addToggle(
					card1,
					QString::fromUtf8("Не читать входящие сообщения"),
					QString::fromUtf8("Собеседник не увидит вторую галочку прочтения."),
					CurrentConfig().dontReadMessages,
					[](bool c) { CurrentConfig().dontReadMessages = c; });

				addToggle(
					card1,
					QString::fromUtf8("Не читать истории (Stories)"),
					QString::fromUtf8("Ваш просмотр не отобразится в списке зрителей."),
					CurrentConfig().dontReadStories,
					[](bool c) { CurrentConfig().dontReadStories = c; });

				addToggle(
					card1,
					QString::fromUtf8("Предупреждение перед просмотром истории"),
					QString::fromUtf8("Спросить подтверждение, если невидимка историй выключена."),
					CurrentConfig().storyGhostAlert,
					[](bool c) { CurrentConfig().storyGhostAlert = c; });

				const auto card2 = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 8, 16, 4));

				card2->add(object_ptr<Ui::FlatLabel>(
					card2,
					rpl::single(QString::fromUtf8("Статус и присутствие")),
					st::boxTitle),
					style::margins(14, 10, 14, 4));

				addToggle(
					card2,
					QString::fromUtf8("Скрывать статус 'Печатает...'"),
					QString::fromUtf8("Не отправлять события набора текста и записи голосовых."),
					CurrentConfig().dontSendTyping,
					[](bool c) { CurrentConfig().dontSendTyping = CurrentConfig().hideTyping = c; });

				addToggle(
					card2,
					QString::fromUtf8("Не отправлять статус 'В сети'"),
					QString::fromUtf8("Ваш онлайн-статус не обновляется при открытии клиента."),
					CurrentConfig().dontSendOnline,
					[](bool c) { CurrentConfig().dontSendOnline = c; });

				addToggle(
					card2,
					QString::fromUtf8("Скрывать 'В сети' сразу после отправки"),
					QString::fromUtf8("Автоматически сбрасывать статус в оффлайн после сообщения."),
					CurrentConfig().goOfflineAutomatically,
					[](bool c) { CurrentConfig().goOfflineAutomatically = c; });

			} else if (tabIndex == 1) {
				// ------------------ TAB 1: APPEARANCE ------------------
				const auto cardShapes = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 4, 16, 4));

				const auto avatarPreview = cardShapes->add(object_ptr<AvatarPreviewWidget>(cardShapes));

				cardShapes->add(object_ptr<Ui::FlatLabel>(
					cardShapes,
					rpl::single(QString::fromUtf8("Форма аватарок")),
					st::boxDividerLabel),
					style::margins(14, 6, 14, 4));

				const auto shapeGroup = std::make_shared<Ui::RadioenumGroup<AvatarShape>>(CurrentConfig().avatarShape);
				const auto addRadio = [&](AvatarShape val, const QString &label) {
					cardShapes->add(
						object_ptr<Ui::Radioenum<AvatarShape>>(cardShapes, shapeGroup, val, label, st::defaultBoxCheckbox),
						style::margins(14, 4, 14, 4));
				};
				addRadio(AvatarShape::Circle, QString::fromUtf8("Круг (Circle)"));
				addRadio(AvatarShape::Squircle, QString::fromUtf8("Сквиркл (Squircle)"));
				addRadio(AvatarShape::Cookie, QString::fromUtf8("Печенька (Cookie M3)"));
				addRadio(AvatarShape::Sunflower, QString::fromUtf8("Цветок (Sunflower M3)"));
				addRadio(AvatarShape::Clover, QString::fromUtf8("Клевер (Clover M3)"));
				addRadio(AvatarShape::Hexagon, QString::fromUtf8("Шестиугольник (Hexagon M3)"));
				addRadio(AvatarShape::Custom, QString::fromUtf8("Свой радиус (Кастом)"));

				cardShapes->add(object_ptr<Ui::FlatLabel>(
					cardShapes,
					rpl::single(QString::fromUtf8("Радиус углов (только для своего радиуса)")),
					st::boxDividerLabel),
					style::margins(14, 8, 14, 4));

				const auto radiusSlider = cardShapes->add(
					object_ptr<Ui::SettingsSlider>(cardShapes, st::settingsSlider),
					style::margins(14, 4, 14, 4));
				radiusSlider->addSection("10%");
				radiusSlider->addSection("20%");
				radiusSlider->addSection("30%");
				radiusSlider->addSection("40%");
				radiusSlider->addSection("50%");
				const auto initialRadiusIndex = std::clamp((CurrentConfig().avatarCornerRadius - 10) / 10, 0, 4);
				radiusSlider->setActiveSectionFast(initialRadiusIndex);

				const auto singleCornerCheckbox = cardShapes->add(object_ptr<Ui::Checkbox>(
					cardShapes,
					QString::fromUtf8("Скруглять только один угол"),
					CurrentConfig().singleCornerRadius,
					st::defaultBoxCheckbox),
					style::margins(14, 6, 14, 10));

				const auto updateShapeControlsState = [=] {
					const auto isCustom = (CurrentConfig().avatarShape == AvatarShape::Custom);
					radiusSlider->setDisabled(!isCustom);
					singleCornerCheckbox->setDisabled(!isCustom);
				};
				updateShapeControlsState();

				shapeGroup->setChangedCallback([=](AvatarShape val) {
					CurrentConfig().avatarShape = val;
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

				// Card 2: Interface & Fonts
				const auto cardUi = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 8, 16, 4));

				cardUi->add(object_ptr<Ui::FlatLabel>(
					cardUi,
					rpl::single(QString::fromUtf8("Интерфейс и шрифты")),
					st::boxTitle),
					style::margins(14, 10, 14, 4));

				addToggle(
					cardUi,
					QString::fromUtf8("Отключить кастомные фоны"),
					QString::fromUtf8("Использовать стандартный фон текущей темы Material You."),
					CurrentConfig().disableCustomBackgrounds,
					[](bool c) { CurrentConfig().disableCustomBackgrounds = c; });

				addToggle(
					cardUi,
					QString::fromUtf8("Скрыть эмодзи-статусы Premium"),
					QString::fromUtf8("Не отображать эмодзи-статусы рядом с именами пользователей."),
					CurrentConfig().hidePremiumStatuses,
					[](bool c) { CurrentConfig().hidePremiumStatuses = c; });

				addToggle(
					cardUi,
					QString::fromUtf8("Моноширинный шрифт"),
					QString::fromUtf8("Применять Consolas для кода и блоков pre/code."),
					!CurrentConfig().monospaceFont.isEmpty(),
					[](bool c) {
						CurrentConfig().monospaceFont = c ? "Consolas" : "";
						style::SetCustomMonospaceFont(CurrentConfig().monospaceFont);
					});



				addToggle(
					cardUi,
					QString::fromUtf8("Скрыть вкладку 'Все чаты'"),
					QString::fromUtf8("Убрать первую вкладку всех диалогов, оставив только папки."),
					CurrentConfig().hideAllChatsTab,
					[](bool c) {
						CurrentConfig().hideAllChatsTab = c;
						SaveConfig();
						if (const auto session = Core::App().domain().active().maybeSession()) {
							session->data().chatsFilters().notifyChanged();
						}
					});

				// Card 3: Tray
				const auto cardTray = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 8, 16, 4));

				cardTray->add(object_ptr<Ui::FlatLabel>(
					cardTray,
					rpl::single(QString::fromUtf8("Меню в трее")),
					st::boxTitle),
					style::margins(14, 10, 14, 4));

				addToggle(
					cardTray,
					QString::fromUtf8("Переключатель Невидимки в трее"),
					"",
					CurrentConfig().trayGhostMode,
					[](bool c) {
						CurrentConfig().trayGhostMode = c;
						SaveConfig();
						Core::App().settings().setTrayIconMonochrome(c);
						Core::App().saveSettingsDelayed();
					});

				addToggle(
					cardTray,
					QString::fromUtf8("Режим стримера в трее"),
					QString::fromUtf8("Блюрит номера, юзернеймы и текст сообщений."),
					CurrentConfig().trayStreamerMode,
					[](bool c) { CurrentConfig().trayStreamerMode = c; });

			} else if (tabIndex == 2) {
				// ------------------ TAB 2: CHATS ------------------
				const auto cardBubble = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 4, 16, 4));

				const auto bubblePreview = cardBubble->add(object_ptr<BubblePreviewWidget>(cardBubble));

				cardBubble->add(object_ptr<Ui::FlatLabel>(
					cardBubble,
					rpl::single(QString::fromUtf8("Радиус пузырей сообщений")),
					st::boxDividerLabel),
					style::margins(14, 6, 14, 4));

				const auto bubbleRadiusSlider = cardBubble->add(
					object_ptr<Ui::SettingsSlider>(cardBubble, st::settingsSlider),
					style::margins(14, 4, 14, 4));
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

				addToggle(
					cardBubble,
					QString::fromUtf8("Убрать хвостик сообщений"),
					QString::fromUtf8("Делает пузыри сообщений идеально скруглёнными со всех сторон."),
					CurrentConfig().removeMessageTail,
					[=](bool c) {
						CurrentConfig().removeMessageTail = c;
						bubblePreview->update();
					});

				// Card 2: Wide & Display
				const auto cardDisplay = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 8, 16, 4));

				cardDisplay->add(object_ptr<Ui::FlatLabel>(
					cardDisplay,
					rpl::single(QString::fromUtf8("Ширина сообщений")),
					st::boxDividerLabel),
					style::margins(14, 10, 14, 4));



				addToggle(
					cardDisplay,
					QString::fromUtf8("Полупрозрачность удалённых сообщений"),
					QString::fromUtf8("Сообщения, сохранённые Anti-Delete, отображаются с прозрачностью ~65%."),
					CurrentConfig().translucentDeletedMessages,
					[](bool c) { CurrentConfig().translucentDeletedMessages = c; });

				addToggle(
					cardDisplay,
					QString::fromUtf8("Скрыть боковую стрелку пересылки"),
					QString::fromUtf8("Не отображать круглую кнопку быстрой пересылки справа от постов."),
					CurrentConfig().hideSideShareButton,
					[](bool c) { CurrentConfig().hideSideShareButton = c; });

				addToggle(
					cardDisplay,
					QString::fromUtf8("Отключить цветные ответы"),
					QString::fromUtf8("Использовать единый тональный цвет темы M3 вместо цвета автора."),
					CurrentConfig().disableColorfulReplies,
					[](bool c) { CurrentConfig().disableColorfulReplies = c; });

				// Card 3: Context Menu
				const auto cardMenu = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 8, 16, 4));

				cardMenu->add(object_ptr<Ui::FlatLabel>(
					cardMenu,
					rpl::single(QString::fromUtf8("Контекстное меню сообщений")),
					st::boxTitle),
					style::margins(14, 10, 14, 4));

				const auto addContextItemFilter = [&](const QString &title, ContextMenuDisplay &val) {
					cardMenu->add(object_ptr<Ui::FlatLabel>(
						cardMenu,
						rpl::single(title),
						st::boxDividerLabel),
						style::margins(14, 6, 14, 2));
					const auto slider = cardMenu->add(
						object_ptr<Ui::SettingsSlider>(cardMenu, st::settingsSlider),
						style::margins(14, 2, 14, 6));
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
				addContextItemFilter(QString::fromUtf8("Сообщения автора"), CurrentConfig().menuUserMessages);
				addContextItemFilter(QString::fromUtf8("Повторить (Repeat)"), CurrentConfig().menuRepeat);
				addContextItemFilter(QString::fromUtf8("Скриншот (Shot)"), CurrentConfig().menuMessageShot);
				addContextItemFilter(QString::fromUtf8("Скрыть локально"), CurrentConfig().menuHideLocally);

			} else if (tabIndex == 3) {
				// ------------------ TAB 3: OTHER ------------------
				const auto cardOther = container->add(
					object_ptr<CardWidget>(container),
					style::margins(16, 4, 16, 4));

				cardOther->add(object_ptr<Ui::FlatLabel>(
					cardOther,
					rpl::single(QString::fromUtf8("Дополнительные возможности")),
					st::boxTitle),
					style::margins(14, 10, 14, 4));

				addToggle(
					cardOther,
					QString::fromUtf8("Локальный Telegram Premium"),
					QString::fromUtf8("Разблокирует эмодзи-статусы и локальные возможности Premium."),
					CurrentConfig().localPremium,
					[](bool c) { CurrentConfig().localPremium = c; });

				addToggle(
					cardOther,
					QString::fromUtf8("Отключить рекламу (Disable Ads)"),
					QString::fromUtf8("Скрывает рекламные и спонсорские посты в каналах."),
					CurrentConfig().disableAds,
					[](bool c) { CurrentConfig().disableAds = c; });

				addToggle(
					cardOther,
					QString::fromUtf8("Анти-удаление сообщений (Anti-Delete)"),
					QString::fromUtf8("Сохраняет удалённые собеседником сообщения в чате."),
					CurrentConfig().antiDelete,
					[](bool c) { CurrentConfig().antiDelete = c; });

				addToggle(
					cardOther,
					QString::fromUtf8("Обход защиты от копирования"),
					QString::fromUtf8("Разрешает скриншоты, копирование и сохранение медиа."),
					CurrentConfig().bypassProtection,
					[](bool c) { CurrentConfig().bypassProtection = c; });

				addToggle(
					cardOther,
					QString::fromUtf8("Блюр фона в медиа"),
					QString::fromUtf8("Красивое размытие чата на заднем плане при просмотре фото/видео."),
					CurrentConfig().blurMediaBackground,
					[](bool c) { CurrentConfig().blurMediaBackground = c; });
			}

			container->resizeToWidth(460);
			box->scrollToY(0);
		};

		tabsBar->setChangedCallback([=](int index) {
			buildTab(index);
		});

		// Build initial tab (Ghost Mode)
		buildTab(0);

		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

static bool g_deletedMessagesHidden = false;

bool AreDeletedMessagesHidden() {
	return g_deletedMessagesHidden;
}

void SetDeletedMessagesHidden(bool hidden) {
	g_deletedMessagesHidden = hidden;
}

void ToggleDeletedMessagesHidden(Window::SessionController *controller) {
	SetDeletedMessagesHidden(!AreDeletedMessagesHidden());
	if (controller) {
		auto &owner = controller->session().data();
		const auto history = controller->activeChatCurrent().history();
		const auto migrated = history ? history->migrateFrom() : nullptr;
		for (const auto h : { history, migrated }) {
			if (!h) continue;
			for (const auto &block : h->blocks) {
				for (const auto &view : block->messages) {
					if (view->data()->isDeletedByAntiDelete()) {
						owner.requestViewResize(view.get());
					}
				}
			}
			owner.notifyHistoryChangeDelayed(h);
		}
		owner.sendHistoryChangeNotifications();
		controller->widget()->update();
	}
}

} // namespace PixelGram
