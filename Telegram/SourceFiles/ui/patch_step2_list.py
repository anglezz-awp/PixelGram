with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_list_widget.cpp", "r", encoding="utf-8") as f:
    c = f.read()

old_paint_userpic = """		// paint the userpic if it intersects the painted rect
		if (userpicTop + st::msgPhotoSize > clip.top()) {
			const auto item = view->data();
			const auto shift = context.gestureHorizontal.visualTranslationFor(
				item->id.bare);
			if (shift) {
				p.translate(shift, 0);
				update(
					QRect(
						st::historyPhotoLeft + std::min(shift, 0),
						userpicTop,
						st::msgPhotoSize + std::abs(shift),
						st::msgPhotoSize));
			}
			if (const auto from = view->displayFrom()) {
				Dialogs::Ui::PaintUserpic(
					p,
					from,
					validateVideoUserpic(from),
					_userpics[from],
					st::historyPhotoLeft,
					userpicTop,
					view->width(),
					st::msgPhotoSize,
					context.paused);
			} else if (const auto info = item->displayHiddenSenderInfo()) {
				if (info->customUserpic.empty()) {
					info->emptyUserpic.paintCircle(
						p,
						st::historyPhotoLeft,
						userpicTop,
						view->width(),
						st::msgPhotoSize);
				} else {
					auto &userpic = _hiddenSenderUserpics[item->id];
					const auto valid = info->paintCustomUserpic(
						p,
						userpic,
						st::historyPhotoLeft,
						userpicTop,
						view->width(),
						st::msgPhotoSize);
					if (!valid) {
						info->customUserpic.load(session, item->fullId());
					}
				}
			}"""

new_paint_userpic = """		// paint the userpic if it intersects the painted rect
		if (userpicTop + st::msgPhotoSize > clip.top()) {
			const auto item = view->data();
			const auto totalWidth = view->width();
			constexpr auto kMaxColumnWidth = 800;
			constexpr auto kMinSideMargin = 16;
			const auto columnWidth = std::max(100, std::min(totalWidth - 2 * kMinSideMargin, kMaxColumnWidth));
			const auto columnLeft = (totalWidth - columnWidth) / 2;
			const auto photoLeft = columnLeft + st::historyPhotoLeft;

			const auto shift = context.gestureHorizontal.visualTranslationFor(
				item->id.bare);
			if (shift) {
				p.translate(shift, 0);
				update(
					QRect(
						photoLeft + std::min(shift, 0),
						userpicTop,
						st::msgPhotoSize + std::abs(shift),
						st::msgPhotoSize));
			}
			if (const auto from = view->displayFrom()) {
				Dialogs::Ui::PaintUserpic(
					p,
					from,
					validateVideoUserpic(from),
					_userpics[from],
					photoLeft,
					userpicTop,
					view->width(),
					st::msgPhotoSize,
					context.paused);
			} else if (const auto info = item->displayHiddenSenderInfo()) {
				if (info->customUserpic.empty()) {
					info->emptyUserpic.paintCircle(
						p,
						photoLeft,
						userpicTop,
						view->width(),
						st::msgPhotoSize);
				} else {
					auto &userpic = _hiddenSenderUserpics[item->id];
					const auto valid = info->paintCustomUserpic(
						p,
						userpic,
						photoLeft,
						userpicTop,
						view->width(),
						st::msgPhotoSize);
					if (!valid) {
						info->customUserpic.load(session, item->fullId());
					}
				}
			}"""

if old_paint_userpic in c:
    c = c.replace(old_paint_userpic, new_paint_userpic, 1)
    print("Replaced paintUserpic!")

# Also click detection on userpic:
old_click = """			&& itemPoint.x() >= st::historyPhotoLeft
			&& itemPoint.x() < st::historyPhotoLeft + st::msgPhotoSize) {"""

new_click = """			&& itemPoint.x() >= (((view->width() - std::max(100, std::min(view->width() - 32, 800))) / 2) + st::historyPhotoLeft)
			&& itemPoint.x() < (((view->width() - std::max(100, std::min(view->width() - 32, 800))) / 2) + st::historyPhotoLeft + st::msgPhotoSize)) {"""

if old_click in c:
    c = c.replace(old_click, new_click, 1)
    print("Replaced userpic click detection!")

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_list_widget.cpp", "w", encoding="utf-8") as f:
    f.write(c)
print("Finished updating history_view_list_widget.cpp!")
