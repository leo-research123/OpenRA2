// YR 0x0044E8F0 / 0x00451B40. Object deletion must also clear building-owned
// references to infantry; restoring only Techno's base pointers is insufficient.
#include "yrpp/BuildingClass.h"
#include "techno_spatial.hpp"
#include "yrpp/InfantryClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/AnimClass.h"
namespace {
void play_slot(BuildingClass& building, BuildingAnimSlot slot, bool garrisoned = false) {
    if (!building.Type) return;
    const bool damaged = !building.IsGreenHP();
    const auto& entry = building.Type->BuildingAnim[static_cast<int>(slot)];
    const char* name = damaged ? entry.Damaged : garrisoned ? entry.Garrisoned : entry.Anim;
    if (*name) building.PlayAnim(name, slot, damaged, garrisoned, 0);
}
}
void BuildingClass::DetachAnim(AnimClass* anim) {
    if (!IsAlive || !Type || !anim) return;
    int slot = 0;
    while (slot < 21 && Anims[slot] != anim)
        ++slot;
    if (slot == 21) return;
    Anims[slot] = nullptr;
    switch (static_cast<BuildingAnimSlot>(slot)) {
        case BuildingAnimSlot::Special:
            if (Type->UnitRepair)
                play_slot(*this, HasAnyLink() && GetCurrentMission() == Mission::Repair ? BuildingAnimSlot::SpecialTwo
                                                                                        : BuildingAnimSlot::Idle);
            else if (Type->IsAnimDelayedFire && anim->TimeToDie)
                play_slot(*this, BuildingAnimSlot::Active, GetOccupantCount() > 0);
            break;
        case BuildingAnimSlot::SpecialThree:
            if (Type->UnitRepair && anim->TimeToDie) play_slot(*this, BuildingAnimSlot::Idle);
            break;
        case BuildingAnimSlot::SuperTwo:
            if (anim->TimeToDie) play_slot(*this, BuildingAnimSlot::SuperThree);
            break;
        case BuildingAnimSlot::SuperFour:
            if (anim->TimeToDie) play_slot(*this, BuildingAnimSlot::Super);
            break;
        default:
            break;
    }
}
void BuildingClass::PointerExpired(AbstractClass* object, bool removed) {
    if (!object) return;
    TechnoClass::PointerExpired(object, removed);
    if (C4AppliedBy == object) C4AppliedBy = nullptr;
    if (FirestormAnim == object) FirestormAnim = nullptr;
    if (Factory == object) Factory = nullptr;
    if (Spotlight == object) Spotlight = nullptr;
    if (LightSource == object) LightSource = nullptr;
    if (Anims[static_cast<int>(BuildingAnimSlot::Production)] == object) play_slot(*this, BuildingAnimSlot::Idle);
    if (Type && Type->Grinding && Anims[static_cast<int>(BuildingAnimSlot::Special)] == object)
        play_slot(*this, BuildingAnimSlot::Active);
    if (object->What_Am_I() == AbstractType::Anim) {
        for (auto*& fire : DamageFireAnims)
            if (fire == object) {
                fire = nullptr;
                return;
            }
        auto* anim = static_cast<AnimClass*>(object);
        if (anim->IsBuildingAnim) {
            DetachAnim(anim);
            return;
        }
    }
    if (Type == object) {
        Type = nullptr;
        game::techno_spatial_dirty(this);
    }
    for (auto*& upgrade : Upgrades)
        if (upgrade == object) upgrade = nullptr;
    // These lists remove a single entry, not all duplicate pointers.
    Overpowerers.Remove(reinterpret_cast<InfantryClass*>(object));
    if (Unsorted::ScenarioInit) Occupants.Remove(reinterpret_cast<InfantryClass*>(object));
}
