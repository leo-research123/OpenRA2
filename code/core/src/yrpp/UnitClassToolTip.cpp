// YR 0x00746B20. Use the original per-unit 0x100 wchar_t buffer, not UI state.
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/StringTable.h"
const wchar_t* UnitClass::GetUIName() const {
    if(IsDisguisedAs(HouseClass::CurrentPlayer) && GetDisguise(true))
        return GetDisguiseHouse(true)?GetDisguise(true)->UIName:nullptr;
    if(!Type)return nullptr;
    if(Type->TurretCount<=0 || !Type->HasTurretTooltips)return Type->UIName;
    const wchar_t* prefix=nullptr;
    const auto* passenger=Passengers.GetFirstPassenger();
    if(passenger && passenger->WhatAmI()==AbstractType::Infantry){
        const auto* infantry=static_cast<const InfantryClass*>(passenger);
        const bool own_name=infantry->Type->UseOwnName;
        // 0x00746BC6 calls 0x0070DCF0: read CurrentTurretNumber (+0x124),
        // not the weapon slot. Engineer weapon 1 selects repair turret 2.
        switch(CurrentTurretNumber){
        case 0:prefix=StringTable::LoadString("Tip:Rocket");break;
        case 1:if(!own_name)prefix=StringTable::LoadString("Tip:MachineGun");break;
        case 2:prefix=StringTable::LoadString("Tip:Repair");break;
        default:break;
        }
        if(!prefix && (CurrentTurretNumber==3 || own_name))prefix=passenger->GetType()->UIName;
    }
    if(!prefix)prefix=StringTable::LoadString("Tip:Rocket");
    auto* buffer=const_cast<UnitClass*>(this)->ToolTipText;
    // Preserve 0x00746C55's wide "prefix name" without a locale conversion:
    // Darwin swprintf("%ls") fails on Chinese in the default C locale and
    // leaves an empty tip. Keep the native bound/terminator adaptation.
    auto* cursor=buffer;
    const auto append=[&](const wchar_t* text){
        if(text)while(*text && cursor<buffer+0xFF)*cursor++=*text++;
    };
    append(prefix);append(L" ");append(Type->UIName);
    *cursor=0;return buffer;
}
