/*
This file is part of PixelGram Desktop,
an enhanced Telegram Desktop modification with Material Design 3.
*/
#include "ui/pixelgram_ui_feedback.h"
#include "settings/pixelgram_settings.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

#include <vector>
#include <cmath>
#include <cstdint>

namespace PixelGram {
namespace {

#ifdef Q_OS_WIN

#ifndef M_PI
constexpr auto M_PI = 3.14159265358979323846;
#endif

// Synthesize a clean 44.1kHz 16-bit mono PCM WAV in memory
std::vector<uint8_t> GenerateWav(
		double freqStart,
		double freqEnd,
		int durationMs,
		double harmonics = 0.25) {
	constexpr int sampleRate = 44100;
	const int numSamples = sampleRate * durationMs / 1000;
	std::vector<int16_t> samples;
	samples.reserve(numSamples);

	double phase = 0.0;
	for (int i = 0; i < numSamples; ++i) {
		const double t = static_cast<double>(i) / numSamples;
		const double freq = freqStart + (freqEnd - freqStart) * std::sqrt(t);
		phase += 2.0 * M_PI * freq / sampleRate;

		// Fast attack (12%), smooth exponential decay
		double env = 0.0;
		if (t < 0.12) {
			env = t / 0.12;
		} else {
			env = std::exp(-4.2 * (t - 0.12));
		}

		const double val = (std::sin(phase) + harmonics * std::sin(phase * 2.0)) * 0.75 * env;
		const int sampleInt = static_cast<int>(val * 28000.0);
		samples.push_back(static_cast<int16_t>(std::clamp(sampleInt, -32767, 32767)));
	}

	const uint32_t dataBytes = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
	const uint32_t riffBytes = 36 + dataBytes;

	std::vector<uint8_t> wav;
	wav.reserve(44 + dataBytes);

	auto writeU32 = [&](uint32_t val) {
		wav.push_back(val & 0xFF);
		wav.push_back((val >> 8) & 0xFF);
		wav.push_back((val >> 16) & 0xFF);
		wav.push_back((val >> 24) & 0xFF);
	};
	auto writeU16 = [&](uint16_t val) {
		wav.push_back(val & 0xFF);
		wav.push_back((val >> 8) & 0xFF);
	};

	// RIFF header
	wav.push_back('R'); wav.push_back('I'); wav.push_back('F'); wav.push_back('F');
	writeU32(riffBytes);
	wav.push_back('W'); wav.push_back('A'); wav.push_back('V'); wav.push_back('E');

	// fmt chunk
	wav.push_back('f'); wav.push_back('m'); wav.push_back('t'); wav.push_back(' ');
	writeU32(16); // chunk size
	writeU16(1);  // PCM format
	writeU16(1);  // mono
	writeU32(sampleRate);
	writeU32(sampleRate * 2); // byte rate
	writeU16(2);  // block align
	writeU16(16); // bits per sample

	// data chunk
	wav.push_back('d'); wav.push_back('a'); wav.push_back('t'); wav.push_back('a');
	writeU32(dataBytes);

	const uint8_t *sampleBytes = reinterpret_cast<const uint8_t*>(samples.data());
	wav.insert(wav.end(), sampleBytes, sampleBytes + dataBytes);

	return wav;
}

const std::vector<uint8_t> &GetWavBuffer(UiSound sound) {
	static const auto chatOpenWav = GenerateWav(680.0, 1150.0, 42, 0.20);
	static const auto recordStartWav = GenerateWav(450.0, 750.0, 52, 0.30);
	static const auto recordStopWav = GenerateWav(620.0, 380.0, 35, 0.15);
	static const auto messageSentWav = GenerateWav(820.0, 1350.0, 48, 0.25);

	switch (sound) {
	case UiSound::ChatOpen: return chatOpenWav;
	case UiSound::RecordStart: return recordStartWav;
	case UiSound::RecordStop: return recordStopWav;
	case UiSound::MessageSent: return messageSentWav;
	}
	return chatOpenWav;
}

#endif

} // namespace

void PlayUiSound(UiSound sound) {
#ifdef Q_OS_WIN
	if (!CurrentConfig().uiSounds) {
		return;
	}
	const auto &wav = GetWavBuffer(sound);
	if (!wav.empty()) {
		PlaySoundA(
			reinterpret_cast<LPCSTR>(wav.data()),
			NULL,
			SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
	}
#endif
}

} // namespace PixelGram
