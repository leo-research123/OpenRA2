/*
 * World clearing adapts EA REDALERT/SCENARIO.CPP::Clear_Scenario, revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc.; GPL-3.0-or-later, third_party/ea/LICENSE.TXT. YR 6851F0..685663
 * supplies preserved tile types, COM releases and module/state ordering.
 * Global object destruction (534450) uses the shared destruction coordinator.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VocClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <stdexcept>

namespace {
template<class T> void clear_slots(DynamicVectorClass<T>& list) {
    // The original inlined clear preserves borrowed Items and IsInitialized,
    // while dropping count/capacity/ownership. VectorClass::Clear has different
    // explicit-reset semantics, so do not call it through the EXE's vtable.
    list.Count = 0;
    if (list.Items && list.IsAllocated) {
        DLLDeleteArray(list.Items, static_cast<size_t>(list.Capacity));
        list.Items = nullptr;
    }
    list.IsAllocated = false;
    list.Capacity = 0;
}
template<class T> bool first(DynamicVectorClass<T*>& list, T*& result) {
    if (list.Count <= 0 || !list.Items || !list.Items[0]) return false;
    result = list.Items[0];
    return true;
}
bool complete(const game::ScenarioRuntimeServices& r) {
    const auto* c = r.clear;
    const auto* i = r.initialize;
    return ScenarioClass::Instance && c && c->types && c->tile_types && c->objects && c->tag_types &&
        c->map_tags && c->logic_tags && c->current_objects && c->particle_system && c->empty_cell &&
        c->destroy_world_objects && c->step && c->drain_particles && c->reset_map_start_positions &&
        i && i->building_read_flag && i->tactical && i->destroy_tactical && i->create_tactical &&
        r.houses && r.houses->current_player && r.pause && r.pause->volume && *r.pause->volume &&
        r.render && r.render->map;
}
}

int YRPP_FASTCALL ScenarioClass::ClearWorld() {
    const auto& r = game::scenario_runtime();
    if (!complete(r)) throw std::logic_error("Scenario world cleanup requires existing world, audio and rendering services");
    const auto& c = *r.clear;
    const auto& initialization = *r.initialize;
    auto& map = *r.render->map;
    const auto step = [&](game::ScenarioClearStep value) { c.step(c.context, value); };
    Instance->UniqueID = 1000000;
    Instance->Random.unknown_00 = true;
    struct RestoreRandomFlag { ~RestoreRandomFlag() { ScenarioClass::Instance->Random.unknown_00 = false; } } random_flag;
    PausedAudioVolume = 0x4000;
    (*r.pause->volume)->SetVolume(0x4000);
    step(game::ScenarioClearStep::stop_audio);
    *r.houses->current_player = nullptr;
    Instance->Reset();
    step(game::ScenarioClearStep::stop_lightning);
    for (int i = c.types->Count - 1; i >= 0; --i) {
        if (!c.types->Items || !c.types->Items[i]) throw std::logic_error("Scenario world cleanup found an invalid type");
        auto* rtti = static_cast<IRTTITypeInfo*>(c.types->Items[i]);
        if (rtti->What_Am_I() == AbstractType::IsotileType) c.types->RemoveItem(i);
    }
    c.destroy_world_objects(c.context);
    for (int i = 0; i < c.tile_types->Count; ++i) {
        if (!c.tile_types->Items) throw std::logic_error("Scenario world cleanup requires live tile type storage");
        // Original ignores failed growth and keeps processing the remaining tiles.
        c.types->AddItem(c.tile_types->Items[i]);
    }
    step(game::ScenarioClearStep::map_objects);
    clear_slots(map.ZoneConnections);
    step(game::ScenarioClearStep::tags);
    for (int i = 0; i < 50; ++i) Instance->SetGlobal(i, 0);
    if (*initialization.tactical) initialization.destroy_tactical(initialization.context, *initialization.tactical);
    *initialization.tactical = nullptr;
    TacticalClass* tactical = nullptr;
    if (!initialization.create_tactical(initialization.context, tactical) || !tactical)
        throw std::runtime_error("Scenario world cleanup could not create TacticalClass");
    *initialization.tactical = tactical;
    step(game::ScenarioClearStep::tactical_records);
    *initialization.building_read_flag = false;
    while (c.objects->Count > 0) {
        ObjectClass* object = nullptr;
        if (!first(*c.objects, object)) throw std::logic_error("Scenario world cleanup found an invalid live object");
        if (object->WhatAmI() == AbstractType::Bullet) object->Release();
        else delete object;
    }
    *initialization.building_read_flag = true;
    while (c.tag_types->Count > 0) {
        TagTypeClass* type = nullptr;
        if (!first(*c.tag_types, type)) throw std::logic_error("Scenario world cleanup found an invalid tag type");
        delete type;
    }
    clear_slots(*c.map_tags);
    clear_slots(*c.logic_tags);
    Instance->ParTimeEasy = Instance->ParTimeMedium = Instance->ParTimeDifficult = 3600;
    Instance->UnderParTitle[0] = Instance->UnderParMessage[0] = Instance->OverParTitle[0] = Instance->OverParMessage[0] = 0;
    for (auto operation : {game::ScenarioClearStep::light_sources, game::ScenarioClearStep::lightning,
            game::ScenarioClearStep::empulses, game::ScenarioClearStep::veinholes,
            game::ScenarioClearStep::tile_cache_front, game::ScenarioClearStep::tile_cache_back,
            game::ScenarioClearStep::bombs, game::ScenarioClearStep::kamikazes,
            game::ScenarioClearStep::aircraft_tracker, game::ScenarioClearStep::planning}) step(operation);
    map.MapRect = {0, 0, 0, 0};
    step(game::ScenarioClearStep::map_initialize);
    step(game::ScenarioClearStep::logic_initialize);
    clear_slots(*c.current_objects);
    std::fill_n(Instance->Waypoints, 702, *c.empty_cell);
    step(game::ScenarioClearStep::campaigns);
    if (*c.particle_system) {
        c.drain_particles(c.context, *c.particle_system);
        if (*c.particle_system) delete *c.particle_system;
        *c.particle_system = nullptr;
    }
    Instance->Random.unknown_00 = false;
    step(game::ScenarioClearStep::sidebar_timers);
    step(game::ScenarioClearStep::sidebar_objects);
    const int result = c.reset_map_start_positions(c.context);
    Instance->UniqueID = 1000000;
    return result;
}
