#include "debug/debug_state.hpp"
// YR 0x75F330: sonic damage follows the live cell object chain.
#include "yrpp/WaveClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"

void WaveClass::DamageArea(const CoordStruct& location) const {
    if (!Owner) return;
    auto* cell = MapClass::Instance.GetCellAt(location);
    auto* weapon = Owner->GetWeapon(0)->WeaponType;
    auto* warhead = weapon->Warhead;
    int override_damage = 0;
    const bool overridden = game::query_debug_attack_damage(Owner, weapon->Damage, warhead, override_damage);
    int damage = weapon->Damage;
    auto* object = (unsigned(cell->Flags) & 0x100u) && LimboCoords.Z >= Unsorted::LevelHeight * (int(cell->Level) + 4)
                       ? cell->AltObject
                       : cell->FirstObject;
    for (; object; object = object->NextObject) {
        if (object != Owner && object->IsAlive && object->IsOnMap && !object->InLimbo && object->Health > 0) {
            // Victim defenses may change damage in place; each override hit starts fresh.
            if (overridden) damage = override_damage;
            object->ReceiveDamage(&damage, 0, warhead, Owner, false, false, nullptr);
        }
    }
    if (cell->OverlayTypeIndex != -1) {
        auto* overlay = OverlayTypeClass::Array[cell->OverlayTypeIndex];
        // 0x75F443's overlay explosion callback is nullsub_111 in YR.
        if (overlay->Wall) cell->DamageWall(game::debug_attack_damage(Owner, weapon->Damage, warhead));
    }
    if (cell->Tile_Is_DestroyableCliff() &&
        ScenarioClass::Instance->Random.RandomRanged(0, 99) < RulesClass::Instance->CollapseChance)
        MapClass::Instance.DestroyCliff(cell);
}
