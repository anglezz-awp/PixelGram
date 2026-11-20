#include "ui/pixelgram_wavy_progress.h"
#include "ui/pixelgram_m3_scheme.h"
#include "ui/painter.h"
#include <QtCore/QtMath>
#include <crl/crl_time.h>

namespace PixelGram {

float64 CurrentWavyPhase() {
	const auto ms = crl::now();
	// Cycle every 1600ms for natural flowing motion
	return (ms % 1600) * (2.0 * M_PI / 1600.0);
}

QPainterPath BuildSinusoidalPath(
		float64 x1,
		float64 x2,
		float64 centerY,
		float64 wavelength,
		float64 amplitude,
		float64 phase) {
	QPainterPath path;
	if (x2 <= x1) {
		return path;
	}
	if (amplitude < 0.1 || wavelength < 4.0) {
		path.moveTo(x1, centerY);
		path.lineTo(x2, centerY);
		return path;
	}

	constexpr auto kStep = 1.5;
	const auto twoPiOverLambda = 2.0 * M_PI / wavelength;

	const auto startY = centerY + amplitude * std::sin(phase);
	path.moveTo(x1, startY);

	for (float64 x = x1 + kStep; x < x2; x += kStep) {
		const auto y = centerY + amplitude * std::sin(phase + twoPiOverLambda * (x - x1));
		path.lineTo(x, y);
	}
	const auto endY = centerY + amplitude * std::sin(phase + twoPiOverLambda * (x2 - x1));
	path.lineTo(x2, endY);

	return path;
}

void PaintWavyTrack(
		QPainter &p,
		const QRectF &rect,
		const WavyTrackOptions &options) {
	if (rect.width() <= 0 || rect.height() <= 0) {
		return;
	}

	PainterHighQualityEnabler hq(p);

	const auto &roles = CurrentM3Roles();
	const auto activeColor = options.activeColor.isValid()
		? options.activeColor
		: (roles.primary.isValid() ? roles.primary : QColor(0x91, 0x7B, 0xBD));
	const auto inactiveColor = options.inactiveColor.isValid()
		? options.inactiveColor
		: (roles.surfaceContainerHighest.isValid() ? roles.surfaceContainerHighest : QColor(0xE6, 0xE0, 0xEA));
	const auto thumbColor = options.thumbColor.isValid()
		? options.thumbColor
		: activeColor;

	const auto centerY = rect.center().y();
	const auto xStart = rect.left();
	const auto xEnd = rect.right();
	const auto totalWidth = rect.width();
	const auto progress = std::clamp(options.progress, 0.0, 1.0);

	const auto amplitude = options.isPlaying ? options.amplitude : 0.0;
	const auto phase = options.isPlaying ? (options.phase != 0.0 ? options.phase : CurrentWavyPhase()) : 0.0;

	QPen activePen(activeColor, options.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	QPen inactivePen(inactiveColor, options.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

	if (!options.showThumb) {
		// Progress bar without thumb (linear indicator)
		const auto midX = xStart + progress * totalWidth;
		if (midX > xStart) {
			p.setPen(activePen);
			p.setBrush(Qt::NoBrush);
			p.drawPath(BuildSinusoidalPath(xStart, midX, centerY, options.wavelength, amplitude, phase));
		}
		if (xEnd > midX) {
			p.setPen(inactivePen);
			p.setBrush(Qt::NoBrush);
			p.drawLine(QPointF(midX, centerY), QPointF(xEnd, centerY));
		}
		return;
	}

	// Seekbar with vertical rounded thumb
	const auto thumbW = options.thumbSize.width();
	const auto thumbH = std::min<float64>(options.thumbSize.height(), rect.height());
	const auto thumbHalfW = thumbW / 2.0;
	const auto thumbGap = options.thumbGap;

	const auto thumbCenterX = std::clamp(xStart + progress * totalWidth, xStart + thumbHalfW, xEnd - thumbHalfW);

	// 1. Active sinusoidal track (0 .. progress - gap)
	const auto activeEndX = thumbCenterX - thumbHalfW - thumbGap;
	if (activeEndX > xStart) {
		p.setPen(activePen);
		p.setBrush(Qt::NoBrush);
		p.drawPath(BuildSinusoidalPath(xStart, activeEndX, centerY, options.wavelength, amplitude, phase));
	}

	// 2. Inactive straight track (progress + gap .. 1.0)
	const auto inactiveStartX = thumbCenterX + thumbHalfW + thumbGap;
	if (xEnd > inactiveStartX) {
		p.setPen(inactivePen);
		p.setBrush(Qt::NoBrush);
		p.drawLine(QPointF(inactiveStartX, centerY), QPointF(xEnd, centerY));
	}

	// 3. Vertical rounded thumb pill
	const auto thumbRect = QRectF(
		thumbCenterX - thumbHalfW,
		centerY - thumbH / 2.0,
		thumbW,
		thumbH);

	p.setPen(Qt::NoPen);
	p.setBrush(thumbColor);
	p.drawRoundedRect(thumbRect, options.thumbRadius, options.thumbRadius);
}

} // namespace PixelGram
