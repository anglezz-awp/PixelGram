with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/section_widget.cpp", "r", encoding="utf-8") as f:
    c = f.read()

inc_target = '#include "window/section_widget.h"'
inc_replacement = '''#include "window/section_widget.h"

#include "window/themes/window_theme.h"
#include "data/data_wall_paper.h"
#include "styles/style_window.h"'''

c = c.replace(inc_target, inc_replacement, 1)

paint_target = """void SectionWidget::PaintBackground(
		QPainter &p,
		not_null<Ui::ChatTheme*> theme,
		QSize fill,
		QRect clip,
		bool paused) {
	const auto &background = theme->background();
	if (background.colorForFill) {
		p.fillRect(clip, *background.colorForFill);
		return;
	}"""

paint_replacement = """void SectionWidget::PaintBackground(
		QPainter &p,
		not_null<Ui::ChatTheme*> theme,
		QSize fill,
		QRect clip,
		bool paused) {
	const auto bgTheme = Window::Theme::Background();
	const auto isCustomOrCloud = bgTheme
		&& (Data::IsCustomWallPaper(bgTheme->paper()) || Data::IsCloudWallPaper(bgTheme->paper()));
	if (!isCustomOrCloud) {
		p.fillRect(clip, st::windowBg->c);
		return;
	}
	const auto &background = theme->background();
	if (background.colorForFill) {
		p.fillRect(clip, *background.colorForFill);
		return;
	}"""

if paint_target in c:
    c = c.replace(paint_target, paint_replacement, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/section_widget.cpp", "w", encoding="utf-8") as f:
        f.write(c)
    print("Updated section_widget.cpp successfully!")
else:
    print("paint_target not found!")
