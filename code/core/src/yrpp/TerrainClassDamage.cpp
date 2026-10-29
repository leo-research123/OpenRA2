// Terrain damage and burning, calibrated to YR 0x71B920/0x71C5B0/0x71C6B0.
#include "yrpp/TerrainClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "background_invalidation.hpp"
#include <exception>
#include <bit>

DamageState TerrainClass::ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead,
                                        ObjectClass* attacker, bool ignoreDefenses, bool preventEscape,
                                        HouseClass* house) {
    if (!warhead || !warhead->Wood || Type->Immune) return DamageState::Unaffected;
    const auto result = ObjectClass::ReceiveDamage(damage, distance, warhead, attacker, ignoreDefenses,
                                                  preventEscape, house);
    if (result != DamageState::NowDead) return result;
    if (Type->SpawnsTiberium) {
        if (void* memory = YRMemory::Allocate(sizeof(AnimClass))) {
            const auto at = Location;
            const auto land = MapClass::Instance.GetCellAt(Location)->LandType;
            auto* animation = MapClass::SelectDamageAnimation(100, RulesClass::Instance->C4Warhead, land, at);
            ::new (memory) AnimClass(animation, at, 0, 1, 0x2600, -15, false);
        }
        const auto at = Location;
        MapClass::DamageArea(at, 100, nullptr, RulesClass::Instance->C4Warhead, true, nullptr);
        // The original follows this with GetMapCoords and a vestigial nullsub.
        GetMapCoords();
    } else if (IsBurning) {
        for (auto* animation : AnimClass::Array)
            if (animation->OwnerObject == this) animation->RemainingIterations = 0;
    } else {
        StartCrumbling();
    }
    RectangleStruct bounds;
    TacticalClass::Instance->RegisterDirtyArea(*GetRenderDimensions(&bounds), false);
    Disappear(true);
    UnInit();
    return result;
}

bool TerrainClass::Ignite() {
    if (IsCrumbling || IsBurning || int(Type->Armor) != 6 || Type->SpawnsTiberium) return false;
    const auto choice = unsigned(ScenarioClass::Instance->Random.Random()) & 1u;
    void* memory = YRMemory::Allocate(sizeof(AnimClass));
    // Original 0x71C5B0 cannot recover from a failed allocation.
    if (!memory) std::terminate();
    auto at = GetCoords();
    at.Z = std::bit_cast<int>(unsigned(at.Z) + 80u);
    auto* animation = ::new (memory) AnimClass(RulesClass::Instance->TreeFire[choice], at, 0, 255, 0x600, 0, false);
    animation->SetOwnerObject(this);
    animation->ZAdjust = std::bit_cast<int>(unsigned(animation->ZAdjust) - 20u);
    IsBurning = true;
    game::invalidate_background_object(*this);
    return true;
}

void TerrainClass::Extinguish() {
    if (!IsBurning) return;
    IsBurning = false;
    game::invalidate_background_object(*this);
    if (!IsCrumbling && Health == 0) {
        Disappear(true);
        Mark(MarkType::Change);
        StartCrumbling();
    }
}
