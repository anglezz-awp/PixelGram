# <p align="center"><img src="Telegram/Resources/art/icon256.png" width="96" height="96" alt="PixelGram Logo"><br>PixelGram Desktop</p>

<p align="center">
  <b>A modern, beautiful, and privacy-focused Telegram Desktop client powered by Google Material You (Material 3 Expressive).</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPLv3">
  <img src="https://img.shields.io/badge/Style-Material%203%20Expressive-purple.svg" alt="Material 3">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-informational.svg" alt="Platforms">
  <img src="https://img.shields.io/badge/C%2B%2B-20-brightgreen.svg" alt="C++20">
</p>

---

## ✨ Features Overview

### 🎨 Google Material You & Material 3 Expressive Design
* **Dynamic M3 Algorithmic Color Schemes**: Fully integrated Google [`material-color-utilities`](https://github.com/material-foundation/material-color-utilities) generating expressive tonal palettes (`surfaceContainer`, `onSurface`, `primaryContainer`, `secondaryContainer`, `outlineVariant`) dynamically from your chosen seed accent color.
* **M3 Expressive Avatar Shapes**: Choose between standard Circle, Squircle (Superellipse), 8-petal Scalloped Cookie, 12-petal Sunflower, 4-leaf Clover, and Rounded Hexagon.
* **Modern M3 Capsules & Pills**: Redesigned chat compose bar, interactive profile status pills, and fluid adaptive geometry.
* **Google Material Symbols Rounded**: Official clean vector icons (`google/material-design-icons`) for menus, settings, and navigation.

### 👻 Advanced Stealth & Ghost Mode
* **Typing & Activity Suppression**: Completely silences outgoing typing, audio/video recording, choosing sticker, and file/photo upload progress packets.
* **Stealth Stories Viewing**: Watch stories without sending read receipts or bumping viewer counters.
* **Mark Read On Action**: Chats remain unread while you read and browse them, and automatically mark as read only when you send a message, reaction, or vote.
* **Quick Drawer Toggle**: Fast one-click Ghost Mode switch right in the main side menu.

### 🛡️ LinkGuard Protection System
* **Anti-Tracking & Cleaner**: Automatically strips tracking parameters (`utm_*`, `fbclid`, `gclid`, `yclid`, referral tags) before opening links.
* **Short Link Expansion**: Resolves shorteners (`t.co`, `bit.ly`, `tinyurl`, etc.) safely to show the final destination.
* **Domain Age & Phishing Detection**: Warns against suspicious, newly-registered, or punycode phishing links.

### 🌐 Built-in MTProto WebSocket Proxy (WS-Proxy)
* Local embedded WebSocket proxy server (`pixelgram_ws_proxy`), powered by the protocol and architecture of **[tg-ws-proxy](https://github.com/Flowseal/tg-ws-proxy)** by **@Flowseal**, that encapsulates native Telegram MTProto traffic into secure WebSocket streams to bypass DPI, ISP filtering, and restrictive firewalls.
* Configurable port, secret key, and automatic fallback.

### 📱 Mini-Apps (Webview) Supercharger
* **Android Platform Spoofing**: Spoofs the platform identifier as `android` across all Bot Mini-App RPCs (`RequestAppWebView`, `RequestMainWebView`, etc.), allowing you to access mobile-exclusive games and webapps on desktop.
* **Enlarged Canvas**: Expanded modal window dimensions (up to 512×782px) for comfortable bot interaction without horizontal scrolling.

### 🛠️ Chat Productivity & Utility
* **Peer ID Display**: Displays formatted Telegram Bot API Peer IDs in profile cards with one-click clipboard copying.
* **Compose Bar Cleanup**: Toggles to hide the Gift button or Voice/Video record button.
* **Accidental Send Prevention**: Confirmation dialogs for stickers, voice notes, and round video messages.
* **Message Shot**: Generates high-resolution quote cards with gradient M3 container backgrounds and crisp borders, ready to paste.
* **Message Revision History**: Local audit log of incoming message revisions (`PixelGram::Revisions`), showing past versions, timestamps, and edit history.
* **Intelligent Forwarding**: Seamless re-sending of protected/restricted content without forward restrictions.

---

## 🛠️ Building from Source

### Prerequisites (Windows)
1. **Visual Studio 2022** (v143 toolset with C++ Desktop development).
2. **CMake 3.25+** and **Git**.
3. **Qt 6.11+** statically built with bundled libraries.
4. Windows 10/11 SDK (10.0.22621.0 or newer).

### Build Command
```powershell
# Generate CMake build files
cmake -B out -S . -DCMAKE_BUILD_TYPE=Release

# Compile Telegram executable
cmake --build out --target Telegram --config Release -- /p:PreferredToolArchitecture=x64 /m:12
```

---

## ❤️ Credits & Acknowledgements

PixelGram is made possible thanks to the following open-source projects:

* **[Telegram Desktop](https://github.com/telegramdesktop/tdesktop)** — The official desktop client for Telegram (GPLv3).
* **[AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop)** by **@Radolyn** — Research, forward mechanics, and ghost mode inspirations.
* **[tg-ws-proxy](https://github.com/Flowseal/tg-ws-proxy)** by **@Flowseal** — The embedded MTProto WebSocket proxy protocol & architecture.
* **[Material Foundation](https://github.com/material-foundation/material-color-utilities)** — Google Material 3 color system algorithms.
* **[Google Material Design Icons](https://github.com/google/material-design-icons)** — Google Material Symbols Rounded vector icons.

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)** with OpenSSL exception. See [LICENSE](LICENSE) for details.
