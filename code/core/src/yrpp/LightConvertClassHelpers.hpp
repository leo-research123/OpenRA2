#pragma once
#include "yrpp/CellClass.h"
#include "yrpp/ConvertClass.h"
namespace game {
unsigned build_light_palette(const BytePalette*, WORD*, int, int, int, int, const BYTE*, int);
unsigned build_light_palette_row(const BytePalette*, WORD*, int, int, int, int, const BYTE*, int, bool) noexcept;
// LightConvertClass::BuildColorTable's channel multiplier for one tint channel.
int light_palette_factor(int channel) noexcept;
// A global effect (ScenarioClass::RecalcLighting) rebuilds every converter's
// table from its tint, keeping the shade count; the cell's own RGB stays.
// Gives the table inputs of the cell's converter while it holds such a tint;
// false when the cell has no converter or its table uses the base color.
inline bool cell_converter_tint(const CellClass& cell, int& red, int& green, int& blue, int& shades) noexcept {
    const auto* convert = cell.LightConvert;
    if (!convert || !convert->Tinted) return false;
    red = convert->Color2.Red;
    green = convert->Color2.Green;
    blue = convert->Color2.Blue;
    shades = convert->ShadeCount;
    return true;
}
}
