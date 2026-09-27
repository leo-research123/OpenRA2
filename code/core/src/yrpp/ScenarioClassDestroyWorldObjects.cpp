/*
 * Object teardown follows EA REDALERT/SCENARIO.CPP::Clear_Scenario, revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc.; GPL-3.0-or-later, third_party/ea/LICENSE.TXT. YR 534450..5349B1
 * supplies the target index, collection order and deferred-expiration passes.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/AbstractClass.h"
#include "scenario_runtime.hpp"
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace {
int add(int value, unsigned amount) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + amount);
}
struct DepthScope {
    int& value;
    bool active = true;
    explicit DepthScope(int& depth) : value(depth) { value = add(value, 1); }
    int finish() { if (active) { value = add(value, 0xffffffffu); active = false; } return value; }
    ~DepthScope() { finish(); }
};
bool complete(const game::ScenarioRuntimeServices& r) {
    const auto* d = r.destruction;
    return d && d->notices && d->target_index && d->first_object && d->first_spotlight &&
        d->destroy_spotlight && d->step && r.world && r.world->initialization_depth &&
        r.initialize && r.initialize->tactical && r.initialize->destroy_tactical;
}
}

int YRPP_FASTCALL ScenarioClass::DestroyWorldObjects() {
    const auto& r = game::scenario_runtime();
    if (!complete(r)) throw std::logic_error("Scenario destruction requires existing world collections and modules");
    const auto& d = *r.destruction;
    DepthScope depth(*r.world->initialization_depth);
    auto& notices = *d.notices;
    notices.Count = 0;
    // 4E0410 preserves borrowed Items and IsInitialized, just like the inline
    // slot-array destruction in ClearWorld. Do not reset the object's vtable.
    if (notices.Items && notices.IsAllocated) {
        DLLDeleteArray(notices.Items, static_cast<size_t>(notices.Capacity));
        notices.Items = nullptr;
    }
    notices.IsAllocated = false;
    notices.Capacity = 0;
    const auto step = [&](game::ScenarioDestructionStep value) { d.step(d.context, value); };
    const auto expired = [&] { step(game::ScenarioDestructionStep::expired_objects); };
    const auto destroy = [&](game::ScenarioObjectCollection collection, bool release = false) {
        for (;;) {
            AbstractClass* object = nullptr;
            if (!d.first_object(d.context, collection, object))
                throw std::logic_error("Scenario destruction could not query a live object collection");
            if (!object) break;
            if (release) object->Release();
            else delete object;
        }
        expired();
    };
    step(game::ScenarioDestructionStep::unload_shapes);
    auto& targets = *d.target_index;
    while (targets.Count() > 0) {
        if (!targets.IndexTable) throw std::logic_error("Scenario destruction found an invalid target index");
        // The original comparator uses signed ID order; Sort also preserves
        // the calibrated CRT tie ordering and clears Archive only if unsorted.
        targets.Sort();
        auto* object = targets.IndexTable[0].Data;
        if (!object) throw std::logic_error("Scenario destruction found an empty live target");
        delete object;
    }
    expired();
    destroy(game::ScenarioObjectCollection::bullets, true);
    for (auto collection : {game::ScenarioObjectCollection::objects, game::ScenarioObjectCollection::tags,
            game::ScenarioObjectCollection::triggers, game::ScenarioObjectCollection::tubes,
            game::ScenarioObjectCollection::building_lights, game::ScenarioObjectCollection::overlays,
            game::ScenarioObjectCollection::particle_systems, game::ScenarioObjectCollection::waves,
            game::ScenarioObjectCollection::factories, game::ScenarioObjectCollection::sides,
            game::ScenarioObjectCollection::teams, game::ScenarioObjectCollection::houses,
            game::ScenarioObjectCollection::animations, game::ScenarioObjectCollection::scripts,
            game::ScenarioObjectCollection::radiation_sites, game::ScenarioObjectCollection::light_sources,
            game::ScenarioObjectCollection::empulses, game::ScenarioObjectCollection::capture_managers,
            game::ScenarioObjectCollection::disk_lasers, game::ScenarioObjectCollection::parasites,
            game::ScenarioObjectCollection::temporals, game::ScenarioObjectCollection::airstrikes,
            game::ScenarioObjectCollection::spawn_managers, game::ScenarioObjectCollection::bombs}) destroy(collection);
    for (;;) {
        SpotlightClass* spotlight = nullptr;
        if (!d.first_spotlight(d.context, spotlight))
            throw std::logic_error("Scenario destruction could not query live spotlights");
        if (!spotlight) break;
        d.destroy_spotlight(d.context, spotlight);
    }
    expired();
    destroy(game::ScenarioObjectCollection::fogged_objects);
    destroy(game::ScenarioObjectCollection::alpha_shapes);
    destroy(game::ScenarioObjectCollection::terrain);
    step(game::ScenarioDestructionStep::electric_bolts);
    step(game::ScenarioDestructionStep::line_trails);
    step(game::ScenarioDestructionStep::lasers);
    destroy(game::ScenarioObjectCollection::types);
    if (*r.initialize->tactical)
        r.initialize->destroy_tactical(r.initialize->context, *r.initialize->tactical);
    *r.initialize->tactical = nullptr;
    step(game::ScenarioDestructionStep::map_objects);
    expired();
    step(game::ScenarioDestructionStep::beacons);
    return depth.finish();
}
