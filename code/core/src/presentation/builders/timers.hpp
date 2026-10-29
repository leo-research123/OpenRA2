#pragma once
#include "yrpp/GeneralStructures.h"
class ColorScheme;
namespace game {
struct PresentationContext;
int build_tactical_timer(PresentationContext&, int row, ColorScheme*, int seconds, const wchar_t*,
                          const LARGE_INTEGER*, const bool*);
void build_tactical_timers(PresentationContext&);
}
