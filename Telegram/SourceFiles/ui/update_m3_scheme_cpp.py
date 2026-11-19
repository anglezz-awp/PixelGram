with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/pixelgram_m3_scheme.cpp", "r", encoding="utf-8") as f:
    c = f.read()

inc_target = '#include "ui/pixelgram_m3_scheme.h"'
inc_rep = '''#include "ui/pixelgram_m3_scheme.h"
#include "window/themes/window_theme.h"'''

if inc_target in c and "window/themes/window_theme.h" not in c:
    c = c.replace(inc_target, inc_rep, 1)

notify_target = """	style::internal::ResetIcons();
	style::NotifyPaletteChanged();"""

notify_rep = """	style::internal::ResetIcons();
	style::NotifyPaletteChanged();
	if (const auto bg = Window::Theme::Background()) {
		bg->notifyBackgroundChanged();
	}"""

if notify_target in c:
    c = c.replace(notify_target, notify_rep, 1)

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/pixelgram_m3_scheme.cpp", "w", encoding="utf-8") as f:
    f.write(c)

print("Updated pixelgram_m3_scheme.cpp successfully!")
