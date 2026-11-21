/*
This file is part of PixelGram Desktop,
an enhanced Telegram Desktop modification with Material Design 3.
*/
#pragma once

namespace PixelGram {

enum class UiSound {
	ChatOpen,
	RecordStart,
	RecordStop,
	MessageSent,
};

void PlayUiSound(UiSound sound);

} // namespace PixelGram
