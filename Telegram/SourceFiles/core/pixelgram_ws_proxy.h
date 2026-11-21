/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <rpl/producer.h>
#include <QtCore/QString>

namespace PixelGram {
namespace WsProxy {

void Start();
void Stop();
void Restart();
[[nodiscard]] bool IsRunning();
[[nodiscard]] int ActivePort();
[[nodiscard]] QString FullSecret();
[[nodiscard]] QString ProxyLink();
void ApplyProxyToTelegram();
[[nodiscard]] QString StatusText();
[[nodiscard]] rpl::producer<QString> StatusValue();

} // namespace WsProxy
} // namespace PixelGram
