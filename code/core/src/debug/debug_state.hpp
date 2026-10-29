#pragma once

#include "api/debug_commands.hpp"
#include "yrpp/GeneralStructures.h"
#include <cstdint>
#include <map>
#include <set>

class ObjectClass;
class HouseClass;
class TechnoClass;
class FootClass;
class WarheadTypeClass;
namespace game {
class MapWorld;

struct DebugPerfSample {
    double time = 0, value = 0;
};
struct DebugPerfSeries {
    static constexpr unsigned capacity = 4096;
    DebugPerfSample samples[capacity]{};
    unsigned cursor = 0, count = 0;
    void add(double time, double value) noexcept;
};
struct DebugPerformance {
    DebugPerfSeries logic, drawing, frames, steps, limited, host_frames, host_render;
    std::uint64_t host_sequence = 0;
};
struct DebugState {
    bool fog_visible = true, teleport = false, performance_visible = false, radar_forced = false;
    int selection = 1, control = 1;
    int original_speed = 0;
    std::map<std::uint32_t, int> attack;
    std::set<std::uint32_t> mind_control_immune;
    DebugCommandFeedback feedback{};
    DebugPerformance performance;
};

// Private standalone services. Original class layout and ownership stay intact.
bool debug_fog_hidden() noexcept;
bool debug_radar_forced() noexcept;
bool debug_selection_expanded() noexcept;
bool debug_can_select_house(const HouseClass*) noexcept;
bool debug_can_control_house(const HouseClass*) noexcept;
bool debug_accept_player_order(const HouseClass*) noexcept;
bool debug_can_mix_selection(const ObjectClass&, const ObjectClass&) noexcept;
bool debug_player_location_shrouded(const CoordStruct&) noexcept;
bool debug_player_location_fogged(const CoordStruct&) noexcept;
int debug_attack_damage(const ObjectClass*, int original, const WarheadTypeClass* warhead = nullptr) noexcept;
bool query_debug_attack_damage(const ObjectClass*, int original, const WarheadTypeClass*, int& output) noexcept;
void forget_debug_object(const ObjectClass*) noexcept;
bool set_debug_attack(MapWorld&, ObjectClass&, int damage);
bool debug_mind_control_immune(const TechnoClass*) noexcept;
bool set_debug_mind_control_allowed(MapWorld&, ObjectClass&, bool allowed);
bool set_debug_god_overrides(MapWorld&, ObjectClass&);
bool set_debug_xp(ObjectClass&, double xp) noexcept;
void set_debug_selection(MapWorld&, int mode);
bool debug_teleport_active() noexcept;
bool debug_teleport_selected(CellStruct destination) noexcept;
DebugTargetFailure debug_teleport_unit(FootClass&, CoordStruct destination);

double debug_performance_time() noexcept;
void format_debug_performance(MapWorld&, char* output, std::uint32_t capacity) noexcept;
}
