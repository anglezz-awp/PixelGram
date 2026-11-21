#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include <base/basic_types.h>

namespace Window {
class SessionController;
} // namespace Window

namespace Main {
class Session;
} // namespace Main

namespace PixelGram {

enum class AvatarShape : int {
	Custom = 0,    // Base rounded rect (with corner radius slider)
	Circle = 1,    // Perfect circle
	Squircle = 2,  // Superellipse
	Cookie = 3,    // 8-petal scalloped cookie (M3 Expressive)
	Sunflower = 4, // 12-petal flower (M3)
	Clover = 5,    // 4-leaf clover (M3)
	Hexagon = 6,   // Rounded hexagon (M3 Puffy)
};

enum class ContextMenuDisplay : int {
	Shown = 0,
	Extended = 1,
	Hidden = 2,
};

enum class ChannelBottomAction : int {
	Discuss = 0,
	Mute = 1,
	Hide = 2,
};

enum class AntiDeleteMark : int {
	Text = 0,
	Emoji = 1,
	Hidden = 2,
};

struct Config {
	// Base
	bool antiDelete = true;
	AntiDeleteMark antiDeleteMark = AntiDeleteMark::Text;
	bool bypassProtection = true;
	bool blurMediaBackground = true;

	// Ghost Mode (Block 1 & 2)
	bool ghostMode = true; // master toggle compatibility
	bool dontReadMessages = true;
	bool dontReadStories = true;
	bool dontSendOnline = true;
	bool dontSendTyping = true;
	bool hideTyping = true; // compatibility
	bool goOfflineAutomatically = true;
	bool storyGhostAlert = true;

	// Other (Block 3)
	bool localPremium = false;
	bool disableAds = true;
	int localColorIndex = -1;
	uint64 localBackgroundEmojiId = 0;
	int localProfileColorIndex = -1;
	uint64 localProfileBackgroundEmojiId = 0;
	uint64 localEmojiStatusId = 0;

	// Appearance & M3 Avatar Shapes (Block 4)
	AvatarShape avatarShape = AvatarShape::Circle;
	int avatarCornerRadius = 50; // 0..50%
	bool singleCornerRadius = false;
	bool disableCustomBackgrounds = false;
	bool hidePremiumStatuses = false;
	QString monospaceFont;
	bool hideAllChatsTab = false;
	bool trayGhostMode = true;
	bool trayStreamerMode = false;
	bool streamerModeActive = false;
	bool drawerMyProfile = true;
	bool drawerNewGroup = true;
	bool drawerNewChannel = true;
	bool drawerContacts = true;
	bool drawerCalls = true;
	bool drawerSavedMessages = true;
	bool drawerNightMode = true;
	bool drawerGhostMode = true;
	bool drawerStreamerMode = true;

	// Chats & Messages (Block 5)
	bool showOnlyAddedStickers = false;
	bool unlimitedRecentStickers = true;
	bool hideReactionsBar = false;
	bool hideReactionsUnderMessage = false;
	bool hideReactionsAnimations = false;
	ChannelBottomAction channelBottomAction = ChannelBottomAction::Discuss;
	bool quickAdminShortcuts = false;
	bool disableGreetingSticker = true;
	ContextMenuDisplay menuMessageShot = ContextMenuDisplay::Shown;
	bool replaceWithIcons = false;
	bool removeMessageTail = true;
	bool hideSideShareButton = false;
	bool disableColorfulReplies = false;
	bool translucentDeletedMessages = true;
	int messageBubbleRadius = 16;
	ContextMenuDisplay menuReactions = ContextMenuDisplay::Shown;
	ContextMenuDisplay menuViews = ContextMenuDisplay::Shown;
	ContextMenuDisplay menuHideLocally = ContextMenuDisplay::Extended;
	ContextMenuDisplay menuUserMessages = ContextMenuDisplay::Shown;
	ContextMenuDisplay menuDetails = ContextMenuDisplay::Shown;
	ContextMenuDisplay menuRepeat = ContextMenuDisplay::Shown;
	bool showAttachButton = true;
	bool showCommandsButton = true;
	bool showTtlButton = true;
	bool showEmojiButton = true;
	bool showVoiceButton = true;
	bool showGiftButton = true;
	bool openAttachOnHover = false;
	bool openEmojiOnHover = false;

	// Webview Tweaks
	bool spoofWebviewAsAndroid = true;
	bool increaseWebviewHeight = true;
	bool increaseWebviewWidth = true;

	// Ghost Extensions
	bool markReadAfterAction = true;

	// Profile & UI
	bool showPeerId = true;

	// Confirmations
	bool stickerConfirmation = false;
	bool voiceConfirmation = false;
	bool roundConfirmation = false;

	// Link Guard
	bool linkGuardCheckEnabled = true;
	int linkGuardShowMode = 0; // 0 = Suspicious, 1 = Always
	bool linkGuardExpandShort = true;
	bool linkGuardCheckAge = false;
	bool linkGuardCleanOnOpen = true;
	bool linkGuardCleanOutgoing = false;
	bool linkGuardAggressive = false;
	bool linkGuardUseDatabase = true;
	std::vector<QString> linkGuardWhitelist;
	int64 linkGuardStatsCleaned = 0;
	int64 linkGuardStatsWarned = 0;
	QString linkGuardDbBuilt;
	int linkGuardDbEntries = 0;
	uint64 linkGuardDbCheckedAt = 0;

	// Material 3 Seed
	uint32 m3SeedColor = 0xFF917BBD;
	int m3SchemeVariant = 0;
	double m3ContrastLevel = 0.0;

	// WS-Proxy (Local MTProto -> WebSocket Proxy)
	bool wsProxyEnabled = true;
	int wsProxyPort = 1443;
	QString wsProxySecret;
	bool wsProxyDisableTls = true;

	[[nodiscard]] int activeGhostCount() const {
		return (dontReadMessages ? 1 : 0)
			+ (dontReadStories ? 1 : 0)
			+ (dontSendOnline ? 1 : 0)
			+ (dontSendTyping ? 1 : 0)
			+ (goOfflineAutomatically ? 1 : 0);
	}

	void setMasterGhostMode(bool enabled) {
		dontReadMessages = enabled;
		dontReadStories = enabled;
		dontSendOnline = enabled;
		dontSendTyping = enabled;
		goOfflineAutomatically = enabled;
		ghostMode = enabled;
		hideTyping = enabled;
	}
};

[[nodiscard]] Config &CurrentConfig();
inline Config &ModifyConfig() { return CurrentConfig(); }
void SaveConfig();
void LoadConfig();
[[nodiscard]] QString GenerateRandomSecret();

void CheckGoOffline(not_null<Main::Session*> session);
bool MaybeShowStoryGhostAlert(
	not_null<Window::SessionController*> controller,
	Fn<void()> callback);

[[nodiscard]] bool AreDeletedMessagesHidden();
void SetDeletedMessagesHidden(bool hidden);
void ToggleDeletedMessagesHidden(Window::SessionController *controller = nullptr);

void ShowSettingsBox(Window::SessionController *controller);

} // namespace PixelGram
