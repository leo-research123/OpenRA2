#pragma once
#include "yrpp/GeneralStructures.h"
#include <cstdint>
class ObjectClass;
class TechnoClass;
class AbstractClass;

namespace game {
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
inline constexpr bool nearest_hunt_policy = false;
#else
inline constexpr bool nearest_hunt_policy = true;
#endif
// Squared world distance avoids the threat score's distant-target floor.
inline double hunt_distance_squared(const CoordStruct& a, const CoordStruct& b) noexcept {
    const double x = double(a.X) - b.X, y = double(a.Y) - b.Y, z = double(a.Z) - b.Z;
    return x * x + y * y + z * z;
}
// Private, disposable cache of TechnoClass::Array. Failure disables acceleration;
// it never changes the original registry, class layout or target evaluation.
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
inline void techno_spatial_register(TechnoClass*) noexcept {}
inline void techno_spatial_forget(TechnoClass*) noexcept {}
inline void techno_spatial_dirty(ObjectClass*) noexcept {}
inline bool try_spatial_ground_threat(TechnoClass&, unsigned, int, int, const CoordStruct*, bool, AbstractClass*&,
                                      int&) noexcept {
    return false;
}
#else
void techno_spatial_register(TechnoClass*) noexcept;
void techno_spatial_forget(TechnoClass*) noexcept;
void techno_spatial_dirty(ObjectClass*) noexcept;
bool try_spatial_ground_threat(TechnoClass&, unsigned flags, int mask, int zone, const CoordStruct*, bool only_enemy,
                               AbstractClass*& best, int& highest) noexcept;
#endif

struct TechnoSpatialCounters {
    std::uint64_t queries = 0, accelerated = 0, fallback = 0;
    std::uint64_t candidates = 0, evaluations = 0, buckets = 0, floor_ties = 0;
};
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
inline void read_techno_spatial_counters(TechnoSpatialCounters& out) noexcept { out = {}; }
inline void reset_techno_spatial_counters() noexcept {}
#else
void read_techno_spatial_counters(TechnoSpatialCounters&) noexcept;
void reset_techno_spatial_counters() noexcept;
#endif
}
