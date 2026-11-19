with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_list_widget.cpp", "r", encoding="utf-8") as f:
    c = f.read()

old_click = """			if (!dragState.link
				&& itemPoint.x() >= st::historyPhotoLeft
				&& itemPoint.x() < st::historyPhotoLeft + st::msgPhotoSize) {"""

new_click = """			const auto totalWidth = view->width();
			const auto columnWidth = std::max(100, std::min(totalWidth - 32, 800));
			const auto photoLeft = ((totalWidth - columnWidth) / 2) + st::historyPhotoLeft;
			if (!dragState.link
				&& itemPoint.x() >= photoLeft
				&& itemPoint.x() < photoLeft + st::msgPhotoSize) {"""

if old_click in c:
    c = c.replace(old_click, new_click, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_list_widget.cpp", "w", encoding="utf-8") as f:
        f.write(c)
    print("Updated userpic click detection successfully!")
else:
    print("old_click not found!")
