# -*- coding: utf-8 -*-
import sys
sys.path.append("G:/TBuild/tdesktop/Telegram/SourceFiles/ui")
import gen_explicit_all

with open("G:/TBuild/tdesktop/Telegram/SourceFiles/ui/all_keys.txt", "r", encoding="utf-8") as f:
    all_keys = [line.strip().split("\t")[0] for line in f if line.strip()]

unmapped = [k for k in all_keys if k not in gen_explicit_all.EXPLICIT_MAP]

unmapped_path = "G:/TBuild/tdesktop/Telegram/SourceFiles/ui/m3_unmapped.txt"
with open(unmapped_path, "w", encoding="utf-8") as f:
    if unmapped:
        for k in unmapped:
            f.write(f"{k}\n")

if unmapped:
    print(f"ERROR: {len(unmapped)} unmapped keys written to m3_unmapped.txt!")
    sys.exit(1)
else:
    print("SUCCESS: 0 unmapped keys! m3_unmapped.txt is empty (0 bytes).")

# 1. Generate m3_mapping.tsv
tsv_path = "G:/TBuild/tdesktop/Telegram/SourceFiles/ui/m3_mapping.tsv"
with open(tsv_path, "w", encoding="utf-8") as f:
    f.write("Key\tM3Role\tAlpha\tDescription\n")
    for k in all_keys:
        role, alpha, desc = gen_explicit_all.EXPLICIT_MAP[k]
        f.write(f"{k}\t{role}\t{alpha}\t{desc}\n")

print(f"Wrote {tsv_path} ({len(all_keys)} entries)")

# 2. Generate m3_mapping.md
md_path = "G:/TBuild/tdesktop/Telegram/SourceFiles/ui/m3_mapping.md"
with open(md_path, "w", encoding="utf-8") as f:
    f.write("# Explicit Material You 3 (M3) Token Mapping Table\n\n")
    f.write(f"Total tokens mapped: {len(all_keys)}\n")
    f.write("Fallback count: 0 (All keys mapped with explicit semantic roles)\n\n")
    f.write("| Key | M3 Role | Alpha | Description |\n")
    f.write("| :--- | :--- | :--- | :--- |\n")
    for k in all_keys:
        role, alpha, desc = gen_explicit_all.EXPLICIT_MAP[k]
        f.write(f"| `{k}` | `{role}` | `{alpha}` | {desc} |\n")

print(f"Wrote {md_path}")

# 3. Update pixelgram_m3_scheme.cpp
cpp_path = "G:/TBuild/tdesktop/Telegram/SourceFiles/ui/pixelgram_m3_scheme.cpp"

cpp_table_entries = []
for k in all_keys:
    role, alpha, desc = gen_explicit_all.EXPLICIT_MAP[k]
    cpp_table_entries.append(f'\t{{ "{k}", &M3SchemeRoles::{role}, {alpha} }},')

table_code = "\n".join(cpp_table_entries)

cpp_content = f'''#include "ui/pixelgram_m3_scheme.h"
#include "settings/pixelgram_settings.h"
#include "ui/style/style_core_palette.h"
#include "ui/style/style_core_icon.h"
#include "ui/style/style_core.h"

#include "cpp/cam/hct.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"

#include <QtCore/QFile>

namespace PixelGram {{

inline QColor ArgbToQColor(material_color_utilities::Argb argb) {{
\treturn QColor(
\t\t(argb >> 16) & 0xFF,
\t\t(argb >> 8) & 0xFF,
\t\targb & 0xFF,
\t\t(argb >> 24) & 0xFF);
}}

inline material_color_utilities::Argb QColorToArgb(const QColor &c) {{
\treturn ((c.alpha() & 0xFF) << 24)
\t\t| ((c.red() & 0xFF) << 16)
\t\t| ((c.green() & 0xFF) << 8)
\t\t| (c.blue() & 0xFF);
}}

M3SchemeRoles GenerateM3SchemeFromSeed(
\t\tconst QColor &seed,
\t\tbool isDark,
\t\tint variant,
\t\tdouble contrast) {{
\tusing namespace material_color_utilities;

\tconst auto argb = QColorToArgb(seed);
\tHct sourceHct(argb);

\tstd::unique_ptr<DynamicScheme> scheme;
\tswitch (variant) {{
\tcase 1:
\t\tscheme = std::make_unique<SchemeVibrant>(sourceHct, isDark, contrast);
\t\tbreak;
\tcase 2:
\t\tscheme = std::make_unique<SchemeExpressive>(sourceHct, isDark, contrast);
\t\tbreak;
\tcase 3:
\t\tscheme = std::make_unique<SchemeNeutral>(sourceHct, isDark, contrast);
\t\tbreak;
\tcase 4:
\t\tscheme = std::make_unique<SchemeMonochrome>(sourceHct, isDark, contrast);
\t\tbreak;
\tdefault:
\t\tscheme = std::make_unique<SchemeTonalSpot>(sourceHct, isDark, contrast);
\t\tbreak;
\t}}

\tM3SchemeRoles roles;
\troles.primary = ArgbToQColor(MaterialDynamicColors::Primary().GetArgb(*scheme));
\troles.onPrimary = ArgbToQColor(MaterialDynamicColors::OnPrimary().GetArgb(*scheme));
\troles.primaryContainer = ArgbToQColor(MaterialDynamicColors::PrimaryContainer().GetArgb(*scheme));
\troles.onPrimaryContainer = ArgbToQColor(MaterialDynamicColors::OnPrimaryContainer().GetArgb(*scheme));
\troles.secondary = ArgbToQColor(MaterialDynamicColors::Secondary().GetArgb(*scheme));
\troles.onSecondary = ArgbToQColor(MaterialDynamicColors::OnSecondary().GetArgb(*scheme));
\troles.secondaryContainer = isDark ? ArgbToQColor(scheme->secondary_palette.get(42.0)) : ArgbToQColor(MaterialDynamicColors::SecondaryContainer().GetArgb(*scheme));
\troles.onSecondaryContainer = isDark ? ArgbToQColor(scheme->secondary_palette.get(92.0)) : ArgbToQColor(MaterialDynamicColors::OnSecondaryContainer().GetArgb(*scheme));
\troles.tertiary = ArgbToQColor(MaterialDynamicColors::Tertiary().GetArgb(*scheme));
\troles.onTertiary = ArgbToQColor(MaterialDynamicColors::OnTertiary().GetArgb(*scheme));
\troles.tertiaryContainer = ArgbToQColor(MaterialDynamicColors::TertiaryContainer().GetArgb(*scheme));
\troles.onTertiaryContainer = ArgbToQColor(MaterialDynamicColors::OnTertiaryContainer().GetArgb(*scheme));
\troles.error = ArgbToQColor(MaterialDynamicColors::Error().GetArgb(*scheme));
\troles.onError = ArgbToQColor(MaterialDynamicColors::OnError().GetArgb(*scheme));
\troles.errorContainer = ArgbToQColor(MaterialDynamicColors::ErrorContainer().GetArgb(*scheme));
\troles.onErrorContainer = ArgbToQColor(MaterialDynamicColors::OnErrorContainer().GetArgb(*scheme));
\troles.surface = ArgbToQColor(MaterialDynamicColors::Surface().GetArgb(*scheme));
\troles.surfaceDim = ArgbToQColor(MaterialDynamicColors::SurfaceDim().GetArgb(*scheme));
\troles.surfaceBright = ArgbToQColor(MaterialDynamicColors::SurfaceBright().GetArgb(*scheme));
\troles.surfaceContainerLowest = ArgbToQColor(MaterialDynamicColors::SurfaceContainerLowest().GetArgb(*scheme));
\troles.surfaceContainerLow = ArgbToQColor(MaterialDynamicColors::SurfaceContainerLow().GetArgb(*scheme));
\troles.surfaceContainer = ArgbToQColor(MaterialDynamicColors::SurfaceContainer().GetArgb(*scheme));
\troles.surfaceContainerHigh = ArgbToQColor(MaterialDynamicColors::SurfaceContainerHigh().GetArgb(*scheme));
\troles.surfaceContainerHighest = ArgbToQColor(MaterialDynamicColors::SurfaceContainerHighest().GetArgb(*scheme));
\troles.onSurface = ArgbToQColor(MaterialDynamicColors::OnSurface().GetArgb(*scheme));
\troles.onSurfaceVariant = ArgbToQColor(MaterialDynamicColors::OnSurfaceVariant().GetArgb(*scheme));
\troles.outline = ArgbToQColor(MaterialDynamicColors::Outline().GetArgb(*scheme));
\troles.outlineVariant = ArgbToQColor(MaterialDynamicColors::OutlineVariant().GetArgb(*scheme));
\troles.shadow = ArgbToQColor(MaterialDynamicColors::Shadow().GetArgb(*scheme));
\troles.scrim = ArgbToQColor(MaterialDynamicColors::Scrim().GetArgb(*scheme));
\troles.inverseSurface = ArgbToQColor(MaterialDynamicColors::InverseSurface().GetArgb(*scheme));
\troles.inverseOnSurface = ArgbToQColor(MaterialDynamicColors::InverseOnSurface().GetArgb(*scheme));
\troles.inversePrimary = ArgbToQColor(MaterialDynamicColors::InversePrimary().GetArgb(*scheme));

\treturn roles;
}}

QColor GetM3SeedColor() {{
\tconst auto &config = CurrentConfig();
\treturn QColor::fromRgba(config.m3SeedColor);
}}

void SetM3SeedColor(const QColor &color) {{
\tauto &config = CurrentConfig();
\tconfig.m3SeedColor = color.rgba();
\tSaveConfig();
}}

struct MappingEntry {{
\tconst char *key;
\tQColor (M3SchemeRoles::*role);
\tuint8 alpha;
}};

// Explicit table mapping all 586 palette keys to M3 roles with 0 fallback
static const MappingEntry kMappingTable[] = {{
{table_code}
}};

// Compile-time check that all keys are covered
static_assert(sizeof(kMappingTable) / sizeof(kMappingTable[0]) == 586, "Explicit M3 mapping must cover all 586 tokens!");

static M3SchemeRoles CurrentRoles;

const M3SchemeRoles &CurrentM3Roles() {{
	if (!CurrentRoles.primary.isValid()) {{
		CurrentRoles = GenerateM3SchemeFromSeed(GetM3SeedColor(), true);
	}}
	return CurrentRoles;
}}

void ApplyM3Scheme(const QColor &seed, bool isDark) {{
\tconst auto &config = CurrentConfig();
\tCurrentRoles = GenerateM3SchemeFromSeed(
\t\tseed,
\t\tisDark,
\t\tconfig.m3SchemeVariant,
\t\tconfig.m3ContrastLevel);
\tconst auto &roles = CurrentRoles;

\tfor (const auto &entry : kMappingTable) {{
\t\tconst auto base = roles.*(entry.role);
\t\tconst auto alpha = (entry.alpha == 255) ? base.alpha() : entry.alpha;
\t\tstyle::main_palette::setColor(
\t\t\tQLatin1String(entry.key),
\t\t\tbase.red(),
\t\t\tbase.green(),
\t\t\tbase.blue(),
\t\t\talpha);
\t}}

\tstyle::internal::ResetIcons();
\tstyle::NotifyPaletteChanged();
}}

}} // namespace PixelGram
'''

with open(cpp_path, "w", encoding="utf-8") as f:
    f.write(cpp_content)

print(f"Wrote {cpp_path} with {len(all_keys)} explicit mapping entries and static_assert(586)!")
