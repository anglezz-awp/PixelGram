#include "ui/pixelgram_m3_scheme.h"
#include "settings/pixelgram_settings.h"
#include "ui/style/style_core_palette.h"
#include "ui/style/style_core_icon.h"
#include "ui/style/style_core.h"

#include "cpp/cam/hct.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_fruit_salad.h"
#include "cpp/scheme/scheme_vibrant.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_rainbow.h"
#include "cpp/scheme/scheme_fidelity.h"
#include "cpp/scheme/scheme_content.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "window/themes/window_theme.h"

#include <QtCore/QFile>
#include <QtGui/QPixmapCache>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

namespace PixelGram {

inline QColor ArgbToQColor(material_color_utilities::Argb argb) {
	return QColor(
		(argb >> 16) & 0xFF,
		(argb >> 8) & 0xFF,
		argb & 0xFF,
		(argb >> 24) & 0xFF);
}

inline material_color_utilities::Argb QColorToArgb(const QColor &c) {
	return ((c.alpha() & 0xFF) << 24)
		| ((c.red() & 0xFF) << 16)
		| ((c.green() & 0xFF) << 8)
		| (c.blue() & 0xFF);
}

M3SchemeRoles GenerateM3SchemeFromSeed(
		const QColor &seed,
		bool isDark,
		int variant,
		double contrast) {
	using namespace material_color_utilities;

	const auto argb = QColorToArgb(seed);
	Hct sourceHct(argb);

	if (variant == 9) { // OLED (AMOLED)
		M3SchemeRoles roles;
		const bool isAchromatic = (sourceHct.get_chroma() < 8.0);

		if (isDark) {
			roles.surface = QColor(0, 0, 0);
			roles.surfaceDim = QColor(0, 0, 0);
			roles.surfaceBright = QColor(0x18, 0x18, 0x18);
			roles.surfaceContainerLowest = QColor(0, 0, 0);
			roles.surfaceContainerLow = QColor(0x08, 0x08, 0x08);
			roles.surfaceContainer = QColor(0x10, 0x10, 0x10);
			roles.surfaceContainerHigh = QColor(0x18, 0x18, 0x18);
			roles.surfaceContainerHighest = QColor(0x22, 0x22, 0x22);
			roles.outline = QColor(0x38, 0x38, 0x38);
			roles.outlineVariant = QColor(0x22, 0x22, 0x22);
			roles.shadow = QColor(0, 0, 0);
			roles.scrim = QColor(0, 0, 0);
			roles.inverseSurface = QColor(255, 255, 255);
			roles.inverseOnSurface = QColor(0, 0, 0);

			roles.onSurface = QColor(255, 255, 255);
			roles.onSurfaceVariant = QColor(0xC4, 0xC4, 0xC4);
			roles.fixedWhite = QColor(255, 255, 255);
			roles.fixedBlack = QColor(0, 0, 0);

			roles.error = QColor(0xFF, 0x54, 0x49);
			roles.onError = QColor(0x69, 0x00, 0x05);
			roles.errorContainer = QColor(0x93, 0x00, 0x0A);
			roles.onErrorContainer = QColor(0xFF, 0xDA, 0xD6);

			if (isAchromatic) {
				roles.primary = QColor(255, 255, 255);
				roles.onPrimary = QColor(0, 0, 0);
				roles.primaryContainer = QColor(0x24, 0x24, 0x24);
				roles.onPrimaryContainer = QColor(255, 255, 255);
				roles.secondary = QColor(0xE0, 0xE0, 0xE0);
				roles.onSecondary = QColor(0, 0, 0);
				roles.secondaryContainer = QColor(0x1C, 0x1C, 0x1C);
				roles.onSecondaryContainer = QColor(255, 255, 255);
				roles.tertiary = QColor(0xC0, 0xC0, 0xC0);
				roles.onTertiary = QColor(0, 0, 0);
				roles.tertiaryContainer = QColor(0x16, 0x16, 0x16);
				roles.onTertiaryContainer = QColor(255, 255, 255);
				roles.inversePrimary = QColor(0, 0, 0);
			} else {
				SchemeVibrant baseScheme(sourceHct, true, contrast);
				roles.primary = ArgbToQColor(MaterialDynamicColors::Primary().GetArgb(baseScheme));
				roles.onPrimary = ArgbToQColor(MaterialDynamicColors::OnPrimary().GetArgb(baseScheme));
				roles.primaryContainer = ArgbToQColor(MaterialDynamicColors::PrimaryContainer().GetArgb(baseScheme));
				roles.onPrimaryContainer = ArgbToQColor(MaterialDynamicColors::OnPrimaryContainer().GetArgb(baseScheme));
				roles.secondary = ArgbToQColor(MaterialDynamicColors::Secondary().GetArgb(baseScheme));
				roles.onSecondary = ArgbToQColor(MaterialDynamicColors::OnSecondary().GetArgb(baseScheme));
				roles.secondaryContainer = ArgbToQColor(MaterialDynamicColors::SecondaryContainer().GetArgb(baseScheme));
				roles.onSecondaryContainer = ArgbToQColor(MaterialDynamicColors::OnSecondaryContainer().GetArgb(baseScheme));
				roles.tertiary = ArgbToQColor(MaterialDynamicColors::Tertiary().GetArgb(baseScheme));
				roles.onTertiary = ArgbToQColor(MaterialDynamicColors::OnTertiary().GetArgb(baseScheme));
				roles.tertiaryContainer = ArgbToQColor(MaterialDynamicColors::TertiaryContainer().GetArgb(baseScheme));
				roles.onTertiaryContainer = ArgbToQColor(MaterialDynamicColors::OnTertiaryContainer().GetArgb(baseScheme));
				roles.inversePrimary = ArgbToQColor(MaterialDynamicColors::InversePrimary().GetArgb(baseScheme));
			}
		} else {
			roles.surface = QColor(255, 255, 255);
			roles.surfaceDim = QColor(0xF5, 0xF5, 0xF5);
			roles.surfaceBright = QColor(255, 255, 255);
			roles.surfaceContainerLowest = QColor(255, 255, 255);
			roles.surfaceContainerLow = QColor(0xFA, 0xFA, 0xFA);
			roles.surfaceContainer = QColor(0xF0, 0xF0, 0xF0);
			roles.surfaceContainerHigh = QColor(0xE8, 0xE8, 0xE8);
			roles.surfaceContainerHighest = QColor(0xDE, 0xDE, 0xDE);
			roles.outline = QColor(0x60, 0x60, 0x60);
			roles.outlineVariant = QColor(0xD0, 0xD0, 0xD0);
			roles.shadow = QColor(0, 0, 0);
			roles.scrim = QColor(0, 0, 0);
			roles.inverseSurface = QColor(0, 0, 0);
			roles.inverseOnSurface = QColor(255, 255, 255);

			roles.onSurface = QColor(0, 0, 0);
			roles.onSurfaceVariant = QColor(0x38, 0x38, 0x38);
			roles.fixedWhite = QColor(255, 255, 255);
			roles.fixedBlack = QColor(0, 0, 0);

			roles.error = QColor(0xBA, 0x1A, 0x1A);
			roles.onError = QColor(255, 255, 255);
			roles.errorContainer = QColor(0xFF, 0xDA, 0xD6);
			roles.onErrorContainer = QColor(0x41, 0x00, 0x02);

			if (isAchromatic) {
				roles.primary = QColor(0, 0, 0);
				roles.onPrimary = QColor(255, 255, 255);
				roles.primaryContainer = QColor(0xE0, 0xE0, 0xE0);
				roles.onPrimaryContainer = QColor(0, 0, 0);
				roles.secondary = QColor(0x2B, 0x2B, 0x2B);
				roles.onSecondary = QColor(255, 255, 255);
				roles.secondaryContainer = QColor(0xEB, 0xEB, 0xEB);
				roles.onSecondaryContainer = QColor(0, 0, 0);
				roles.tertiary = QColor(0x44, 0x44, 0x44);
				roles.onTertiary = QColor(255, 255, 255);
				roles.tertiaryContainer = QColor(0xF0, 0xF0, 0xF0);
				roles.onTertiaryContainer = QColor(0, 0, 0);
				roles.inversePrimary = QColor(255, 255, 255);
			} else {
				SchemeVibrant baseScheme(sourceHct, false, contrast);
				roles.primary = ArgbToQColor(MaterialDynamicColors::Primary().GetArgb(baseScheme));
				roles.onPrimary = ArgbToQColor(MaterialDynamicColors::OnPrimary().GetArgb(baseScheme));
				roles.primaryContainer = ArgbToQColor(MaterialDynamicColors::PrimaryContainer().GetArgb(baseScheme));
				roles.onPrimaryContainer = ArgbToQColor(MaterialDynamicColors::OnPrimaryContainer().GetArgb(baseScheme));
				roles.secondary = ArgbToQColor(MaterialDynamicColors::Secondary().GetArgb(baseScheme));
				roles.onSecondary = ArgbToQColor(MaterialDynamicColors::OnSecondary().GetArgb(baseScheme));
				roles.secondaryContainer = ArgbToQColor(MaterialDynamicColors::SecondaryContainer().GetArgb(baseScheme));
				roles.onSecondaryContainer = ArgbToQColor(MaterialDynamicColors::OnSecondaryContainer().GetArgb(baseScheme));
				roles.tertiary = ArgbToQColor(MaterialDynamicColors::Tertiary().GetArgb(baseScheme));
				roles.onTertiary = ArgbToQColor(MaterialDynamicColors::OnTertiary().GetArgb(baseScheme));
				roles.tertiaryContainer = ArgbToQColor(MaterialDynamicColors::TertiaryContainer().GetArgb(baseScheme));
				roles.onTertiaryContainer = ArgbToQColor(MaterialDynamicColors::OnTertiaryContainer().GetArgb(baseScheme));
				roles.inversePrimary = ArgbToQColor(MaterialDynamicColors::InversePrimary().GetArgb(baseScheme));
			}
		}
		return roles;
	}

	std::unique_ptr<DynamicScheme> scheme;
	switch (variant) {
	case 1:
		scheme = std::make_unique<SchemeExpressive>(sourceHct, isDark, contrast);
		break;
	case 2:
		scheme = std::make_unique<SchemeFruitSalad>(sourceHct, isDark, contrast);
		break;
	case 3:
		scheme = std::make_unique<SchemeVibrant>(sourceHct, isDark, contrast);
		break;
	case 4:
		scheme = std::make_unique<SchemeNeutral>(sourceHct, isDark, contrast);
		break;
	case 5:
		scheme = std::make_unique<SchemeMonochrome>(sourceHct, isDark, contrast);
		break;
	case 6:
		scheme = std::make_unique<SchemeRainbow>(sourceHct, isDark, contrast);
		break;
	case 7:
		scheme = std::make_unique<SchemeFidelity>(sourceHct, isDark, contrast);
		break;
	case 8:
		scheme = std::make_unique<SchemeContent>(sourceHct, isDark, contrast);
		break;
	case 0:
	default:
		scheme = std::make_unique<SchemeTonalSpot>(sourceHct, isDark, contrast);
		break;
	}

	M3SchemeRoles roles;
	roles.primary = ArgbToQColor(MaterialDynamicColors::Primary().GetArgb(*scheme));
	roles.onPrimary = ArgbToQColor(MaterialDynamicColors::OnPrimary().GetArgb(*scheme));
	roles.primaryContainer = ArgbToQColor(MaterialDynamicColors::PrimaryContainer().GetArgb(*scheme));
	roles.onPrimaryContainer = ArgbToQColor(MaterialDynamicColors::OnPrimaryContainer().GetArgb(*scheme));
	roles.secondary = ArgbToQColor(MaterialDynamicColors::Secondary().GetArgb(*scheme));
	roles.onSecondary = ArgbToQColor(MaterialDynamicColors::OnSecondary().GetArgb(*scheme));
	roles.secondaryContainer = ArgbToQColor(MaterialDynamicColors::SecondaryContainer().GetArgb(*scheme));
	roles.onSecondaryContainer = ArgbToQColor(MaterialDynamicColors::OnSecondaryContainer().GetArgb(*scheme));
	roles.tertiary = ArgbToQColor(MaterialDynamicColors::Tertiary().GetArgb(*scheme));
	roles.onTertiary = ArgbToQColor(MaterialDynamicColors::OnTertiary().GetArgb(*scheme));
	roles.tertiaryContainer = ArgbToQColor(MaterialDynamicColors::TertiaryContainer().GetArgb(*scheme));
	roles.onTertiaryContainer = ArgbToQColor(MaterialDynamicColors::OnTertiaryContainer().GetArgb(*scheme));
	roles.error = ArgbToQColor(MaterialDynamicColors::Error().GetArgb(*scheme));
	roles.onError = ArgbToQColor(MaterialDynamicColors::OnError().GetArgb(*scheme));
	roles.errorContainer = ArgbToQColor(MaterialDynamicColors::ErrorContainer().GetArgb(*scheme));
	roles.onErrorContainer = ArgbToQColor(MaterialDynamicColors::OnErrorContainer().GetArgb(*scheme));
	roles.surface = ArgbToQColor(MaterialDynamicColors::Surface().GetArgb(*scheme));
	roles.surfaceDim = ArgbToQColor(MaterialDynamicColors::SurfaceDim().GetArgb(*scheme));
	roles.surfaceBright = ArgbToQColor(MaterialDynamicColors::SurfaceBright().GetArgb(*scheme));
	roles.surfaceContainerLowest = ArgbToQColor(MaterialDynamicColors::SurfaceContainerLowest().GetArgb(*scheme));
	roles.surfaceContainerLow = ArgbToQColor(MaterialDynamicColors::SurfaceContainerLow().GetArgb(*scheme));
	roles.surfaceContainer = ArgbToQColor(MaterialDynamicColors::SurfaceContainer().GetArgb(*scheme));
	roles.surfaceContainerHigh = ArgbToQColor(MaterialDynamicColors::SurfaceContainerHigh().GetArgb(*scheme));
	roles.surfaceContainerHighest = ArgbToQColor(MaterialDynamicColors::SurfaceContainerHighest().GetArgb(*scheme));
	roles.onSurface = ArgbToQColor(MaterialDynamicColors::OnSurface().GetArgb(*scheme));
	roles.onSurfaceVariant = ArgbToQColor(MaterialDynamicColors::OnSurfaceVariant().GetArgb(*scheme));
	roles.outline = ArgbToQColor(MaterialDynamicColors::Outline().GetArgb(*scheme));
	roles.outlineVariant = ArgbToQColor(MaterialDynamicColors::OutlineVariant().GetArgb(*scheme));
	roles.shadow = ArgbToQColor(MaterialDynamicColors::Shadow().GetArgb(*scheme));
	roles.scrim = ArgbToQColor(MaterialDynamicColors::Scrim().GetArgb(*scheme));
	roles.inverseSurface = ArgbToQColor(MaterialDynamicColors::InverseSurface().GetArgb(*scheme));
	roles.inverseOnSurface = ArgbToQColor(MaterialDynamicColors::InverseOnSurface().GetArgb(*scheme));
	roles.inversePrimary = ArgbToQColor(MaterialDynamicColors::InversePrimary().GetArgb(*scheme));
	roles.fixedWhite = QColor(255, 255, 255);
	roles.fixedBlack = QColor(0, 0, 0);

	return roles;
}

QColor GetM3SeedColor() {
	if (Core::Application::exists()) {
		const auto &settings = Core::App().settings();
		if (settings.systemAccentColorEnabled()) {
			if (const auto sys = Window::Theme::SystemAccentColor()) {
				return *sys;
			}
		}
	}
	const auto &config = CurrentConfig();
	return QColor::fromRgba(config.m3SeedColor);
}

void SetM3SeedColor(const QColor &color) {
	auto &config = CurrentConfig();
	config.m3SeedColor = color.rgba();
	SaveConfig();
}

struct MappingEntry {
	const char *key;
	QColor (M3SchemeRoles::*role);
	uint8 alpha;
};

// Explicit table mapping all 586 palette keys to M3 roles with 0 fallback
static const MappingEntry kMappingTable[] = {
	{ "windowBg", &M3SchemeRoles::surface, 255 },
	{ "windowFg", &M3SchemeRoles::onSurface, 255 },
	{ "windowBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "windowBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "windowFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "windowSubTextFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "windowSubTextFgOver", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "windowBoldFg", &M3SchemeRoles::onSurface, 255 },
	{ "windowBoldFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "windowBgActive", &M3SchemeRoles::primary, 255 },
	{ "windowFgActive", &M3SchemeRoles::onPrimary, 255 },
	{ "windowActiveTextFg", &M3SchemeRoles::primary, 255 },
	{ "windowShadowFg", &M3SchemeRoles::shadow, 100 },
	{ "windowShadowFgFallback", &M3SchemeRoles::outlineVariant, 255 },
	{ "shadowFg", &M3SchemeRoles::outlineVariant, 76 },
	{ "slideFadeOutBg", &M3SchemeRoles::scrim, 60 },
	{ "slideFadeOutShadowFg", &M3SchemeRoles::shadow, 100 },
	{ "imageBg", &M3SchemeRoles::surfaceContainerLowest, 255 },
	{ "imageBgTransparent", &M3SchemeRoles::surfaceContainer, 255 },
	{ "activeButtonBg", &M3SchemeRoles::primary, 255 },
	{ "activeButtonBgOver", &M3SchemeRoles::primary, 255 },
	{ "activeButtonBgRipple", &M3SchemeRoles::onPrimary, 40 },
	{ "activeButtonFg", &M3SchemeRoles::onPrimary, 255 },
	{ "activeButtonFgOver", &M3SchemeRoles::onPrimary, 255 },
	{ "activeButtonSecondaryFg", &M3SchemeRoles::onPrimary, 255 },
	{ "activeButtonSecondaryFgOver", &M3SchemeRoles::onPrimary, 255 },
	{ "activeLineFg", &M3SchemeRoles::primary, 255 },
	{ "activeLineFgError", &M3SchemeRoles::error, 255 },
	{ "lightButtonBg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "lightButtonBgOver", &M3SchemeRoles::secondaryContainer, 255 },
	{ "lightButtonBgRipple", &M3SchemeRoles::onSecondaryContainer, 40 },
	{ "lightButtonFg", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "lightButtonFgOver", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "attentionButtonFg", &M3SchemeRoles::onErrorContainer, 255 },
	{ "attentionButtonFgOver", &M3SchemeRoles::onErrorContainer, 255 },
	{ "attentionButtonBgOver", &M3SchemeRoles::errorContainer, 255 },
	{ "attentionButtonBgRipple", &M3SchemeRoles::onErrorContainer, 40 },
	{ "menuBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "menuBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "menuBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "menuIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "menuIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "menuSubmenuArrowFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "menuFgDisabled", &M3SchemeRoles::outline, 255 },
	{ "menuSeparatorFg", &M3SchemeRoles::outlineVariant, 255 },
	{ "scrollBarBg", &M3SchemeRoles::onSurfaceVariant, 83 },
	{ "scrollBarBgOver", &M3SchemeRoles::onSurfaceVariant, 122 },
	{ "scrollBg", &M3SchemeRoles::outlineVariant, 26 },
	{ "scrollBgOver", &M3SchemeRoles::outlineVariant, 44 },
	{ "smallCloseIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "smallCloseIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "radialFg", &M3SchemeRoles::primary, 255 },
	{ "radialBg", &M3SchemeRoles::scrim, 86 },
	{ "placeholderFg", &M3SchemeRoles::outline, 255 },
	{ "placeholderFgActive", &M3SchemeRoles::outline, 255 },
	{ "inputBorderFg", &M3SchemeRoles::outlineVariant, 255 },
	{ "filterInputBorderFg", &M3SchemeRoles::primary, 255 },
	{ "filterInputActiveBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "filterInputInactiveBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "checkboxFg", &M3SchemeRoles::outline, 255 },
	{ "botKbBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "botKbDownBg", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "botKbColor", &M3SchemeRoles::onSurface, 255 },
	{ "botKbPrimaryBg", &M3SchemeRoles::primaryContainer, 204 },
	{ "botKbDangerBg", &M3SchemeRoles::errorContainer, 204 },
	{ "botKbSuccessBg", &M3SchemeRoles::tertiaryContainer, 204 },
	{ "botKbInlinePrimaryBg", &M3SchemeRoles::primaryContainer, 179 },
	{ "botKbInlineDangerBg", &M3SchemeRoles::errorContainer, 179 },
	{ "botKbInlineSuccessBg", &M3SchemeRoles::tertiaryContainer, 179 },
	{ "sliderBgInactive", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "sliderBgActive", &M3SchemeRoles::primary, 255 },
	{ "tooltipBg", &M3SchemeRoles::inverseSurface, 255 },
	{ "tooltipFg", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "tooltipBorderFg", &M3SchemeRoles::outlineVariant, 255 },
	{ "titleShadow", &M3SchemeRoles::shadow, 3 },
	{ "titleBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "titleBgActive", &M3SchemeRoles::surfaceContainer, 255 },
	{ "titleButtonBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "titleButtonFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "titleButtonBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "titleButtonFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "titleButtonBgActive", &M3SchemeRoles::surfaceContainer, 255 },
	{ "titleButtonFgActive", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "titleButtonBgActiveOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "titleButtonFgActiveOver", &M3SchemeRoles::onSurface, 255 },
	{ "titleButtonCloseBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "titleButtonCloseFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "titleButtonCloseBgOver", &M3SchemeRoles::error, 255 },
	{ "titleButtonCloseFgOver", &M3SchemeRoles::onError, 255 },
	{ "titleButtonCloseBgActive", &M3SchemeRoles::surfaceContainer, 255 },
	{ "titleButtonCloseFgActive", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "titleButtonCloseBgActiveOver", &M3SchemeRoles::error, 255 },
	{ "titleButtonCloseFgActiveOver", &M3SchemeRoles::onError, 255 },
	{ "titleFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "titleFgActive", &M3SchemeRoles::onSurface, 255 },
	{ "trayCounterBg", &M3SchemeRoles::error, 255 },
	{ "trayCounterBgMute", &M3SchemeRoles::outline, 255 },
	{ "trayCounterFg", &M3SchemeRoles::onError, 255 },
	{ "trayCounterBgMacInvert", &M3SchemeRoles::onSurface, 255 },
	{ "trayCounterFgMacInvert", &M3SchemeRoles::surface, 1 },
	{ "layerBg", &M3SchemeRoles::scrim, 127 },
	{ "cancelIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "cancelIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "boxBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "boxTextFg", &M3SchemeRoles::onSurface, 255 },
	{ "boxTextFgGood", &M3SchemeRoles::tertiary, 255 },
	{ "boxTextFgError", &M3SchemeRoles::error, 255 },
	{ "boxTitleFg", &M3SchemeRoles::onSurface, 255 },
	{ "boxSearchBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "boxTitleAdditionalFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "boxTitleCloseFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "boxTitleCloseFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "boxDividerBg", &M3SchemeRoles::surface, 0 },
	{ "boxDividerFg", &M3SchemeRoles::surface, 0 },
	{ "paymentsTipActive", &M3SchemeRoles::primary, 255 },
	{ "membersAboutLimitFg", &M3SchemeRoles::error, 255 },
	{ "contactsBg", &M3SchemeRoles::surface, 255 },
	{ "contactsBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "contactsNameFg", &M3SchemeRoles::onSurface, 255 },
	{ "contactsStatusFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "contactsStatusFgOver", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "contactsStatusFgOnline", &M3SchemeRoles::primary, 255 },
	{ "photoCropFadeBg", &M3SchemeRoles::scrim, 127 },
	{ "photoCropPointFg", &M3SchemeRoles::onSurface, 127 },
	{ "callArrowFg", &M3SchemeRoles::primary, 255 },
	{ "callArrowMissedFg", &M3SchemeRoles::error, 255 },
	{ "introBg", &M3SchemeRoles::surface, 255 },
	{ "introTitleFg", &M3SchemeRoles::onSurface, 255 },
	{ "introDescriptionFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "introCoverTopBg", &M3SchemeRoles::primary, 255 },
	{ "introCoverBottomBg", &M3SchemeRoles::primaryContainer, 255 },
	{ "introCoverIconsFg", &M3SchemeRoles::onPrimary, 255 },
	{ "introCoverPlaneTrace", &M3SchemeRoles::onPrimary, 105 },
	{ "introCoverPlaneInner", &M3SchemeRoles::onPrimary, 255 },
	{ "introCoverPlaneOuter", &M3SchemeRoles::onPrimary, 255 },
	{ "introCoverPlaneTop", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsMenuIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsMenuIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "dialogsBg", &M3SchemeRoles::surfaceContainerLow, 255 },
	{ "dialogsNameFg", &M3SchemeRoles::onSurface, 255 },
	{ "dialogsChatIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsDateFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsTextFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsTextFgService", &M3SchemeRoles::primary, 255 },
	{ "dialogsDraftFg", &M3SchemeRoles::error, 255 },
	{ "dialogsVerifiedIconBg", &M3SchemeRoles::primary, 255 },
	{ "dialogsVerifiedIconFg", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsSendingIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsSentIconFg", &M3SchemeRoles::primary, 255 },
	{ "dialogsUnreadBg", &M3SchemeRoles::primary, 255 },
	{ "dialogsUnreadBgMuted", &M3SchemeRoles::secondaryContainer, 255 },
	{ "dialogsUnreadFg", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsArchiveFg", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsOnlineBadgeFg", &M3SchemeRoles::primary, 255 },
	{ "dialogsScamFg", &M3SchemeRoles::error, 255 },
	{ "dialogsBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "dialogsNameFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "dialogsChatIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "dialogsDateFgOver", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsTextFgOver", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsTextFgServiceOver", &M3SchemeRoles::primary, 255 },
	{ "dialogsDraftFgOver", &M3SchemeRoles::error, 255 },
	{ "dialogsVerifiedIconBgOver", &M3SchemeRoles::primary, 255 },
	{ "dialogsVerifiedIconFgOver", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsSendingIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "dialogsSentIconFgOver", &M3SchemeRoles::primary, 255 },
	{ "dialogsUnreadBgOver", &M3SchemeRoles::primary, 255 },
	{ "dialogsUnreadBgMutedOver", &M3SchemeRoles::secondaryContainer, 255 },
	{ "dialogsUnreadFgOver", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsArchiveFgOver", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsScamFgOver", &M3SchemeRoles::error, 255 },
	{ "dialogsBgActive", &M3SchemeRoles::secondaryContainer, 255 },
	{ "dialogsNameFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsChatIconFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsDateFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsTextFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsTextFgServiceActive", &M3SchemeRoles::primary, 255 },
	{ "dialogsDraftFgActive", &M3SchemeRoles::error, 255 },
	{ "dialogsVerifiedIconBgActive", &M3SchemeRoles::primary, 255 },
	{ "dialogsVerifiedIconFgActive", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsSendingIconFgActive", &M3SchemeRoles::onSecondaryContainer, 153 },
	{ "dialogsSentIconFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsUnreadBgActive", &M3SchemeRoles::primary, 255 },
	{ "dialogsUnreadBgMutedActive", &M3SchemeRoles::secondaryContainer, 255 },
	{ "dialogsUnreadFgActive", &M3SchemeRoles::onPrimary, 255 },
	{ "dialogsOnlineBadgeFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "dialogsScamFgActive", &M3SchemeRoles::error, 255 },
	{ "dialogsRippleBg", &M3SchemeRoles::onSurface, 30 },
	{ "dialogsRippleBgActive", &M3SchemeRoles::onSecondaryContainer, 30 },
	{ "searchedBarBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "searchedBarFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "searchedTextMatchBg", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "searchedTextMatchFg", &M3SchemeRoles::onTertiaryContainer, 255 },
	{ "searchedTextCurrentMatchBg", &M3SchemeRoles::primary, 255 },
	{ "searchedTextCurrentMatchFg", &M3SchemeRoles::onPrimary, 255 },
	{ "topBarBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "emojiPanBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "emojiPanCategories", &M3SchemeRoles::surfaceContainer, 255 },
	{ "emojiPanHeaderFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "emojiPanHeaderBg", &M3SchemeRoles::surfaceContainer, 242 },
	{ "emojiIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "emojiSubIconFgActive", &M3SchemeRoles::primary, 255 },
	{ "stickerPanDeleteBg", &M3SchemeRoles::error, 255 },
	{ "stickerPanDeleteFg", &M3SchemeRoles::onError, 255 },
	{ "stickerPreviewBg", &M3SchemeRoles::scrim, 176 },
	{ "stickerPanPremium1", &M3SchemeRoles::primary, 255 },
	{ "stickerPanPremium2", &M3SchemeRoles::tertiary, 255 },
	{ "historyTextInFg", &M3SchemeRoles::onSurface, 255 },
	{ "historyTextInFgSelected", &M3SchemeRoles::onSurface, 255 },
	{ "historyTextOutFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyTextOutFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyLinkInFg", &M3SchemeRoles::primary, 255 },
	{ "historyLinkInFgSelected", &M3SchemeRoles::primary, 255 },
	{ "historyLinkOutFg", &M3SchemeRoles::primary, 255 },
	{ "historyLinkOutFgSelected", &M3SchemeRoles::primary, 255 },
	{ "historyFileNameInFg", &M3SchemeRoles::onSurface, 255 },
	{ "historyFileNameInFgSelected", &M3SchemeRoles::onSurface, 255 },
	{ "historyFileNameOutFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyFileNameOutFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyOutIconFg", &M3SchemeRoles::primary, 255 },
	{ "historyOutIconFgSelected", &M3SchemeRoles::primary, 255 },
	{ "historyIconFgInverted", &M3SchemeRoles::fixedWhite, 255 },
	{ "historySendingOutIconFg", &M3SchemeRoles::primary, 255 },
	{ "historySendingInIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "historySendingInvertedIconFg", &M3SchemeRoles::fixedWhite, 200 },
	{ "historyCallArrowInFg", &M3SchemeRoles::primary, 255 },
	{ "historyCallArrowInFgSelected", &M3SchemeRoles::primary, 255 },
	{ "historyCallArrowMissedInFg", &M3SchemeRoles::error, 255 },
	{ "historyCallArrowMissedInFgSelected", &M3SchemeRoles::error, 255 },
	{ "historyCallArrowOutFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyCallArrowOutFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyUnreadBarBg", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "historyUnreadBarBorder", &M3SchemeRoles::outlineVariant, 24 },
	{ "historyUnreadBarFg", &M3SchemeRoles::primary, 255 },
	{ "historyForwardChooseBg", &M3SchemeRoles::primary, 76 },
	{ "historyForwardChooseFg", &M3SchemeRoles::onPrimary, 255 },
	{ "historyPeer1NameFg", &M3SchemeRoles::primary, 255 },
	{ "historyPeer1NameFgSelected", &M3SchemeRoles::primary, 255 },
	{ "historyPeer1UserpicBg", &M3SchemeRoles::primary, 255 },
	{ "historyPeer2NameFg", &M3SchemeRoles::secondary, 255 },
	{ "historyPeer2NameFgSelected", &M3SchemeRoles::secondary, 255 },
	{ "historyPeer2UserpicBg", &M3SchemeRoles::secondary, 255 },
	{ "historyPeer3NameFg", &M3SchemeRoles::tertiary, 255 },
	{ "historyPeer3NameFgSelected", &M3SchemeRoles::tertiary, 255 },
	{ "historyPeer3UserpicBg", &M3SchemeRoles::tertiary, 255 },
	{ "historyPeer4NameFg", &M3SchemeRoles::primaryContainer, 255 },
	{ "historyPeer4NameFgSelected", &M3SchemeRoles::primaryContainer, 255 },
	{ "historyPeer4UserpicBg", &M3SchemeRoles::primaryContainer, 255 },
	{ "historyPeer5NameFg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "historyPeer5NameFgSelected", &M3SchemeRoles::secondaryContainer, 255 },
	{ "historyPeer5UserpicBg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "historyPeer6NameFg", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "historyPeer6NameFgSelected", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "historyPeer6UserpicBg", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "historyPeer7NameFg", &M3SchemeRoles::error, 255 },
	{ "historyPeer7NameFgSelected", &M3SchemeRoles::error, 255 },
	{ "historyPeer7UserpicBg", &M3SchemeRoles::error, 255 },
	{ "historyPeer8NameFg", &M3SchemeRoles::outline, 255 },
	{ "historyPeer8NameFgSelected", &M3SchemeRoles::outline, 255 },
	{ "historyPeer8UserpicBg", &M3SchemeRoles::outline, 255 },
	{ "historyPeerUserpicFg", &M3SchemeRoles::onPrimary, 255 },
	{ "historyPeerSavedMessagesBg", &M3SchemeRoles::primary, 255 },
	{ "historyPeerArchiveUserpicBg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "historyPeer1UserpicBg2", &M3SchemeRoles::primary, 255 },
	{ "historyPeer2UserpicBg2", &M3SchemeRoles::secondary, 255 },
	{ "historyPeer3UserpicBg2", &M3SchemeRoles::tertiary, 255 },
	{ "historyPeer4UserpicBg2", &M3SchemeRoles::primaryContainer, 255 },
	{ "historyPeer5UserpicBg2", &M3SchemeRoles::secondaryContainer, 255 },
	{ "historyPeer6UserpicBg2", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "historyPeer7UserpicBg2", &M3SchemeRoles::error, 255 },
	{ "historyPeer8UserpicBg2", &M3SchemeRoles::outline, 255 },
	{ "historyPeerSavedMessagesBg2", &M3SchemeRoles::primary, 255 },
	{ "settingsIconBg1", &M3SchemeRoles::error, 255 },
	{ "settingsIconBg2", &M3SchemeRoles::secondary, 255 },
	{ "settingsIconBg3", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "settingsIconBg4", &M3SchemeRoles::primary, 255 },
	{ "settingsIconBg5", &M3SchemeRoles::primaryContainer, 255 },
	{ "settingsIconBg6", &M3SchemeRoles::secondaryContainer, 255 },
	{ "settingsIconBg8", &M3SchemeRoles::outline, 255 },
	{ "settingsIconBgArchive", &M3SchemeRoles::secondaryContainer, 255 },
	{ "settingsIconFg", &M3SchemeRoles::onPrimary, 255 },
	{ "historyScrollBarBg", &M3SchemeRoles::onSurfaceVariant, 122 },
	{ "historyScrollBarBgOver", &M3SchemeRoles::onSurfaceVariant, 188 },
	{ "historyScrollBg", &M3SchemeRoles::outlineVariant, 76 },
	{ "historyScrollBgOver", &M3SchemeRoles::outlineVariant, 107 },
	{ "msgInBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "msgInBgSelected", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "msgOutBg", &M3SchemeRoles::primaryContainer, 255 },
	{ "msgOutBgSelected", &M3SchemeRoles::primaryContainer, 255 },
	{ "msgSelectOverlay", &M3SchemeRoles::primary, 76 },
	{ "msgStickerOverlay", &M3SchemeRoles::scrim, 127 },
	{ "msgInServiceFg", &M3SchemeRoles::primary, 255 },
	{ "msgInServiceFgSelected", &M3SchemeRoles::primary, 255 },
	{ "msgOutServiceFg", &M3SchemeRoles::primary, 255 },
	{ "msgOutServiceFgSelected", &M3SchemeRoles::primary, 255 },
	{ "msgInShadow", &M3SchemeRoles::shadow, 41 },
	{ "msgInShadowSelected", &M3SchemeRoles::shadow, 41 },
	{ "msgOutShadow", &M3SchemeRoles::shadow, 29 },
	{ "msgOutShadowSelected", &M3SchemeRoles::shadow, 34 },
	{ "msgInDateFg", &M3SchemeRoles::onSurfaceVariant, 180 },
	{ "msgInDateFgSelected", &M3SchemeRoles::onSurfaceVariant, 180 },
	{ "msgOutDateFg", &M3SchemeRoles::onPrimaryContainer, 180 },
	{ "msgOutDateFgSelected", &M3SchemeRoles::onPrimaryContainer, 180 },
	{ "msgServiceFg", &M3SchemeRoles::onSurface, 255 },
	{ "msgServiceBg", &M3SchemeRoles::surfaceContainer, 127 },
	{ "msgServiceBgSelected", &M3SchemeRoles::surfaceContainerHigh, 162 },
	{ "msgInReplyBarColor", &M3SchemeRoles::primary, 255 },
	{ "msgInReplyBarSelColor", &M3SchemeRoles::primary, 255 },
	{ "msgOutReplyBarColor", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgOutReplyBarSelColor", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgImgReplyBarColor", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "msgInMonoFg", &M3SchemeRoles::onSurface, 255 },
	{ "msgOutMonoFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgInMonoFgSelected", &M3SchemeRoles::onSurface, 255 },
	{ "msgOutMonoFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgDateImgFg", &M3SchemeRoles::fixedWhite, 255 },
	{ "msgDateImgBg", &M3SchemeRoles::scrim, 140 },
	{ "msgDateImgBgOver", &M3SchemeRoles::scrim, 180 },
	{ "msgDateImgBgSelected", &M3SchemeRoles::scrim, 190 },
	{ "msgFileThumbLinkInFg", &M3SchemeRoles::primary, 255 },
	{ "msgFileThumbLinkInFgSelected", &M3SchemeRoles::primary, 255 },
	{ "msgFileThumbLinkOutFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgFileThumbLinkOutFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgFileInBg", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "msgFileInBgOver", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "msgFileInBgSelected", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "msgFileOutBg", &M3SchemeRoles::primary, 255 },
	{ "msgFileOutBgSelected", &M3SchemeRoles::primary, 255 },
	{ "msgFile1Bg", &M3SchemeRoles::primary, 255 },
	{ "msgFile1BgDark", &M3SchemeRoles::primary, 255 },
	{ "msgFile1BgOver", &M3SchemeRoles::primary, 255 },
	{ "msgFile1BgSelected", &M3SchemeRoles::primary, 255 },
	{ "msgFile2Bg", &M3SchemeRoles::secondary, 255 },
	{ "msgFile2BgDark", &M3SchemeRoles::secondary, 255 },
	{ "msgFile2BgOver", &M3SchemeRoles::secondary, 255 },
	{ "msgFile2BgSelected", &M3SchemeRoles::secondary, 255 },
	{ "msgFile3Bg", &M3SchemeRoles::tertiary, 255 },
	{ "msgFile3BgDark", &M3SchemeRoles::tertiary, 255 },
	{ "msgFile3BgOver", &M3SchemeRoles::tertiary, 255 },
	{ "msgFile3BgSelected", &M3SchemeRoles::tertiary, 255 },
	{ "msgFile4Bg", &M3SchemeRoles::error, 255 },
	{ "msgFile4BgDark", &M3SchemeRoles::error, 255 },
	{ "msgFile4BgOver", &M3SchemeRoles::error, 255 },
	{ "msgFile4BgSelected", &M3SchemeRoles::error, 255 },
	{ "historyFileInIconFg", &M3SchemeRoles::onSurface, 255 },
	{ "historyFileInIconFgSelected", &M3SchemeRoles::onSurface, 255 },
	{ "historyFileInRadialFg", &M3SchemeRoles::primary, 255 },
	{ "historyFileInRadialFgSelected", &M3SchemeRoles::primary, 255 },
	{ "historyFileOutIconFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyFileOutIconFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyFileOutRadialFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyFileOutRadialFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "historyFileThumbIconFg", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "historyFileThumbIconFgSelected", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "historyFileThumbRadialFg", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "historyFileThumbRadialFgSelected", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "historyVideoMessageProgressFg", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "msgWaveformInActive", &M3SchemeRoles::primary, 255 },
	{ "msgWaveformInActiveSelected", &M3SchemeRoles::primary, 255 },
	{ "msgWaveformInInactive", &M3SchemeRoles::outlineVariant, 255 },
	{ "msgWaveformInInactiveSelected", &M3SchemeRoles::outlineVariant, 255 },
	{ "msgWaveformOutActive", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgWaveformOutActiveSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "msgWaveformOutInactive", &M3SchemeRoles::outline, 255 },
	{ "msgWaveformOutInactiveSelected", &M3SchemeRoles::outline, 255 },
	{ "msgBotKbOverBgAdd", &M3SchemeRoles::onSurface, 32 },
	{ "msgBotKbIconFg", &M3SchemeRoles::onSurface, 255 },
	{ "msgBotKbRippleBg", &M3SchemeRoles::onSurface, 32 },
	{ "mediaInFg", &M3SchemeRoles::onSurface, 255 },
	{ "mediaInFgSelected", &M3SchemeRoles::onSurface, 255 },
	{ "mediaOutFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "mediaOutFgSelected", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "youtubePlayIconBg", &M3SchemeRoles::scrim, 200 },
	{ "youtubePlayIconFg", &M3SchemeRoles::fixedWhite, 255 },
	{ "videoPlayIconBg", &M3SchemeRoles::scrim, 127 },
	{ "videoPlayIconFg", &M3SchemeRoles::fixedWhite, 255 },
	{ "toastBg", &M3SchemeRoles::inverseSurface, 229 },
	{ "toastFg", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "historyToDownBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "historyToDownBgOver", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "historyToDownBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "historyToDownFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "historyToDownFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "historyToDownShadow", &M3SchemeRoles::shadow, 64 },
	{ "historyComposeAreaBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "historyComposeAreaFg", &M3SchemeRoles::onSurface, 255 },
	{ "historyComposeAreaFgService", &M3SchemeRoles::outline, 255 },
	{ "historyComposeIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "historyComposeIconFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "historySendIconFg", &M3SchemeRoles::primary, 255 },
	{ "historySendIconFgOver", &M3SchemeRoles::primary, 255 },
	{ "historyPinnedBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "historyReplyBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "historyReplyIconFg", &M3SchemeRoles::primary, 255 },
	{ "historyReplyCancelFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "historyReplyCancelFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "historyComposeButtonBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "historyComposeButtonBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "historyComposeButtonBgRipple", &M3SchemeRoles::onSurface, 40 },
	{ "mapPointDrop", &M3SchemeRoles::error, 255 },
	{ "mapPointDot", &M3SchemeRoles::onError, 255 },
	{ "overviewCheckBg", &M3SchemeRoles::scrim, 64 },
	{ "overviewCheckBgActive", &M3SchemeRoles::primary, 255 },
	{ "overviewCheckBorder", &M3SchemeRoles::surface, 255 },
	{ "overviewCheckFgActive", &M3SchemeRoles::onPrimary, 255 },
	{ "overviewPhotoSelectOverlay", &M3SchemeRoles::primary, 51 },
	{ "profileStatusFgOver", &M3SchemeRoles::primary, 255 },
	{ "profileVerifiedCheckBg", &M3SchemeRoles::primary, 255 },
	{ "profileVerifiedCheckFg", &M3SchemeRoles::onPrimary, 255 },
	{ "profileAdminStartFg", &M3SchemeRoles::primary, 255 },
	{ "notificationsBoxMonitorFg", &M3SchemeRoles::onSurface, 255 },
	{ "notificationsBoxScreenBg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "notificationSampleUserpicFg", &M3SchemeRoles::primary, 255 },
	{ "notificationSampleCloseFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "notificationSampleTextFg", &M3SchemeRoles::onSurface, 255 },
	{ "notificationSampleNameFg", &M3SchemeRoles::onSurface, 255 },
	{ "mainMenuBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "mainMenuCoverBg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "mainMenuCloudFg", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "mainMenuCloudBg", &M3SchemeRoles::secondaryContainer, 255 },
	{ "mediaPlayerBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "mediaPlayerActiveFg", &M3SchemeRoles::primary, 255 },
	{ "mediaPlayerInactiveFg", &M3SchemeRoles::outlineVariant, 255 },
	{ "mediaPlayerDisabledFg", &M3SchemeRoles::outline, 255 },
	{ "mediaviewFileBg", &M3SchemeRoles::scrim, 200 },
	{ "mediaviewFileNameFg", &M3SchemeRoles::fixedWhite, 230 },
	{ "mediaviewFileSizeFg", &M3SchemeRoles::fixedWhite, 178 },
	{ "mediaviewFileRedCornerFg", &M3SchemeRoles::error, 255 },
	{ "mediaviewFileYellowCornerFg", &M3SchemeRoles::tertiary, 255 },
	{ "mediaviewFileGreenCornerFg", &M3SchemeRoles::primary, 255 },
	{ "mediaviewFileBlueCornerFg", &M3SchemeRoles::secondary, 255 },
	{ "mediaviewFileExtFg", &M3SchemeRoles::onPrimary, 255 },
	{ "mediaviewMenuBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "mediaviewMenuBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "mediaviewMenuBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "mediaviewMenuFg", &M3SchemeRoles::onSurface, 255 },
	{ "mediaviewBg", &M3SchemeRoles::scrim, 128 },
	{ "mediaviewVideoBg", &M3SchemeRoles::fixedBlack, 255 },
	{ "mediaviewControlBg", &M3SchemeRoles::scrim, 166 },
	{ "mediaviewControlFg", &M3SchemeRoles::fixedWhite, 230 },
	{ "mediaviewCaptionBg", &M3SchemeRoles::scrim, 166 },
	{ "mediaviewCaptionFg", &M3SchemeRoles::fixedWhite, 230 },
	{ "mediaviewTextLinkFg", &M3SchemeRoles::primary, 255 },
	{ "mediaviewSaveMsgBg", &M3SchemeRoles::scrim, 166 },
	{ "mediaviewSaveMsgFg", &M3SchemeRoles::fixedWhite, 230 },
	{ "mediaviewPlaybackActive", &M3SchemeRoles::primary, 255 },
	{ "mediaviewPlaybackInactive", &M3SchemeRoles::fixedWhite, 76 },
	{ "mediaviewPlaybackActiveOver", &M3SchemeRoles::primary, 255 },
	{ "mediaviewPlaybackInactiveOver", &M3SchemeRoles::fixedWhite, 102 },
	{ "mediaviewPlaybackProgressFg", &M3SchemeRoles::fixedWhite, 230 },
	{ "mediaviewPlaybackIconFg", &M3SchemeRoles::fixedWhite, 230 },
	{ "mediaviewPlaybackIconFgOver", &M3SchemeRoles::fixedWhite, 255 },
	{ "mediaviewPlaybackIconRipple", &M3SchemeRoles::fixedWhite, 30 },
	{ "mediaviewPipControlsFg", &M3SchemeRoles::fixedWhite, 220 },
	{ "mediaviewPipControlsFgOver", &M3SchemeRoles::fixedWhite, 255 },
	{ "mediaviewPipPlaybackActive", &M3SchemeRoles::primary, 220 },
	{ "mediaviewPipPlaybackInactive", &M3SchemeRoles::fixedWhite, 76 },
	{ "mediaviewTransparentBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "mediaviewTransparentFg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "notificationBg", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "callBg", &M3SchemeRoles::surfaceContainerLowest, 242 },
	{ "callBgOpaque", &M3SchemeRoles::surfaceContainerLowest, 255 },
	{ "callBgButton", &M3SchemeRoles::surfaceContainerHigh, 127 },
	{ "callNameFg", &M3SchemeRoles::onSurface, 255 },
	{ "callStatusFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "callIconBg", &M3SchemeRoles::surfaceContainerHigh, 31 },
	{ "callIconFg", &M3SchemeRoles::onSurface, 255 },
	{ "callIconBgActive", &M3SchemeRoles::primary, 229 },
	{ "callIconFgActive", &M3SchemeRoles::onPrimary, 255 },
	{ "callIconActiveRipple", &M3SchemeRoles::onPrimary, 40 },
	{ "callAnswerBg", &M3SchemeRoles::primary, 255 },
	{ "callAnswerRipple", &M3SchemeRoles::onPrimary, 40 },
	{ "callAnswerBgOuter", &M3SchemeRoles::primary, 38 },
	{ "callHangupBg", &M3SchemeRoles::error, 255 },
	{ "callHangupRipple", &M3SchemeRoles::onError, 40 },
	{ "callMuteRipple", &M3SchemeRoles::onSurface, 18 },
	{ "groupCallBg", &M3SchemeRoles::surfaceContainerLowest, 255 },
	{ "groupCallActiveFg", &M3SchemeRoles::primary, 255 },
	{ "groupCallMembersBg", &M3SchemeRoles::surfaceContainerLow, 255 },
	{ "groupCallMembersBgOver", &M3SchemeRoles::surfaceContainer, 255 },
	{ "groupCallMembersBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "groupCallMembersFg", &M3SchemeRoles::onSurface, 255 },
	{ "groupCallMemberActiveIcon", &M3SchemeRoles::primary, 255 },
	{ "groupCallMemberActiveStatus", &M3SchemeRoles::primary, 255 },
	{ "groupCallMemberInactiveIcon", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "groupCallMemberInactiveStatus", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "groupCallMemberMutedIcon", &M3SchemeRoles::error, 255 },
	{ "groupCallMemberNotJoinedStatus", &M3SchemeRoles::outline, 255 },
	{ "groupCallIconFg", &M3SchemeRoles::onSurface, 255 },
	{ "groupCallLive1", &M3SchemeRoles::primary, 255 },
	{ "groupCallLive2", &M3SchemeRoles::secondary, 255 },
	{ "groupCallMuted1", &M3SchemeRoles::outline, 255 },
	{ "groupCallMuted2", &M3SchemeRoles::outlineVariant, 255 },
	{ "groupCallForceMutedBar1", &M3SchemeRoles::error, 255 },
	{ "groupCallForceMutedBar2", &M3SchemeRoles::errorContainer, 255 },
	{ "groupCallForceMutedBar3", &M3SchemeRoles::outline, 255 },
	{ "groupCallForceMuted1", &M3SchemeRoles::error, 255 },
	{ "groupCallForceMuted2", &M3SchemeRoles::errorContainer, 255 },
	{ "groupCallForceMuted3", &M3SchemeRoles::outline, 255 },
	{ "groupCallMenuBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "groupCallMenuBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "groupCallMenuBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "groupCallLeaveBg", &M3SchemeRoles::error, 127 },
	{ "groupCallLeaveBgRipple", &M3SchemeRoles::error, 158 },
	{ "groupCallVideoTextFg", &M3SchemeRoles::inverseOnSurface, 224 },
	{ "groupCallVideoSubTextFg", &M3SchemeRoles::inverseOnSurface, 192 },
	{ "callBarBg", &M3SchemeRoles::primaryContainer, 255 },
	{ "callBarMuteRipple", &M3SchemeRoles::onPrimaryContainer, 30 },
	{ "callBarBgMuted", &M3SchemeRoles::surfaceContainerHighest, 255 },
	{ "callBarFg", &M3SchemeRoles::onPrimaryContainer, 255 },
	{ "importantTooltipBg", &M3SchemeRoles::inverseSurface, 229 },
	{ "importantTooltipFg", &M3SchemeRoles::inverseOnSurface, 255 },
	{ "importantTooltipFgLink", &M3SchemeRoles::inversePrimary, 255 },
	{ "outdatedFg", &M3SchemeRoles::onErrorContainer, 255 },
	{ "outdateSoonBg", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "outdatedBg", &M3SchemeRoles::errorContainer, 255 },
	{ "spellUnderline", &M3SchemeRoles::error, 136 },
	{ "walletTitleBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletTitleBgActive", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletTitleButtonBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletTitleButtonFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "walletTitleButtonBgOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "walletTitleButtonFgOver", &M3SchemeRoles::onSurface, 255 },
	{ "walletTitleButtonBgActive", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletTitleButtonFgActive", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "walletTitleButtonBgActiveOver", &M3SchemeRoles::surfaceContainerHigh, 255 },
	{ "walletTitleButtonFgActiveOver", &M3SchemeRoles::onSurface, 255 },
	{ "walletTitleButtonCloseBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletTitleButtonCloseFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "walletTitleButtonCloseBgOver", &M3SchemeRoles::error, 255 },
	{ "walletTitleButtonCloseFgOver", &M3SchemeRoles::onError, 255 },
	{ "walletTitleButtonCloseBgActive", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletTitleButtonCloseFgActive", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "walletTitleButtonCloseBgActiveOver", &M3SchemeRoles::error, 255 },
	{ "walletTitleButtonCloseFgActiveOver", &M3SchemeRoles::onError, 255 },
	{ "walletTopBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "walletBalanceFg", &M3SchemeRoles::onSurface, 255 },
	{ "walletSubBalanceFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "walletTopLabelFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "walletTopIconFg", &M3SchemeRoles::primary, 255 },
	{ "walletTopIconRipple", &M3SchemeRoles::onSurface, 18 },
	{ "sideBarBg", &M3SchemeRoles::surfaceContainer, 255 },
	{ "sideBarBgActive", &M3SchemeRoles::secondaryContainer, 255 },
	{ "sideBarBgRipple", &M3SchemeRoles::onSurface, 30 },
	{ "sideBarTextFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "sideBarTextFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "sideBarIconFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "sideBarIconFgActive", &M3SchemeRoles::onSecondaryContainer, 255 },
	{ "sideBarBadgeBg", &M3SchemeRoles::primary, 255 },
	{ "sideBarBadgeBgActive", &M3SchemeRoles::primary, 255 },
	{ "sideBarBadgeBgMuted", &M3SchemeRoles::secondaryContainer, 255 },
	{ "sideBarBadgeBgMutedActive", &M3SchemeRoles::secondaryContainer, 255 },
	{ "sideBarBadgeFg", &M3SchemeRoles::onPrimary, 255 },
	{ "songCoverOverlayFg", &M3SchemeRoles::scrim, 102 },
	{ "photoEditorItemBaseHandleFg", &M3SchemeRoles::primary, 255 },
	{ "premiumButtonBg1", &M3SchemeRoles::tertiary, 255 },
	{ "premiumButtonBg2", &M3SchemeRoles::primary, 255 },
	{ "premiumButtonBg3", &M3SchemeRoles::secondary, 255 },
	{ "premiumButtonFg", &M3SchemeRoles::onPrimary, 255 },
	{ "premiumIconBg1", &M3SchemeRoles::tertiary, 255 },
	{ "premiumIconBg2", &M3SchemeRoles::primary, 255 },
	{ "premiumIconBg3", &M3SchemeRoles::secondary, 255 },
	{ "statisticsChartInactive", &M3SchemeRoles::onSurfaceVariant, 153 },
	{ "statisticsChartActive", &M3SchemeRoles::onSurface, 216 },
	{ "statisticsChartLineBlue", &M3SchemeRoles::primary, 255 },
	{ "statisticsChartLineGreen", &M3SchemeRoles::secondary, 255 },
	{ "statisticsChartLineRed", &M3SchemeRoles::error, 255 },
	{ "statisticsChartLineGolden", &M3SchemeRoles::tertiary, 255 },
	{ "statisticsChartLineLightblue", &M3SchemeRoles::primaryContainer, 255 },
	{ "statisticsChartLineLightgreen", &M3SchemeRoles::secondaryContainer, 255 },
	{ "statisticsChartLineOrange", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "statisticsChartLineIndigo", &M3SchemeRoles::outline, 255 },
	{ "statisticsChartLinePurple", &M3SchemeRoles::inversePrimary, 255 },
	{ "statisticsChartLineCyan", &M3SchemeRoles::outlineVariant, 255 },
	{ "creditsBg1", &M3SchemeRoles::tertiary, 255 },
	{ "creditsBg2", &M3SchemeRoles::primary, 255 },
	{ "creditsBg3", &M3SchemeRoles::secondary, 255 },
	{ "creditsFg", &M3SchemeRoles::onPrimary, 255 },
	{ "creditsStroke", &M3SchemeRoles::tertiaryContainer, 255 },
	{ "currencyFg", &M3SchemeRoles::tertiary, 255 },
	{ "rankAdminFg", &M3SchemeRoles::secondary, 255 },
	{ "rankOwnerFg", &M3SchemeRoles::primary, 255 },
	{ "rankUserFg", &M3SchemeRoles::onSurfaceVariant, 255 },
	{ "dialogsMentionIconFg", &M3SchemeRoles::primary, 255 },
	{ "dialogsReactionIconFg", &M3SchemeRoles::primary, 255 },
	{ "dialogsPollIconFg", &M3SchemeRoles::primary, 255 },
};

// Compile-time check that all keys are covered
static_assert(sizeof(kMappingTable) / sizeof(kMappingTable[0]) == 586, "Explicit M3 mapping must cover all 586 tokens!");

static M3SchemeRoles CurrentRoles;

const M3SchemeRoles &CurrentM3Roles() {
	if (!CurrentRoles.primary.isValid()) {
		CurrentRoles = GenerateM3SchemeFromSeed(GetM3SeedColor(), true);
	}
	return CurrentRoles;
}

void ApplyM3Scheme(const QColor &seed, bool isDark) {
	const auto &config = CurrentConfig();
	CurrentRoles = GenerateM3SchemeFromSeed(
		seed,
		isDark,
		config.m3ColorSchemeVariant,
		config.m3ContrastLevel);
	const auto &roles = CurrentRoles;

	for (const auto &entry : kMappingTable) {
		const auto base = roles.*(entry.role);
		const auto alpha = (entry.alpha == 255) ? base.alpha() : entry.alpha;
		style::main_palette::setColor(
			QLatin1String(entry.key),
			base.red(),
			base.green(),
			base.blue(),
			alpha);
	}

	style::internal::ResetIcons();
	QPixmapCache::clear();
	style::NotifyPaletteChanged();
	for (const auto widget : QApplication::allWidgets()) {
		widget->update();
	}
}

} // namespace PixelGram
