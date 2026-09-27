// Existing Building/Anim lifecycle model (EA REDALERT/BUILDING.CPP and ANIM.CPP,
// f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae; copyright Electronic Arts Inc.,
// GPL-3.0-or-later plus code/third_party/ea/LICENSE.TXT).
// YR's eight ART fire slots, thresholds and positioning are calibrated to
// 0x0043C0D0 and 0x0043FC41..0x0043FCC4. No combat damage is simulated here.
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "map_world.hpp"
#include <algorithm>
#include <new>

void BuildingClass::CreateDamageFires() noexcept {
    auto* rules=RulesClass::Instance;auto* scenario=ScenarioClass::Instance;
    if(!Type||!rules||!scenario||!TacticalClass::Instance||rules->DamageFireTypes.Count<=0)return;
    const auto& types=rules->DamageFireTypes;
    int index=scenario->Random.RandomRanged(0,types.Count-1);
    for(int slot=0;slot<8;++slot){
        const auto offset=Type->DamageFireOffset[slot];
        if(offset==Point2D{0,0}||DamageFireAnims[slot])break;
        auto location=GetRenderCoords();
        const auto delta=TacticalClass::Instance->ApplyMatrix_Pixel(offset);
        location.X+=delta.X;location.Y+=delta.Y;
        auto* type=types[index];if(!type)continue;
        auto* anim=new(std::nothrow) AnimClass(type,location,0,1,0x600,0,false);
        if(!anim)continue;
        DamageFireAnims[slot]=anim;
        // SAR rounds negative odd values down, unlike signed division by two.
        const int depth=3*(offset.Y-15*Type->GetFoundationWidth()-15*Type->GetFoundationHeight(false));
        anim->ZAdjust=std::min((depth>>1)-10,0);
        if(type->End>0)anim->Animation.Value=scenario->Random.RandomRanged(0,type->End-1);
        if(++index>=types.Count)index=0;
        game::map_object_changed();
    }
}

void BuildingClass::UpdateDamageFires() noexcept {
    // Garrisonable civilian buildings catch fire at ConditionRed; others at
    // ConditionYellow. This flag tracks the state, not a one-tick request.
    const bool active=Type&&IsOnMap&&!InLimbo&&!IsDead();
    const bool required=active&&(Type->CanBeOccupied?IsRedHP():!IsGreenHP());
    if(required!=RequiresDamageFires){
        if(required)CreateDamageFires();
        else for(auto*& fire:DamageFireAnims){auto* old=fire;fire=nullptr;delete old;}
        RequiresDamageFires=required;game::map_object_changed();
    }
    // Also release externally-created fires when an object leaves the map.
    if(!active)for(auto*& fire:DamageFireAnims)if(fire){auto* old=fire;fire=nullptr;delete old;game::map_object_changed();}
}
