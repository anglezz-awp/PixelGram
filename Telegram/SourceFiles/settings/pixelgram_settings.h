#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>

namespace Window {
class SessionController;
} // namespace Window

namespace PixelGram {

struct Config {
	bool antiDelete = true;
	bool ghostMode = true;
	bool hideTyping = true;
	bool bypassProtection = true;
	bool blurMediaBackground = true;
	uint32 m3SeedColor = 0xFF917BBD; // Default purple seed ARGB
	int m3SchemeVariant = 0; // 0 = TonalSpot, 1 = Vibrant, 2 = Expressive, 3 = Neutral, 4 = Monochrome
	double m3ContrastLevel = 0.0;
};

[[nodiscard]] Config &CurrentConfig();
void SaveConfig();
void LoadConfig();

void ShowSettingsBox(Window::SessionController *controller);

} // namespace PixelGram
