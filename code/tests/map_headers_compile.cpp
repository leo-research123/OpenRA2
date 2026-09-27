// Regression: including the real map-root header must compile on native hosts.
// This does not claim that the still-incomplete Mouse lifecycle is usable.
#include "yrpp/MouseClass.h"
#include "yrpp/TacticalClass.h"
#include <type_traits>
static_assert(std::is_base_of_v<MapClass, MouseClass>);
static_assert(std::is_base_of_v<RadarClass, MouseClass>);
static_assert(std::is_base_of_v<SidebarClass, MouseClass>);
