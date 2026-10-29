#include "debug/debug_state.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/WarheadTypeClass.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

namespace game {
static DebugState* active_debug_state() noexcept {
    auto* world = current_map_world();
    return world ? &world->impl->view.debug : nullptr;
}
bool debug_fog_hidden() noexcept {
    auto* state = active_debug_state();
    return state && !state->fog_visible;
}
bool debug_radar_forced() noexcept {
    auto* state = active_debug_state();
    return state && state->radar_forced;
}
bool debug_selection_expanded() noexcept {
    auto* state = active_debug_state();
    return state && state->selection > 1;
}
static bool debug_house_allowed(const HouseClass* house, int mode) noexcept {
    return house && (house->IsControlledByCurrentPlayer() || (mode > 1 && (mode == 3 || !house->IsNeutral())));
}
bool debug_can_select_house(const HouseClass* house) noexcept {
    auto* state = active_debug_state();
    return debug_house_allowed(house, state ? state->selection : 1);
}
bool debug_can_control_house(const HouseClass* house) noexcept {
    auto* state = active_debug_state();
    return debug_house_allowed(house, state ? state->control : 1);
}
bool debug_accept_player_order(const HouseClass* house) noexcept {
    // Default selection keeps original dispatch and its virtual eligibility
    // checks. Expanded groups need an explicit per-actor permission gate.
    return !debug_selection_expanded() || debug_can_control_house(house);
}
bool debug_can_mix_selection(const ObjectClass& a, const ObjectClass& b) noexcept {
    return debug_selection_expanded() && a.WhatAmI() != AbstractType::Building &&
           b.WhatAmI() != AbstractType::Building && debug_can_select_house(a.GetOwningHouse()) &&
           debug_can_select_house(b.GetOwningHouse());
}
bool debug_player_location_shrouded(const CoordStruct& at) noexcept {
    return !debug_fog_hidden() && MapClass::Instance.IsLocationShrouded(at);
}
bool debug_player_location_fogged(const CoordStruct& at) noexcept {
    return !debug_fog_hidden() && MapClass::Instance.IsLocationFogged(at);
}
bool query_debug_attack_damage(const ObjectClass* object, int original, const WarheadTypeClass* warhead,
                               int& output) noexcept {
    if (!object || original <= 0) return false;
    if (warhead && (warhead->MindControl || warhead->Temporal || warhead->IsLocomotor || warhead->Parasite ||
                    warhead->Psychedelic || warhead->IvanBomb))
        return false;
    auto* state = active_debug_state();
    if (!state) return false;
    const auto entry = state->attack.find(static_cast<std::uint32_t>(object->UniqueID));
    if (entry == state->attack.end()) return false;
    output = entry->second;
    return true;
}
int debug_attack_damage(const ObjectClass* object, int original, const WarheadTypeClass* warhead) noexcept {
    int output = original;
    query_debug_attack_damage(object, original, warhead, output);
    return output;
}
void forget_debug_object(const ObjectClass* object) noexcept {
    if (auto* state = active_debug_state(); state && object) {
        state->attack.erase(static_cast<std::uint32_t>(object->UniqueID));
        state->mind_control_immune.erase(static_cast<std::uint32_t>(object->UniqueID));
    }
}
bool debug_mind_control_immune(const TechnoClass* object) noexcept {
    const auto* state = active_debug_state();
    return state && object && state->mind_control_immune.contains(static_cast<std::uint32_t>(object->UniqueID));
}
bool set_debug_mind_control_allowed(MapWorld& world, ObjectClass& object, bool allowed) {
    if (object.IsDead() || (object.AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) return false;
    auto& state = world.impl->view.debug;
    const auto id = static_cast<std::uint32_t>(object.UniqueID);
    if (allowed)
        state.mind_control_immune.erase(id);
    else if (!static_cast<TechnoClass&>(object).GetTechnoType()->ImmuneToPsionics)
        state.mind_control_immune.insert(id);
    return true;
}
bool set_debug_god_overrides(MapWorld& world, ObjectClass& object) {
    auto& state = world.impl->view.debug;
    const auto id = static_cast<std::uint32_t>(object.UniqueID);
    const bool was_immune = state.mind_control_immune.contains(id);
    if (!set_debug_mind_control_allowed(world, object, false)) return false;
    // Both allocating overrides must succeed before health or rank changes.
    // Failed insertion preserves the attack map; erase only our new immunity.
    try {
        if (set_debug_attack(world, object, 1000000)) return true;
    } catch (...) {
        if (!was_immune) state.mind_control_immune.erase(id);
        throw;
    }
    if (!was_immune) state.mind_control_immune.erase(id);
    return false;
}
bool set_debug_attack(MapWorld& world, ObjectClass& object, int damage) {
    if (object.IsDead() || (object.AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) return false;
    auto& state = world.impl->view.debug;
    const auto id = static_cast<std::uint32_t>(object.UniqueID);
    if (damage < 0)
        state.attack.erase(id);
    else
        state.attack.insert_or_assign(id, damage);
    return true;
}
bool set_debug_xp(ObjectClass& object, double xp) noexcept {
    if (object.IsDead() || (object.AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None ||
        !RulesClass::Instance || !std::isfinite(xp) || xp < 0 || xp > RulesClass::Instance->VeteranCap)
        return false;
    static_cast<TechnoClass&>(object).Veterancy.Veterancy = static_cast<float>(xp);
    object.NeedsRedraw = true;
    map_object_changed();
    return true;
}
void set_debug_selection(MapWorld& world, int mode) {
    world.impl->view.debug.selection = mode;
    // Keep single-object inspection, but discard groups no longer allowed.
    auto& selected = ObjectClass::CurrentObjects;
    for (int i = selected.Count - 1; selected.Count > 1 && i >= 0; --i) {
        auto* object = selected[i];
        if (object->WhatAmI() == AbstractType::Building || !debug_can_select_house(object->GetOwningHouse()))
            object->Deselect();
    }
    map_object_changed();
}
bool debug_teleport_active() noexcept {
    auto* state = active_debug_state();
    return state && state->teleport;
}

double debug_performance_time() noexcept {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void DebugPerfSeries::add(double time, double value) noexcept {
    if (!std::isfinite(time) || !std::isfinite(value) || value < 0) return;
    samples[cursor] = {time, value};
    cursor = (cursor + 1) % capacity;
    count = std::min(count + 1, capacity);
}
struct DebugPerfSummary {
    unsigned count = 0;
    double sum = 0, average = 0, p95 = 0;
};
static DebugPerfSummary summarize(const DebugPerfSeries& series, double now) noexcept {
    DebugPerfSummary result;
    double values[DebugPerfSeries::capacity];
    for (unsigned i = 0; i < series.count; ++i) {
        const auto& sample = series.samples[i];
        if (sample.time > now || sample.time < now - 2.0) continue;
        values[result.count++] = sample.value;
        result.sum += sample.value;
    }
    if (result.count) {
        result.average = result.sum / result.count;
        std::sort(values, values + result.count);
        result.p95 = values[(result.count * 95 + 99) / 100 - 1];
    }
    return result;
}
void format_debug_performance(MapWorld& world, char* output, std::uint32_t capacity) noexcept {
    if (!output || !capacity) return;
    const auto& view = world.impl->view;
    const auto& performance = view.debug.performance;
    const double now = debug_performance_time();
    const auto frames = summarize(performance.frames, now), steps = summarize(performance.steps, now);
    const auto host = summarize(performance.host_frames, now), limited = summarize(performance.limited, now);
    const auto logic = summarize(performance.logic, now), drawing = summarize(performance.drawing, now);
    const auto render = summarize(performance.host_render, now);
    unsigned units = 0, buildings = 0;
    for (auto* object : TechnoClass::Array)
        if (object->IsAlive && object->Health > 0 && !object->InLimbo) {
            if (object->WhatAmI() == AbstractType::Building)
                ++buildings;
            else
                ++units;
        }
    const bool paused = view.loop.host_paused || view.scenario.unknown_62C != 0 || Game::SpecialDialog == 1;
    char target[48];
    const int rate = game_loop_target_rate(view.loop);
    if (view.loop.speed_level == 9)
        std::snprintf(target, sizeof(target), "unlimited");
    else if (rate)
        std::snprintf(target, sizeof(target), "%d FPS", rate);
    else
        std::snprintf(target, sizeof(target), "original timed speed");
    char fps[48];
    if (host.count && host.sum > 0)
        std::snprintf(fps, sizeof(fps), "%.1f FPS", host.count / host.sum);
    else
        std::snprintf(fps, sizeof(fps), "采样中");
    std::snprintf(output, capacity,
                  "PERF · %s · 最近 2 秒\n画面 %s · 逻辑 %.1f FPS\n"
                  "CPU ms (avg / P95)\n逻辑 %.2f / %.2f · 绘制 %.2f / %.2f\n"
                  "提交 %.2f / %.2f\ngamespeed %d · 目标 %s · 受限 %.1f%%\n单位 %u · 建筑 %u",
                  paused ? "暂停" : "运行", fps, frames.sum > 0 ? steps.sum / frames.sum : 0, logic.average, logic.p95,
                  drawing.average, drawing.p95, render.average, render.p95, view.loop.speed_level, target,
                  limited.count ? limited.average * 100 : 0, units, buildings);
}

bool get_debug_performance(MapViewHandle& view, std::uint64_t expected_world,
                           DebugPerformanceSnapshot& output) noexcept {
    if (view.operation_depth || !view.world || view.state != MapViewState::ready || !expected_world ||
        view.world->impl->world_id != expected_world)
        return false;
    struct Query {
        MapViewHandle& view;
        DebugPerformanceSnapshot snapshot;
    } query{view, {}};
    query.snapshot.visible = view.debug.performance_visible;
    if (query.snapshot.visible && !with_map_view(
                                      view,
                                      [](void* pointer) {
                                          auto& query = *static_cast<Query*>(pointer);
                                          format_debug_performance(*query.view.world, query.snapshot.message,
                                                                   sizeof(query.snapshot.message));
                                      },
                                      &query))
        return false;
    output = query.snapshot;
    return true;
}

bool get_debug_command_feedback(MapViewHandle& view, std::uint64_t request, DebugCommandFeedback& output) noexcept {
    if (view.operation_depth || !view.world || !request || view.debug.feedback.request != request) return false;
    output = view.debug.feedback;
    return true;
}
bool get_debug_modes(const MapViewHandle& view, DebugModes& output) noexcept {
    if (view.operation_depth || !view.world || view.state != MapViewState::ready) return false;
    output = {view.debug.fog_visible, view.debug.teleport, view.debug.selection, view.debug.control};
    return true;
}
bool record_debug_host_performance(MapViewHandle& view, std::uint64_t world,
                                   const DebugHostPerformance& sample) noexcept {
    if (view.operation_depth || !view.world || view.world->impl->world_id != world ||
        !std::isfinite(sample.frame_seconds) || sample.frame_seconds < 0 || !std::isfinite(sample.render_cpu_ms) ||
        sample.render_cpu_ms < 0)
        return false;
    auto& perf = view.debug.performance;
    const auto now = debug_performance_time();
    if (sample.frame_seconds > 0) perf.host_frames.add(now, sample.frame_seconds);
    if (sample.sequence && sample.sequence > perf.host_sequence) {
        perf.host_sequence = sample.sequence;
        perf.host_render.add(now, sample.render_cpu_ms);
    }
    return true;
}
}
