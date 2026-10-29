// Original InfantryClass disguise queries and restoration.
#include "yrpp/InfantryClass.h"
#include "techno_spatial.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"

void InfantryClass::ClearDisguise() {
    game::techno_spatial_dirty(this);
    if (Type->PermaDisguise) {
        Disguised = true;
        DisguisedAsHouse = Owner;
        const auto* rules = RulesClass::Instance;
        Disguise = Owner->SideIndex == 0   ? rules->AlliedDisguise
                   : Owner->SideIndex == 1 ? rules->SovietDisguise
                                           : rules->ThirdDisguise;
    } else {
        Disguised = false;
    }
}

DWORD InfantryClass::DisguiseAs(AbstractClass* target) {
    if (target && target->WhatAmI() == AbstractType::Infantry) {
        TechnoClass::DisguiseAs(target);
        auto* object = static_cast<ObjectClass*>(target);
        if (object->IsDisguised()) {
            Disguise = object->GetDisguise(true);
            DisguisedAsHouse = object->GetDisguiseHouse(true);
        } else {
            Disguise = object->GetType();
            DisguisedAsHouse = object->GetOwningHouse();
        }
    }
    return 0;
}
ObjectTypeClass* InfantryClass::GetDisguise(bool againstAllies) const {
    if (!againstAllies && Owner && Owner->IsAlliedWith(HouseClass::CurrentPlayer)) return Type;
    if (Disguise) return Disguise;
    const auto* rules = RulesClass::Instance;
    if (!rules) return nullptr;
    const auto* player = HouseClass::CurrentPlayer;
    if (!player) return rules->ThirdDisguise;
    return player->SideIndex == 0   ? rules->AlliedDisguise
           : player->SideIndex == 1 ? rules->SovietDisguise
                                    : rules->ThirdDisguise;
}
HouseClass* InfantryClass::GetDisguiseHouse(bool againstAllies) const {
    if (!againstAllies && Owner && Owner->IsAlliedWith(HouseClass::CurrentPlayer)) return Owner;
    return DisguisedAsHouse ? DisguisedAsHouse : HouseClass::CurrentPlayer;
}
