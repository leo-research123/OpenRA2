#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"

void HouseClass::UpdateSuperWeapons() {
    if(!Game::IsActive)return;
    const bool player=this==CurrentPlayer;
    for(int i=0;i<Supers.Count;++i) {
        auto* super=Supers[i];
        if(!super->IsPresent||((!super->CanHold||super->IsOneTime)&&!Defeated))continue;
        bool powered=false,present=false;
        if(!Defeated)for(int j=0;j<BuildingClass::Array.Count;++j) {
            auto* building=BuildingClass::Array[j];
            if(building->InLimbo||!building->IsAlive||building->Owner->Fetch_ID()!=Fetch_ID())continue;
            for(auto* upgrade:building->Upgrades)if(upgrade&&(upgrade->SuperWeapon==i||upgrade->SuperWeapon2==i)) {
                present=true;
                if(!powered)powered=building->HasPower;
            }
            if(building->FirstActiveSWIdx()==i||building->SecondActiveSWIdx()==i) {
                present=true;
                if(powered)break;
                powered=building->HasPower;
            }
            if(powered&&present)break;
        }
        const auto& runtime=game::scenario_runtime();
        const auto* session=runtime.houses?runtime.houses->session:nullptr;
        if(super->Type->DisableableFromShell&&session&&!session->Config.SWAllowed)present=false;
        if(GetPowerPercentage()<1.0)powered=false;
        const int tab=SidebarClass::GetObjectTabIdx(AbstractType::SuperWeaponType,super->Type->GetArrayIndex(),0);
        bool changed=false;
        if(!present||Defeated) {
            changed=super->Lose();
            if(!CurrentPlayer)continue;
        }else if(!powered&&super->IsPowered())changed=super->SetOnHold(true);
        else if(powered)changed=super->SetOnHold(false);
        if(!changed)continue;
        if(player) {
            if(DisplayClass::Instance.CurrentSWTypeIndex==i)DisplayClass::Instance.CurrentSWTypeIndex=-1;
            SidebarClass::Instance.RepaintSidebar(tab);
        }
        RecheckTechTree=true;
    }
}
