with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/themes/window_theme.h", "r", encoding="utf-8") as f:
    h = f.read()

target_h = "void reset();"
replacement_h = """void reset();
	void notifyBackgroundChanged();"""

if target_h in h and "notifyBackgroundChanged" not in h:
    h = h.replace(target_h, replacement_h, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/themes/window_theme.h", "w", encoding="utf-8") as f:
        f.write(h)
    print("Updated window_theme.h successfully!")
else:
    print("window_theme.h already updated or target not found")

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/themes/window_theme.cpp", "r", encoding="utf-8") as f:
    c = f.read()

# 1. setThemeData
old_theme_data = """void ChatBackground::setThemeData(QImage &&themeImage, bool themeTile) {
	_themeImage = PostprocessBackgroundImage(
		std::move(themeImage),
		Data::ThemeWallPaper());
	_themeTile = themeTile;
}"""

new_theme_data = """void ChatBackground::setThemeData(QImage &&themeImage, bool themeTile) {
	_themeImage = QImage();
	_themeTile = false;
}

void ChatBackground::notifyBackgroundChanged() {
	_updates.fire({ BackgroundUpdate::Type::New, tile() });
}"""

if old_theme_data in c:
    c = c.replace(old_theme_data, new_theme_data, 1)

# 2. colorForFill
old_color_fill = """std::optional<QColor> ChatBackground::colorForFill() const {
	return !_prepared.isNull()
		? imageMonoColor()
		: (!_gradient.isNull() || _paper.backgroundColors().empty())
		? std::nullopt
		: std::make_optional(_paper.backgroundColors().front());
}"""

new_color_fill = """std::optional<QColor> ChatBackground::colorForFill() const {
	if (!Data::IsCustomWallPaper(_paper) && !Data::IsCloudWallPaper(_paper)) {
		return st::windowBg->c;
	}
	return !_prepared.isNull()
		? imageMonoColor()
		: (!_gradient.isNull() || _paper.backgroundColors().empty())
		? std::nullopt
		: std::make_optional(_paper.backgroundColors().front());
}"""

if old_color_fill in c:
    c = c.replace(old_color_fill, new_color_fill, 1)

# 3. setPreparedAfterPaper
old_set_prepared = """void ChatBackground::setPreparedAfterPaper(QImage image) {
	const auto &bgColors = _paper.backgroundColors();"""

new_set_prepared = """void ChatBackground::setPreparedAfterPaper(QImage image) {
	if (!Data::IsCustomWallPaper(_paper) && !Data::IsCloudWallPaper(_paper)) {
		setPrepared(QImage(), QImage(), QImage());
		return;
	}
	const auto &bgColors = _paper.backgroundColors();"""

if old_set_prepared in c:
    c = c.replace(old_set_prepared, new_set_prepared, 1)

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/themes/window_theme.cpp", "w", encoding="utf-8") as f:
    f.write(c)
print("Updated window_theme.cpp successfully!")

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/window_session_controller.cpp", "r", encoding="utf-8") as f:
    sc = f.read()

old_pal_changed = """	style::PaletteChanged(
	) | rpl::on_next([=] {
		for (auto &[key, value] : _customChatThemes) {
			if (!key.theme.id) {
				value.theme.reset();
			}
		}
	}, _lifetime);"""

new_pal_changed = """	style::PaletteChanged(
	) | rpl::on_next([=] {
		pushDefaultChatBackground();
		for (auto &[key, value] : _customChatThemes) {
			if (!key.theme.id) {
				value.theme.reset();
			}
		}
		if (widget()) {
			widget()->update();
		}
	}, _lifetime);"""

if old_pal_changed in sc:
    sc = sc.replace(old_pal_changed, new_pal_changed, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/window_session_controller.cpp", "w", encoding="utf-8") as f:
        f.write(sc)
    print("Updated window_session_controller.cpp successfully!")
else:
    print("old_pal_changed not found in window_session_controller.cpp!")
