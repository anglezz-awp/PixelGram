# 1. Update history_view_element.cpp hasRightLayout
with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_element.cpp", "r", encoding="utf-8") as f:
    c = f.read()

old_hasRight = """bool Element::hasRightLayout() const {
	return hasOutLayout()
		&& (_delegate->elementChatMode() != ElementChatMode::Wide);
}"""

new_hasRight = """bool Element::hasRightLayout() const {
	return hasOutLayout();
}"""

if old_hasRight in c:
    c = c.replace(old_hasRight, new_hasRight, 1)
    with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_element.cpp", "w", encoding="utf-8") as f:
        f.write(c)
    print("Updated history_view_element.cpp hasRightLayout!")
else:
    print("old_hasRight not found in history_view_element.cpp")
