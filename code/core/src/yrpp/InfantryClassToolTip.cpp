// YR 0x0051F2C0. Technician precedes disguise; allied spies show their real name.
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/RulesClass.h"
ObjectTypeClass* InfantryClass::GetDisguise(bool againstAllies) const {
    if(!againstAllies && Owner && Owner->IsAlliedWith(HouseClass::CurrentPlayer))return Type;
    if(Disguise)return Disguise;
    const auto* rules=RulesClass::Instance;if(!rules)return nullptr;
    const auto* player=HouseClass::CurrentPlayer;
    if(!player)return rules->ThirdDisguise;
    return player->SideIndex==0?rules->AlliedDisguise:player->SideIndex==1?rules->SovietDisguise:rules->ThirdDisguise;
}
HouseClass* InfantryClass::GetDisguiseHouse(bool againstAllies) const {
    if(!againstAllies && Owner && Owner->IsAlliedWith(HouseClass::CurrentPlayer))return Owner;
    return DisguisedAsHouse?DisguisedAsHouse:HouseClass::CurrentPlayer;
}
const wchar_t* InfantryClass::GetUIName() const {
    if(Technician)return StringTable::LoadString("TXT_TECHNICIAN");
    const auto* type=IsDisguised() && !(Owner && Owner->IsAlliedWith(HouseClass::CurrentPlayer))
        ? GetDisguise(true) : Type;
    return type?type->UIName:nullptr;
}
