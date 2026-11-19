with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_message.cpp", "r", encoding="utf-8") as f:
    c = f.read()

# 1. marginTop and marginBottom
old_margin_top = """int Message::marginTop() const {
	auto result = 0;
	if (!isHidden()) {
		if (isAttachedToPrevious()) {
			result += st::msgMarginTopAttached;
		} else {
			result += st::msgMargin.top();
		}
	}"""

new_margin_top = """int Message::marginTop() const {
	auto result = 0;
	if (!isHidden()) {
		if (isAttachedToPrevious()) {
			result += 2;
		} else {
			result += 10;
		}
	}"""

if old_margin_top in c:
    c = c.replace(old_margin_top, new_margin_top, 1)
    print("Replaced marginTop!")

old_margin_bottom = """int Message::marginBottom() const {
	if (isHidden()) {
		return 0;
	}
	auto result = st::msgMargin.bottom();
	if (const auto service = Get<ServicePreMessage>()) {
		if (service->below) {
			result += service->height;
		}
	}
	if (const auto margins = Get<ViewAddedMargins>()) {
		result += margins->bottom;
	}
	return result;
}"""

new_margin_bottom = """int Message::marginBottom() const {
	if (isHidden()) {
		return 0;
	}
	auto result = 0;
	if (const auto service = Get<ServicePreMessage>()) {
		if (service->below) {
			result += service->height;
		}
	}
	if (const auto margins = Get<ViewAddedMargins>()) {
		result += margins->bottom;
	}
	return result;
}"""

if old_margin_bottom in c:
    c = c.replace(old_margin_bottom, new_margin_bottom, 1)
    print("Replaced marginBottom!")

# 2. countMessageRounding
old_rounding = """Ui::BubbleRounding Message::countMessageRounding() const {
	const auto smallTop = isBubbleAttachedToPrevious();
	const auto smallBottom = isBubbleAttachedToNext();
	const auto media = smallBottom ? nullptr : this->media();
	const auto rich = smallBottom ? nullptr : richpage();
	const auto item = data();
	const auto keyboard = item->inlineReplyKeyboard();
	const auto richMediaAtBottom = [&] {
		using Block = Iv::Markdown::MarkdownArticleEdgeBlock;
		const auto edges = countRichPageBubbleEdges();
		return edges.bottom
			&& !edges.infoBelow
			&& (rich->article.edgeBlocks().bottom == Block::VisualMedia);
	};
	const auto skipTail = smallBottom
		|| (media && media->skipBubbleTail())
		|| (rich && richMediaAtBottom())
		|| (keyboard != nullptr)
		|| item->isFakeAboutView()
		|| isCommentsRootView();
	const auto right = hasRightLayout();
	using Corner = Ui::BubbleCornerRounding;
	return Ui::BubbleRounding{
		.topLeft = (smallTop && !right) ? Corner::Small : Corner::Large,
		.topRight = (smallTop && right) ? Corner::Small : Corner::Large,
		.bottomLeft = ((smallBottom && !right)
			? Corner::Small
			: (!skipTail && !right)
			? Corner::Tail
			: Corner::Large),
		.bottomRight = ((smallBottom && right)
			? Corner::Small
			: (!skipTail && right)
			? Corner::Tail
			: Corner::Large),
	};
}"""

new_rounding = """Ui::BubbleRounding Message::countMessageRounding() const {
	const auto smallTop = isBubbleAttachedToPrevious();
	const auto smallBottom = isBubbleAttachedToNext();
	const auto right = hasRightLayout();
	using Corner = Ui::BubbleCornerRounding;
	return Ui::BubbleRounding{
		.topLeft = (smallTop && !right) ? Corner::Small : Corner::Large,
		.topRight = (smallTop && right) ? Corner::Small : Corner::Large,
		.bottomLeft = (smallBottom && !right) ? Corner::Small : Corner::Large,
		.bottomRight = (smallBottom && right) ? Corner::Small : Corner::Large,
	};
}"""

if old_rounding in c:
    c = c.replace(old_rounding, new_rounding, 1)
    print("Replaced countMessageRounding!")

# 3. hasFromPhoto
old_has_from_photo = """bool Message::hasFromPhoto() const {
	if (isHidden()) {
		return false;
	}
	switch (context()) {
	case Context::AdminLog:
		return true;
	case Context::WelcomeMessages:
		if (const auto channel = data()->history()->peer->asBroadcast()) {
			return channel->signatureProfiles();
		}
		return true;
	case Context::Monoforum:
		return (delegate()->elementChatMode() == ElementChatMode::Wide);
	case Context::History:
	case Context::ChatPreview:
	case Context::TTLViewer:
	case Context::MediaEditor:
	case Context::Pinned:
	case Context::Replies:
	case Context::SavedSublist:
	case Context::ScheduledTopic: {
		const auto item = data();
		if (item->isSponsored()) {
			return false;
		} else if (item->isPost()) {
			return (context() == Context::MediaEditor)
				|| !item->isPostHidingAuthor();
		} else if (item->isEmpty()
			|| item->isFakeAboutView()
			|| isCommentsRootView()) {
			return false;
		}
		if (item->isEphemeral()) {
			if (const auto channel = item->history()->peer->asBroadcast()) {
				return channel->signatureProfiles();
			}
		}
		const auto mode = delegate()->elementChatMode();
		if (mode != ElementChatMode::Default) {
			return (mode == ElementChatMode::Wide);
		} else if (item->history()->peer->isVerifyCodes()) {
			return !hasOutLayout();
		} else if (item->Has<HistoryMessageForwarded>()) {
			const auto peer = item->history()->peer;
			if (peer->isSelf() || peer->isRepliesChat()) {
				return !hasOutLayout();
			}
		}
		if (item->isGuestChatBotMessage()) {
			return true;
		}
		return !item->out() && !item->history()->peer->isUser();
	} break;
	case Context::ContactPreview:
	case Context::ShortcutMessages:
		return false;
	}
	Unexpected("Context in Message::hasFromPhoto.");
}"""

new_has_from_photo = """bool Message::hasFromPhoto() const {
	if (isHidden()) {
		return false;
	}
	const auto item = data();
	const auto peer = item->history()->peer;
	if (peer->isUser()) {
		return false;
	}
	if (hasOutLayout() || item->out()) {
		return false;
	}
	if (const auto channel = peer->asBroadcast()) {
		return channel->signatureProfiles();
	}
	return true;
}"""

if old_has_from_photo in c:
    c = c.replace(old_has_from_photo, new_has_from_photo, 1)
    print("Replaced hasFromPhoto!")

# 4. countGeometry
old_count_geo = """QRect Message::countGeometry() const {
	const auto item = data();
	const auto centeredView = item->isFakeAboutView()
		|| isCommentsRootView();
	const auto media = this->media();
	const auto mediaDisplayed = media && media->isDisplayed();
	const auto mediaWidth = mediaDisplayed ? media->width() : width();
	const auto outbg = hasOutLayout();
	const auto useMoreSpace = (delegate()->elementChatMode()
		== ElementChatMode::Narrow);
	const auto wideSkip = useMoreSpace
		? st::msgMargin.left()
		: st::msgMargin.right();
	const auto availableWidth = width()
		- st::msgMargin.left()
		- (centeredView ? st::msgMargin.left() : wideSkip);
	auto contentLeft = hasRightLayout() ? wideSkip : st::msgMargin.left();
	auto contentWidth = availableWidth;
	if (hasFromPhoto()) {
		contentLeft += st::msgPhotoSkip;
		if (const auto size = rightActionSize()) {
			contentWidth -= size->width() + (st::msgPhotoSkip - st::historyFastShareSize);
		}
	//} else if (!Adaptive::Wide() && !out() && !fromChannel() && st::msgPhotoSkip - (hmaxwidth - hwidth) > 0) {
	//	contentLeft += st::msgPhotoSkip - (hmaxwidth - hwidth);
	}
	accumulate_min(contentWidth, maxWidth());
	accumulate_min(contentWidth, int(_bubbleWidthLimit));
	if (mediaWidth < contentWidth) {
		const auto textualWidth = bubbleTextualWidth();
		if (mediaWidth < textualWidth
			&& (!media || !media->enforceBubbleWidth())) {
			accumulate_min(contentWidth, textualWidth);
		} else {
			contentWidth = mediaWidth;
		}
	} else if (!mediaDisplayed) {
		const auto appearing = Get<TextAppearing>();
		const auto use = (appearing && appearing->use)
			? appearing->shownWidth
			: textRealWidth();
		if (use > 0) {
			const auto shrunk = std::max(
				use + st::msgPadding.left() + st::msgPadding.right(),
				int(_nonTextMaxWidth));
			accumulate_min(contentWidth, shrunk);
		}
	}
	if (contentWidth < availableWidth
		&& delegate()->elementChatMode() != ElementChatMode::Wide) {
		if (outbg) {
			contentLeft += availableWidth - contentWidth;
		} else if (centeredView) {
			contentLeft += (availableWidth - contentWidth) / 2;
		}
	} else if (contentWidth < availableWidth && centeredView) {
		contentLeft += std::max(
			((st::msgMaxWidth + 2 * st::msgPhotoSkip) - contentWidth) / 2,
			0);
	}

	const auto contentTop = marginTop();
	return QRect(
		contentLeft,
		contentTop,
		contentWidth,
		height() - contentTop - marginBottom());
}"""

new_count_geo = """QRect Message::countGeometry() const {
	const auto item = data();
	const auto centeredView = item->isFakeAboutView()
		|| isCommentsRootView();
	const auto media = this->media();
	const auto mediaDisplayed = media && media->isDisplayed();
	const auto mediaWidth = mediaDisplayed ? media->width() : width();
	const auto outbg = hasOutLayout();

	const auto totalWidth = width();
	constexpr auto kMaxColumnWidth = 800;
	constexpr auto kMinSideMargin = 16;
	const auto columnWidth = std::max(100, std::min(totalWidth - 2 * kMinSideMargin, kMaxColumnWidth));
	const auto columnLeft = (totalWidth - columnWidth) / 2;
	const auto maxBubbleWidth = std::max(60, int(columnWidth * 0.78));

	auto availableWidth = maxBubbleWidth;
	if (hasFromPhoto()) {
		availableWidth -= st::msgPhotoSkip;
		if (const auto size = rightActionSize()) {
			availableWidth -= size->width() + (st::msgPhotoSkip - st::historyFastShareSize);
		}
	}
	auto contentWidth = availableWidth;
	accumulate_min(contentWidth, maxWidth());
	accumulate_min(contentWidth, int(_bubbleWidthLimit));
	if (mediaWidth < contentWidth) {
		const auto textualWidth = bubbleTextualWidth();
		if (mediaWidth < textualWidth
			&& (!media || !media->enforceBubbleWidth())) {
			accumulate_min(contentWidth, textualWidth);
		} else {
			contentWidth = mediaWidth;
		}
	} else if (!mediaDisplayed) {
		const auto appearing = Get<TextAppearing>();
		const auto use = (appearing && appearing->use)
			? appearing->shownWidth
			: textRealWidth();
		if (use > 0) {
			const auto shrunk = std::max(
				use + st::msgPadding.left() + st::msgPadding.right(),
				int(_nonTextMaxWidth));
			accumulate_min(contentWidth, shrunk);
		}
	}

	int contentLeft = 0;
	if (outbg) {
		contentLeft = columnLeft + (columnWidth - contentWidth);
	} else if (centeredView) {
		contentLeft = columnLeft + (columnWidth - contentWidth) / 2;
	} else {
		contentLeft = columnLeft;
		if (hasFromPhoto()) {
			contentLeft += st::msgPhotoSkip;
		}
	}

	const auto contentTop = marginTop();
	return QRect(
		contentLeft,
		contentTop,
		contentWidth,
		height() - contentTop - marginBottom());
}"""

if old_count_geo in c:
    c = c.replace(old_count_geo, new_count_geo, 1)
    print("Replaced countGeometry!")

# 5. resizeContentGetHeight
old_resize = """	const auto centeredView = item->isFakeAboutView()
		|| isCommentsRootView();
	const auto useMoreSpace = (delegate()->elementChatMode()
		== ElementChatMode::Narrow);
	const auto wideSkip = useMoreSpace
		? st::msgMargin.left()
		: st::msgMargin.right();
	auto contentWidth = newWidth
		- st::msgMargin.left()
		- (centeredView ? st::msgMargin.left() : wideSkip);
	if (hasFromPhoto()) {
		if (const auto size = rightActionSize()) {
			contentWidth -= size->width() + (st::msgPhotoSkip - st::historyFastShareSize);
		}
	}
	accumulate_min(contentWidth, maxWidth());
	_bubbleWidthLimit = (UnlimitedMessageWidth.value() && !mediaDisplayed)
		? 0x3FFFFFF
		: std::max({
			st::msgMaxWidth,
			monospaceMaxWidth(),
			mediaDisplayed ? media->bubbleWidthLimit() : 0,
		});
	accumulate_min(contentWidth, int(_bubbleWidthLimit));"""

new_resize = """	constexpr auto kMaxColumnWidth = 800;
	constexpr auto kMinSideMargin = 16;
	const auto columnWidth = std::max(100, std::min(newWidth - 2 * kMinSideMargin, kMaxColumnWidth));
	const auto maxBubbleWidth = std::max(60, int(columnWidth * 0.78));

	auto contentWidth = maxBubbleWidth;
	if (hasFromPhoto()) {
		contentWidth -= st::msgPhotoSkip;
		if (const auto size = rightActionSize()) {
			contentWidth -= size->width() + (st::msgPhotoSkip - st::historyFastShareSize);
		}
	}
	accumulate_min(contentWidth, maxWidth());
	_bubbleWidthLimit = (UnlimitedMessageWidth.value() && !mediaDisplayed)
		? 0x3FFFFFF
		: std::max({
			maxBubbleWidth,
			monospaceMaxWidth(),
			mediaDisplayed ? media->bubbleWidthLimit() : 0,
		});
	accumulate_min(contentWidth, int(_bubbleWidthLimit));"""

if old_resize in c:
    c = c.replace(old_resize, new_resize, 1)
    print("Replaced resizeContentGetHeight!")

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/history/view/history_view_message.cpp", "w", encoding="utf-8") as f:
    f.write(c)

print("Finished updating history_view_message.cpp!")
