#pragma once

#include <QtGui/QColor>

namespace PixelGram {

struct M3SchemeRoles {
	QColor primary;
	QColor onPrimary;
	QColor primaryContainer;
	QColor onPrimaryContainer;
	QColor secondary;
	QColor onSecondary;
	QColor secondaryContainer;
	QColor onSecondaryContainer;
	QColor tertiary;
	QColor onTertiary;
	QColor tertiaryContainer;
	QColor onTertiaryContainer;
	QColor error;
	QColor onError;
	QColor errorContainer;
	QColor onErrorContainer;
	QColor surface;
	QColor surfaceDim;
	QColor surfaceBright;
	QColor surfaceContainerLowest;
	QColor surfaceContainerLow;
	QColor surfaceContainer;
	QColor surfaceContainerHigh;
	QColor surfaceContainerHighest;
	QColor onSurface;
	QColor onSurfaceVariant;
	QColor outline;
	QColor outlineVariant;
	QColor shadow;
	QColor scrim;
	QColor inverseSurface;
	QColor inverseOnSurface;
	QColor inversePrimary;
	QColor fixedWhite;
	QColor fixedBlack;
};

[[nodiscard]] M3SchemeRoles GenerateM3SchemeFromSeed(
	const QColor &seed,
	bool isDark,
	int variant = 0,
	double contrast = 0.0);

[[nodiscard]] const M3SchemeRoles &CurrentM3Roles();
[[nodiscard]] QColor GetM3SeedColor();
void SetM3SeedColor(const QColor &color);
void ApplyM3Scheme(const QColor &seed, bool isDark);

} // namespace PixelGram
