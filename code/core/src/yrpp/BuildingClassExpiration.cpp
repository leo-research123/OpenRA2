// YR 0x0044E8F0 / 0x00451B40. Object deletion must also clear building-owned
// references to infantry; restoring only Techno's base pointers is insufficient.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/AnimClass.h"
namespace {
void play_slot(BuildingClass& building, int slot, bool garrisoned = false) {
    if (!building.Type) return;
    const bool damaged = !building.IsGreenHP();
    const auto& entry = building.Type->BuildingAnim[slot];
    const char* name = damaged ? entry.Damaged : garrisoned ? entry.Garrisoned : entry.Anim;
    if (*name) building.PlayAnim(name, static_cast<BuildingAnimSlot>(slot), damaged, garrisoned, 0);
}
}
void BuildingClass::DetachAnim(AnimClass* anim) {
    if (!IsAlive || !Type || !anim) return;
    int slot = 0;
    while (slot < 21 && Anims[slot] != anim) ++slot;
    if (slot == 21) return;
    Anims[slot] = nullptr;
    switch (slot) {
        case 10:
            if (Type->UnitRepair) play_slot(*this, HasAnyLink() && GetCurrentMission() == Mission::Repair ? 11 : 18);
            else if (Type->IsAnimDelayedFire && anim->HasExtras) play_slot(*this, 3, GetOccupantCount() > 0);
            break;
        case 12: if (Type->UnitRepair && anim->HasExtras) play_slot(*this, 18); break;
        case 15: if (anim->HasExtras) play_slot(*this, 16); break;
        case 17: if (anim->HasExtras) play_slot(*this, 14); break;
        default: break;
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
    if (Anims[8] == object) play_slot(*this, 18);
    if (Type && Type->Grinding && Anims[10] == object) play_slot(*this, 3);
    if (object->What_Am_I() == AbstractType::Anim) {
        for (auto*& fire : DamageFireAnims) if (fire == object) { fire = nullptr; return; }
        auto* anim = static_cast<AnimClass*>(object);
        if (anim->IsBuildingAnim) { DetachAnim(anim); return; }
    }
    if (Type == object) Type = nullptr;
    for (auto*& upgrade : Upgrades) if (upgrade == object) upgrade = nullptr;
    // These lists remove a single entry, not all duplicate pointers.
    Overpowerers.Remove(reinterpret_cast<InfantryClass*>(object));
    if (Unsorted::ScenarioInit) Occupants.Remove(reinterpret_cast<InfantryClass*>(object));
}
