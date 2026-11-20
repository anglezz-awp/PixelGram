#pragma once

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <base/basic_types.h>

namespace PixelGram {

struct WavyTrackOptions {
	float64 progress = 0.0;         // 0.0 to 1.0
	float64 phase = 0.0;            // wave phase offset in radians
	float64 amplitude = 3.5;        // amplitude in pixels (0.0 = straight line)
	float64 wavelength = 50.0;      // wavelength in pixels (~24-28px)
	float64 strokeWidth = 3.5;      // track line thickness
	bool isPlaying = true;          // whether animation is active
	bool showThumb = true;          // show M3 vertical pill thumb
	QSize thumbSize = QSize(4, 20); // vertical bar: width 4px, height 20-24px
	float64 thumbRadius = 2.0;      // rounded ends
	float64 thumbGap = 3.0;         // visual gap between track and thumb
	QColor activeColor;             // roles.primary
	QColor inactiveColor;           // roles.secondaryContainer / surfaceContainerHighest
	QColor thumbColor;              // roles.primary
};

// Generates smooth sinusoidal QPainterPath between x1 and x2
[[nodiscard]] QPainterPath BuildSinusoidalPath(
	float64 x1,
	float64 x2,
	float64 centerY,
	float64 wavelength,
	float64 amplitude,
	float64 phase);

// Paints the Material 3 Expressive wavy track and vertical thumb
void PaintWavyTrack(
	QPainter &p,
	const QRectF &rect,
	const WavyTrackOptions &options);

// Helper for live animated phase based on monotonic clock
[[nodiscard]] float64 CurrentWavyPhase();

} // namespace PixelGram
