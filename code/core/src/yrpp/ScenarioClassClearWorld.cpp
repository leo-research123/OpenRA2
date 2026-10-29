/*
 * World clearing adapts EA REDALERT/SCENARIO.CPP::Clear_Scenario, revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc.; GPL-3.0-or-later, third_party/ea/LICENSE.TXT. YR 6851F0..685663
 * supplies preserved tile types, COM releases and module/state ordering.
 */
#include "background_invalidation.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/BombListClass.h"
#include "yrpp/EMPulseClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/IonBlastClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Kamikaze.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/VeinholeMonsterClass.h"
#include "yrpp/VocClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <stdexcept>

namespace {
template <class T>
void clear_slots(DynamicVectorClass<T>& list) {
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
// Deletes the first live entry through its virtual destructor until the
// collection is empty; destructors unregister themselves.
template <class T>
void delete_all(DynamicVectorClass<T*>& objects) {
    while (objects.Count > 0) {
        auto* object = objects.Items[0];
        if (!object) throw std::logic_error("Scenario world cleanup found an empty live object slot");
        delete static_cast<AbstractClass*>(object);
    }
}
bool complete(const game::ScenarioRuntimeServices& r) {
    const auto* c = r.clear;
    return ScenarioClass::Instance && c && c->stop_audio && c->init_campaigns;
}
}

int YRPP_FASTCALL ScenarioClass::ClearWorld() {
    game::reset_background_invalidation_extent();
    const auto& r = game::scenario_runtime();
    if (!complete(r)) throw std::logic_error("Scenario world cleanup requires the audio, campaign and Tactical owners");
    const auto& c = *r.clear;
    auto& map = MapClass::Instance;
    Instance->UniqueID = 1000000;
    Instance->Random.unknown_00 = true;
    struct RestoreRandomFlag {
        ~RestoreRandomFlag() { ScenarioClass::Instance->Random.unknown_00 = false; }
    } random_flag;
    PausedAudioVolume = 0x4000;
    // A host without the original mixer volume object has none to restore.
    if (r.pause && r.pause->volume && *r.pause->volume) (*r.pause->volume)->SetVolume(0x4000);
    c.stop_audio(c.context);  // 0x750FA0 releases the playing sound events.
    HouseClass::CurrentPlayer = nullptr;
    Instance->Reset();
    // Drop borrowed storm animation pointers before destroying the world.
    while (IonBlastClass::Array.Count) delete IonBlastClass::Array[IonBlastClass::Array.Count - 1];
    LightningStorm::Init();
    // Tile types survive the type destruction and return to the type list.
    for (int i = AbstractTypeClass::Array.Count - 1; i >= 0; --i) {
        if (!AbstractTypeClass::Array.Items || !AbstractTypeClass::Array.Items[i])
            throw std::logic_error("Scenario world cleanup found an invalid type");
        // 0x685256 dispatches through the secondary COM interface.
        if (AbstractTypeClass::Array.Items[i]->What_Am_I() == AbstractType::IsotileType)
            AbstractTypeClass::Array.RemoveItem(i);
    }
    DestroyWorldObjects();
    for (int i = 0; i < IsometricTileTypeClass::Array.Count; ++i) {
        if (!IsometricTileTypeClass::Array.Items)
            throw std::logic_error("Scenario world cleanup requires live tile type storage");
        // Original ignores failed growth and keeps processing the remaining tiles.
        AbstractTypeClass::Array.AddItem(IsometricTileTypeClass::Array.Items[i]);
    }
    map.MapClass::DestructCells();
    clear_slots(map.ZoneConnections);
    TagClass::DeleteAll();
    for (int i = 0; i < 50; ++i)
        Instance->SetGlobal(i, 0);
    // 0x006853A6: the old object goes through its virtual deleting destructor
    // (which clears the global), then a fresh one is allocated and constructed.
    if (TacticalClass::Instance) delete TacticalClass::Instance;
    TacticalClass::Instance = new TacticalClass();
    // 0x6DA980 empties the dirty-area list; native hosts redraw whole frames
    // and keep no such list.
    BuildingReadFlag = false;
    while (ObjectClass::Array.Count > 0) {
        auto* object = ObjectClass::Array.Items[0];
        if (!object) throw std::logic_error("Scenario world cleanup found an invalid live object");
        if (object->WhatAmI() == AbstractType::Bullet)
            object->Release();
        else
            delete object;
    }
    BuildingReadFlag = true;
    while (TagTypeClass::Array.Count > 0) {
        auto* type = TagTypeClass::Array.Items[0];
        if (!type) throw std::logic_error("Scenario world cleanup found an invalid tag type");
        delete type;
    }
    clear_slots(MapClass::PendingTags);
    clear_slots(LogicClass::PendingTags);
    Instance->ParTimeEasy = Instance->ParTimeMedium = Instance->ParTimeDifficult = 3600;
    Instance->UnderParTitle[0] = Instance->UnderParMessage[0] = Instance->OverParTitle[0] =
        Instance->OverParMessage[0] = 0;
    // 0x5549A0: light sources, with light updates suspended. Its queue of
    // time-sliced light recalculations has no native counterpart: native
    // light sources recalculate their cells immediately.
    BuildingReadFlag = false;
    delete_all(LightSourceClass::Array);
    BuildingReadFlag = true;
    // 0x539760: superweapon screen effects, as far as native code keeps them.
    LightningStorm::Active = false;
    NukeFlash::Status = NukeFlashStatus::Inactive;
    PsyDom::Status = PsychicDominatorStatus::Inactive;
    PsyDom::Anim = nullptr;
    PsyDom::Owner = nullptr;
    PsyDom::Coords = {-1,-1};
    NukeFlash::Duration = NukeFlash::StartTime = -1;
    ChronoScreenEffect::Status = 0;
    Instance->AmbientTarget = Instance->AmbientOriginal;
    RecalcLighting(-1, -1, -1, false);
    delete_all(EMPulseClass::Array);          // 0x4C5470
    delete_all(VeinholeMonsterClass::Array);  // 0x74D760
    clear_slots(VeinholeMonsterClass::Array);
    TiberiumClass::ClearSpreadLogic();
    TiberiumClass::ClearGrowthLogic();
    BombListClass::Instance.Clear();
    Kamikaze::Instance.Clear();
    AircraftTrackerClass::Instance.Clear();
    Game::PlanningManager_DeleteAll();
    map.MapRect = {0, 0, 0, 0};
    // 0x5BDF50, Map.Init_Clear: the sidebar's clear, then the mouse state.
    MouseClass::Instance.SidebarClass::Init_Clear();
    MouseClass::Instance.MouseCursorIsMini = false;
    MouseClass::Instance.MouseCursorLastIndex = static_cast<MouseCursorType>(0);
    LogicClass::Instance.Clear();
    clear_slots(ObjectClass::CurrentObjects);
    std::fill_n(Instance->Waypoints, 702, CellStruct{0, 0});
    c.init_campaigns(c.context);  // 0x52CB90 reads BATTLE*.INI from disk.
    if (ParticleSystemClass::DefaultSystem) {
        ParticleSystemClass::DefaultSystem->ClearParticles();
        if (ParticleSystemClass::DefaultSystem) delete ParticleSystemClass::DefaultSystem;
        ParticleSystemClass::DefaultSystem = nullptr;
    }
    Instance->Random.unknown_00 = false;
    // 0x734210 restarts the double-press timers of the team and radar-event
    // commands; native commands keep no such timers.
    Game::SidebarTabObjects[0] = Game::SidebarTabObjects[1] = nullptr;  // 0x734270(nullptr)
    const int result = RadarEventClass::ClearHistory();
    Instance->UniqueID = 1000000;
    return result;
}
