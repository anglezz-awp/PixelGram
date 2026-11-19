with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/section_widget.cpp", "r", encoding="utf-8") as f:
    c = f.read()

target = """void SectionWidget::PaintBackground(
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
	}"""

replacement = """void SectionWidget::PaintBackground(
		QPainter &p,
		not_null<Ui::ChatTheme*> theme,
		QSize fill,
		QRect clip,
		bool paused) {
	const auto bgTheme = Window::Theme::Background();
	const auto isUserChosenWallPaper = bgTheme
		&& (Data::IsCustomWallPaper(bgTheme->paper())
			|| (Data::IsCloudWallPaper(bgTheme->paper()) && !Data::IsDefaultWallPaper(bgTheme->paper())));
	if (!isUserChosenWallPaper) {
		p.fillRect(clip, st::windowBg->c);
		return;
	}"""

if target in c:
    c = c.replace(target, replacement, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/section_widget.cpp", "w", encoding="utf-8") as f:
        f.write(c)
    print("Updated section_widget.cpp!")
else:
    print("target not found in section_widget.cpp!")

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/themes/window_theme.cpp", "r", encoding="utf-8") as f:
    tc = f.read()

old_color_fill = """std::optional<QColor> ChatBackground::colorForFill() const {
	if (!Data::IsCustomWallPaper(_paper) && !Data::IsCloudWallPaper(_paper)) {
		return st::windowBg->c;
	}"""

new_color_fill = """std::optional<QColor> ChatBackground::colorForFill() const {
	const auto isUserChosenWallPaper = Data::IsCustomWallPaper(_paper)
		|| (Data::IsCloudWallPaper(_paper) && !Data::IsDefaultWallPaper(_paper));
	if (!isUserChosenWallPaper) {
		return st::windowBg->c;
	}"""

if old_color_fill in tc:
    tc = tc.replace(old_color_fill, new_color_fill, 1)

old_set_prepared = """void ChatBackground::setPreparedAfterPaper(QImage image) {
	if (!Data::IsCustomWallPaper(_paper) && !Data::IsCloudWallPaper(_paper)) {
		setPrepared(QImage(), QImage(), QImage());
		return;
	}"""

new_set_prepared = """void ChatBackground::setPreparedAfterPaper(QImage image) {
	const auto isUserChosenWallPaper = Data::IsCustomWallPaper(_paper)
		|| (Data::IsCloudWallPaper(_paper) && !Data::IsDefaultWallPaper(_paper));
	if (!isUserChosenWallPaper) {
		setPrepared(QImage(), QImage(), QImage());
		return;
	}"""

if old_set_prepared in tc:
    tc = tc.replace(old_set_prepared, new_set_prepared, 1)

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/themes/window_theme.cpp", "w", encoding="utf-8") as f:
    f.write(tc)
print("Updated window_theme.cpp!")

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/window_session_controller.cpp", "r", encoding="utf-8") as f:
    sc = f.read()

old_push = """void SessionController::pushDefaultChatBackground() {
	const auto background = Theme::Background();
	const auto &paper = background->paper();
	_defaultChatTheme->setBackground({
		.prepared = background->prepared(),
		.preparedForTiled = background->preparedForTiled(),
		.gradientForFill = background->gradientForFill(),
		.colorForFill = background->colorForFill(),
		.colors = paper.backgroundColors(),
		.patternOpacity = paper.patternOpacity(),
		.gradientRotation = paper.gradientRotation(),
		.isPattern = paper.isPattern(),
		.tile = background->tile(),
	});"""

new_push = """void SessionController::pushDefaultChatBackground() {
	const auto background = Theme::Background();
	const auto &paper = background->paper();
	const auto isUserChosenWallPaper = Data::IsCustomWallPaper(paper)
		|| (Data::IsCloudWallPaper(paper) && !Data::IsDefaultWallPaper(paper));
	_defaultChatTheme->setBackground({
		.prepared = isUserChosenWallPaper ? background->prepared() : QImage(),
		.preparedForTiled = isUserChosenWallPaper ? background->preparedForTiled() : QImage(),
		.gradientForFill = isUserChosenWallPaper ? background->gradientForFill() : QImage(),
		.colorForFill = isUserChosenWallPaper ? background->colorForFill() : std::make_optional(st::windowBg->c),
		.colors = isUserChosenWallPaper ? paper.backgroundColors() : std::vector<QColor>{ st::windowBg->c },
		.patternOpacity = isUserChosenWallPaper ? paper.patternOpacity() : 0.0,
		.gradientRotation = isUserChosenWallPaper ? paper.gradientRotation() : 0,
		.isPattern = isUserChosenWallPaper ? paper.isPattern() : false,
		.tile = isUserChosenWallPaper ? background->tile() : false,
	});"""

if old_push in sc:
    sc = sc.replace(old_push, new_push, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/window/window_session_controller.cpp", "w", encoding="utf-8") as f:
        f.write(sc)
    print("Updated window_session_controller.cpp!")
else:
    print("old_push not found in window_session_controller.cpp!")
