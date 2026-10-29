// Waypoint effects used by campaign scripts; YR TActionClass helper entries.
#include "yrpp/TActionClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/VocClass.h"
#include <bit>
#include <cstdlib>

namespace {
CoordStruct waypoint_position(int waypoint, bool lowBridge = true) {
    const auto cell = ScenarioClass::Instance->GetWaypointCoords(waypoint);
    CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
    at.Z = MapClass::Instance.GetCellFloorHeight(at);
    const unsigned mask = lowBridge ? 0x500u : 0x100u;
    if (unsigned(MapClass::Instance.GetCellAt(cell)->Flags) & mask) at.Z += CellClass::BridgeHeight;
    return at;
}
ObjectClass* sound_object(const CoordStruct& at) {
    const CellStruct cell{short(at.X / 256), short(at.Y / 256)};
    if (cell == CellStruct::Empty) return nullptr;
    auto* tile = MapClass::Instance.GetCellAt(cell);
    if (auto* building = tile->GetBuilding()) return building;
    return tile->GetTerrain(false);
}
}
bool TActionClass::RevealZoneOfWaypoint(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (HouseClass::CurrentPlayer->Visionary) return true;
    auto& map = MapClass::Instance;
    const auto cell = ScenarioClass::Instance->GetWaypointCoords(Value);
    const int zone = map.GetMovementZoneType(cell, static_cast<MovementZone>(1), false);
    map.ZoneIteratorReset();
    while (auto* tile = map.ZoneIteratorNext()) {
        if (map.GetMovementZoneType(tile->MapCoords, static_cast<MovementZone>(1), false) != zone) continue;
        const int height = static_cast<signed char>(tile->Level) / 2;
        const auto project = [height](short value) {
            return int(std::bit_cast<short>(static_cast<unsigned short>(int(value) - height / 2))) * 256 + 128;
        };
        CoordStruct at{project(tile->MapCoords.X), project(tile->MapCoords.Y), height * Unsorted::LevelHeight};
        const auto original = at;
        map.RevealArea2(&at, 2, HouseClass::CurrentPlayer, 0, 0, 0, 1, 0);
        at = original;
        map.RevealArea2(&at, 2, HouseClass::CurrentPlayer, 0, 0, 0, 1, 1);
    }
    return true;
}
bool TActionClass::PlayAnimAtWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        auto at = waypoint_position(Waypoint);
        auto* type = AnimTypeClass::Array.GetItemOrDefault(Value);
        if (!type) return false;
        auto* storage = YRMemory::Allocate(sizeof(AnimClass));
        if (!storage) std::abort();
        auto* anim = ::new (storage) AnimClass(type, at, 0, 1, 0x600, 0, false);
        anim->IsInert = true;
        return true;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::FireWeaponAtWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        const auto at = waypoint_position(Waypoint);
        auto* weapon = WeaponTypeClass::Array.GetItemOrDefault(Value);
        if (!weapon) return false;
        const int damage = weapon->Damage;
        auto* warhead = weapon->Warhead;
        auto* type = MapClass::SelectDamageAnimation(damage, warhead, MapClass::Instance.GetCellAt(at)->LandType, at);
        if (type)
            if (auto* storage = YRMemory::Allocate(sizeof(AnimClass)))
                ::new (storage) AnimClass(type, at, 0, 1, 0x2600, -15, false);
        MapClass::FlashbangWarheadAt(damage, warhead, at, false, SpotlightFlags::None);
        MapClass::DamageArea(at, damage, nullptr, warhead, true, nullptr);
        return true;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::LightningStormStart(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (LightningStorm::IsActive()) return false;
    LightningStorm::Start(Value, RulesClass::Instance->LightningDeferment, CellStruct::Empty, nullptr);
    return true;
}
bool TActionClass::LightningStormAtWP(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    const auto at = waypoint_position(Waypoint, false);
    const CellStruct cell{short(at.X / 256), short(at.Y / 256)};
    if (LightningStorm::IsActive()) return false;
    auto& rules = *RulesClass::Instance;
    LightningStorm::Start(rules.LightningStormDuration, rules.LightningDeferment, cell, house);
    return true;
}
bool TActionClass::LightningStrikeAt(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    LightningStorm::Strike(ScenarioClass::Instance->GetWaypointCoords(Waypoint));
    return true;
}
bool TActionClass::Apply100DamageAt(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    constexpr int dx[]{0, 85, -85, 85, -85}, dy[]{0, 85, 85, -85, -85};
    for (int i = 0; i < 5; ++i) {
        auto at = waypoint_position(Value);
        at.X += dx[i];
        at.Y += dy[i];
        MapClass::DamageArea(at, 100, nullptr, RulesClass::Instance->C4Warhead, true, nullptr);
    }
    if (ScenarioClass::Instance->IsDefinedWaypoint(Value)) {
        const auto cell = ScenarioClass::Instance->GetWaypointCoords(Value);
        for (int i = 0; i < 4; ++i)
            if (MapClass::Instance.DamageBridgeAt(cell)) break;
        const CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, CellClass::BridgeHeight};
        Point2D point{};
        TacticalClass::Instance->CoordsToClient(&at, &point);
        TacticalClass::Instance->RegisterDirtyArea({point.X - 128, point.Y - 128, 256, 256}, false);
    }
    return true;
}
bool TActionClass::PlayAudioAtSpecificWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    const auto at = waypoint_position(Waypoint);
    if (auto* object = sound_object(at))
        object->SetCustomSound(Value);
    else
        VocClass::PlayIndexAtPos(Value, at, 1);
    return true;
}
bool TActionClass::StopAudioAtSpecificWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    const auto at = waypoint_position(Waypoint);
    if (auto* object = sound_object(at))
        object->SetCustomSound(-1);
    else
        VocClass::StopIndexAtPos(at, 1);
    return true;
}
