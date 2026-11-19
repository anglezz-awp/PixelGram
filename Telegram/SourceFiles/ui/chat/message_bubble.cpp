/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/message_bubble.h"

#include "ui/cached_round_corners.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include <QtGui/QPainterPath>
#include "ui/chat/chat_style.h"
#include "ui/chat/torn_edge.h"
#include "styles/style_chat.h"
#include "styles/style_chat_style.h"

namespace Ui {
namespace {

using Corner = BubbleCornerRounding;

[[nodiscard]] bool UsePatternBubble(const SimpleBubble &args) {
	return !args.selected
		&& args.outbg
		&& args.pattern
		&& !args.patternViewport.isEmpty()
		&& !args.pattern->pixmap.size().isEmpty();
}

[[nodiscard]] float64 PatternBubbleOpacity(
		const SimpleBubble &args,
		float64 wasOpacity) {
	return args.st->msgOutBg()->c.alphaF() * wasOpacity;
}

void PaintBubblePiece(
		QPainter &p,
		const SimpleBubble &args,
		QRect geometry,
		bool fromTop,
		bool tillBottom) {
	auto simple = args;
	simple.geometry = geometry;
	if (!fromTop) {
		simple.rounding.topLeft
			= simple.rounding.topRight
			= Corner::None;
	}
	if (!tillBottom) {
		simple.rounding.bottomLeft
			= simple.rounding.bottomRight
			= Corner::None;
		simple.shadowed = false;
	}
	PaintBubble(p, simple);
}

void PaintBubblePiece(
		QPainter &p,
		const SimpleBubble &args,
		QRect geometry,
		bool selected,
		bool fromTop,
		bool tillBottom) {
	auto simple = args;
	simple.selected = selected;
	PaintBubblePiece(p, simple, geometry, fromTop, tillBottom);
}

[[nodiscard]] QPainterPath BuildBubblePath(
		const QRect &rect,
		BubbleRounding rounding) {
	const auto small = BubbleRadiusSmall();
	const auto large = BubbleRadiusLarge();
	const auto radiusFor = [&](Corner corner) -> qreal {
		switch (corner) {
		case Corner::None: return 0.;
		case Corner::Small: return small;
		case Corner::Large:
		case Corner::Tail: return large;
		}
		return 0.;
	};
	auto tl = radiusFor(rounding.topLeft);
	auto tr = radiusFor(rounding.topRight);
	auto bl = radiusFor(rounding.bottomLeft);
	auto br = radiusFor(rounding.bottomRight);

	const auto rf = QRectF(rect);
	if (rf.isEmpty()) {
		return QPainterPath();
	}
	if (tl <= 0. && tr <= 0. && bl <= 0. && br <= 0.) {
		auto path = QPainterPath();
		path.addRect(rf);
		return path;
	}
	const auto scale = std::min({
		qreal(1.),
		(tl + tr > 0.) ? (rf.width() / (tl + tr)) : qreal(1.),
		(bl + br > 0.) ? (rf.width() / (bl + br)) : qreal(1.),
		(tl + bl > 0.) ? (rf.height() / (tl + bl)) : qreal(1.),
		(tr + br > 0.) ? (rf.height() / (tr + br)) : qreal(1.),
	});
	tl *= scale;
	tr *= scale;
	bl *= scale;
	br *= scale;

	auto path = QPainterPath();
	path.moveTo(rf.left() + tl, rf.top());
	path.lineTo(rf.right() - tr, rf.top());
	if (tr > 0.) {
		path.arcTo(
			QRectF(rf.right() - 2. * tr, rf.top(), 2. * tr, 2. * tr),
			90.,
			-90.);
	}
	path.lineTo(rf.right(), rf.bottom() - br);
	if (br > 0.) {
		path.arcTo(
			QRectF(
				rf.right() - 2. * br,
				rf.bottom() - 2. * br,
				2. * br,
				2. * br),
			0.,
			-90.);
	}
	path.lineTo(rf.left() + bl, rf.bottom());
	if (bl > 0.) {
		path.arcTo(
			QRectF(rf.left(), rf.bottom() - 2. * bl, 2. * bl, 2. * bl),
			270.,
			-90.);
	}
	path.lineTo(rf.left(), rf.top() + tl);
	if (tl > 0.) {
		path.arcTo(
			QRectF(rf.left(), rf.top(), 2. * tl, 2. * tl),
			180.,
			-90.);
	}
	path.closeSubpath();
	return path;
}

void PaintPatternBubble(QPainter &p, const SimpleBubble &args) {
	const auto rect = args.geometry;
	const auto target = rect.intersected(args.patternViewport);
	if (target.isEmpty()) {
		return;
	}
	const auto wasOpacity = p.opacity();
	const auto opacity = PatternBubbleOpacity(args, wasOpacity);
	const auto pattern = args.pattern;
	const auto path = BuildBubblePath(rect, args.rounding);

	p.setOpacity(opacity);
	PaintPatternBubblePart(
		p,
		args.patternViewport,
		pattern->pixmap,
		target,
		[&](QPainter &q) {
			auto hq = PainterHighQualityEnabler(q);
			q.setPen(Qt::NoPen);
			q.setBrush(Qt::white);
			q.drawPath(path);
		},
		pattern->bubbleCache);
	p.setOpacity(wasOpacity);
}

void PaintSolidBubble(QPainter &p, const SimpleBubble &args) {
	const auto &st = args.st->messageStyle(args.outbg, args.selected);
	const auto rect = args.geometry;
	if (rect.isEmpty()) {
		return;
	}
	p.save();
	{
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(st.msgBg);
		p.drawPath(BuildBubblePath(rect, args.rounding));
	}
	p.restore();
}

} // namespace

std::unique_ptr<BubblePattern> PrepareBubblePattern(
		not_null<const style::palette*> st) {
	auto result = std::make_unique<Ui::BubblePattern>();
	result->cornersSmall = Images::CornersMask(BubbleRadiusSmall());
	result->cornersLarge = Images::CornersMask(BubbleRadiusLarge());
	result->cornerTopSmallCache = QImage(
		result->cornersSmall[0].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerTopLargeCache = QImage(
		result->cornersLarge[0].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerBottomSmallCache = QImage(
		result->cornersSmall[2].size(),
		QImage::Format_ARGB32_Premultiplied);
	result->cornerBottomLargeCache = QImage(
		result->cornersLarge[2].size(),
		QImage::Format_ARGB32_Premultiplied);
	return result;
}

void FinishBubblePatternOnMain(not_null<BubblePattern*> pattern) {
}

void PaintBubble(QPainter &p, const SimpleBubble &args) {
	if (UsePatternBubble(args)) {
		PaintPatternBubble(p, args);
	} else {
		PaintSolidBubble(p, args);
	}
}

void PaintBubble(QPainter &p, const ComplexBubble &args) {
	if (args.selection.empty()) {
		PaintBubble(p, args.simple);
		return;
	}
	const auto rect = args.simple.geometry;
	const auto left = rect.x();
	const auto width = rect.width();
	const auto top = rect.y();
	const auto bottom = top + rect.height();
	auto from = top;
	for (const auto &selected : args.selection) {
		if (selected.top > from) {
			PaintBubblePiece(
				p,
				args.simple,
				QRect(left, from, width, selected.top - from),
				false,
				(from <= top),
				false);
		}
		PaintBubblePiece(
			p,
			args.simple,
			QRect(left, selected.top, width, selected.height),
			true,
			(selected.top <= top),
			(selected.top + selected.height >= bottom));
		from = selected.top + selected.height;
	}
	if (from < bottom) {
		PaintBubblePiece(
			p,
			args.simple,
			QRect(left, from, width, bottom - from),
			false,
			false,
			true);
	}
}

void PaintBubble(QPainter &p, const BubbleWithGaps &args) {
	if (args.gaps.empty()) {
		PaintBubble(p, args.simple);
		return;
	}
	const auto rect = args.simple.geometry;
	const auto left = rect.x();
	const auto width = rect.width();
	const auto top = rect.y();
	const auto bottom = top + rect.height();
	const auto pattern = UsePatternBubble(args.simple);
	const auto paintStrip = [&](
			const QImage &mask,
			QImage &patternCache,
			QImage &solidCache,
			QColor &solidColor,
			int y) {
		const auto target = QRect(
			left,
			y,
			width,
			mask.height() / int(mask.devicePixelRatio()));
		if (pattern) {
			const auto wasOpacity = p.opacity();
			p.setOpacity(PatternBubbleOpacity(args.simple, wasOpacity));
			PaintPatternBubblePart(
				p,
				args.simple.patternViewport,
				args.simple.pattern->pixmap,
				target,
				mask,
				patternCache);
			p.setOpacity(wasOpacity);
		} else {
			const auto &st = args.simple.st->messageStyle(
				args.simple.outbg,
				args.simple.selected);
			const auto color = st.msgBg->c;
			if (solidCache.size() != mask.size() || solidColor != color) {
				if (solidCache.size() != mask.size()) {
					solidCache = QImage(
						mask.size(),
						QImage::Format_ARGB32_Premultiplied);
					solidCache.setDevicePixelRatio(mask.devicePixelRatio());
				}
				style::colorizeImage(mask, color, &solidCache);
				solidColor = color;
			}
			p.drawImage(target, solidCache);
		}
	};
	const auto torn = args.torn;
	const auto stripHeight = torn->maskTop.height()
		/ int(torn->maskTop.devicePixelRatio());
	auto from = top;
	for (const auto &gap : args.gaps) {
		const auto gapTop = gap.top;
		const auto gapBottom = gap.top + gap.height;
		if (gapTop > from) {
			PaintBubblePiece(
				p,
				args.simple,
				QRect(left, from, width, gapTop - from),
				(from <= top),
				false);
			paintStrip(
				torn->maskBottom,
				torn->patternCacheBottom,
				torn->solidCacheBottom,
				torn->solidColorBottom,
				gapTop);
		}
		if (gapBottom < bottom) {
			paintStrip(
				torn->maskTop,
				torn->patternCacheTop,
				torn->solidCacheTop,
				torn->solidColorTop,
				gapBottom - stripHeight);
		}
		from = gapBottom;
	}
	if (from < bottom) {
		PaintBubblePiece(
			p,
			args.simple,
			QRect(left, from, width, bottom - from),
			false,
			true);
	}
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target) {
	const auto factor = pixmap.devicePixelRatio();
	if (viewport.size() * factor == pixmap.size()) {
		const auto fill = target.intersected(viewport);
		if (fill.isEmpty()) {
			return;
		}
		p.drawPixmap(fill, pixmap, QRect(
			(fill.topLeft() - viewport.topLeft()) * factor,
			fill.size() * factor));
	} else {
		const auto to = viewport;
		const auto from = QRect(QPoint(), pixmap.size());
		const auto deviceRect = QRect(
			QPoint(),
			QSize(p.device()->width(), p.device()->height()));
		const auto clip = (target != deviceRect);
		if (clip) {
			p.setClipRect(target);
		}
		p.drawPixmap(to, pixmap, from);
		if (clip) {
			p.setClipping(false);
		}
	}
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target,
		const QImage &mask,
		QImage &cache) {
	Expects(mask.bytesPerLine() == mask.width() * 4);
	Expects(mask.format() == QImage::Format_ARGB32_Premultiplied);

	if (cache.size() != mask.size()) {
		cache = QImage(
			mask.size(),
			QImage::Format_ARGB32_Premultiplied);
	}
	cache.setDevicePixelRatio(mask.devicePixelRatio());
	Assert(cache.bytesPerLine() == cache.width() * 4);
	memcpy(cache.bits(), mask.constBits(), mask.sizeInBytes());

	auto q = QPainter(&cache);
	q.setCompositionMode(QPainter::CompositionMode_SourceIn);
	PaintPatternBubblePart(
		q,
		viewport.translated(-target.topLeft()),
		pixmap,
		QRect(QPoint(), cache.size() / int(cache.devicePixelRatio())));
	q.end();

	p.drawImage(target, cache);
}

void PaintPatternBubblePart(
		QPainter &p,
		const QRect &viewport,
		const QPixmap &pixmap,
		const QRect &target,
		Fn<void(QPainter&)> paintContent,
		QImage &cache) {
	Expects(paintContent != nullptr);

	const auto targetOrigin = target.topLeft();
	const auto targetSize = target.size();
	if (cache.size() != targetSize * style::DevicePixelRatio()) {
		cache = QImage(
			target.size() * style::DevicePixelRatio(),
			QImage::Format_ARGB32_Premultiplied);
		cache.setDevicePixelRatio(style::DevicePixelRatio());
	}
	cache.fill(Qt::transparent);
	auto q = QPainter(&cache);
	q.translate(-targetOrigin);
	paintContent(q);
	q.translate(targetOrigin);
	q.setCompositionMode(QPainter::CompositionMode_SourceIn);
	PaintPatternBubblePart(
		q,
		viewport.translated(-targetOrigin),
		pixmap,
		QRect(QPoint(), targetSize));
	q.end();

	p.drawImage(target, cache);
}

} // namespace Ui
