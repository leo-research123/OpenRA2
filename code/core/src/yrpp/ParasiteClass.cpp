// YR ParasiteClass, calibrated to 0x6292B0..0x62AF60. The fixed OpenTS
// infantry/foot baseline has no equivalent RA2 parasite/dog-pounce manager.
#include "yrpp/ParasiteClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>

namespace {
#if !defined(RA2_YRPP_GAME)
DynamicVectorClass<ParasiteClass*> parasites,animListeners;
#endif
void reset_grapple(ParasiteClass& parasite){
    parasite.GrappleAnimFrame=parasite.GrappleAnimDelay=0;parasite.GrappleState=ParasiteState::Start;
    if(parasite.GrappleAnim){auto* anim=parasite.GrappleAnim;parasite.GrappleAnim=nullptr;anim->UnInit();}
    if(parasite.GrappleAnimGotInvalid){ParasiteClass::AnimExpirationListeners.Remove(&parasite);parasite.GrappleAnimGotInvalid=false;}
}
void resume_owner(FootClass& owner,bool restoreSelection){
    if(restoreSelection){
        if(owner.ShouldBeReselectOnUnlimbo&&owner.Owner->IsControlledByCurrentPlayer()){
            owner.ShouldBeReselectOnUnlimbo=false;owner.Select();
        }
        if(owner.OldTeam)owner.OldTeam->AddMember(&owner,false);
    }
    if(owner.MegaMission==Mission::None){owner.SetArchiveTarget(nullptr);owner.SetTarget(nullptr);owner.SetDestination(nullptr,true);}
    owner.EnterIdleMode(false,true);
    owner.UpdateSight(0,0,0,0,0);
    auto at=owner.GetCoords();MapClass::Instance.RevealArea3(&at,owner.LastSightRange-3,owner.LastSightRange+3,false);
}
bool water_without_bridge(const CellClass& cell,const CellClass& bridgeSource){
    if(cell.LandType!=LandType::Water&&cell.LandType!=LandType::Beach&&cell.LandType!=LandType::Rock)return false;
    const int tile=bridgeSource.IsoTileTypeIndex;
    return !(unsigned(bridgeSource.Flags)&0x100)&&!(tile>=74&&tile<=99)&&!(tile>=205&&tile<=230);
}
}
#if !defined(RA2_YRPP_GAME)
DynamicVectorClass<ParasiteClass*>& ParasiteClass::Array=parasites;
DynamicVectorClass<ParasiteClass*>& ParasiteClass::AnimExpirationListeners=animListeners;
#endif
ParasiteClass::ParasiteClass(FootClass* owner) noexcept:AbstractClass(),Owner(owner),Victim(nullptr),
    SuppressionTimer{},DamageDeliveryTimer{},GrappleAnim(nullptr),GrappleState(ParasiteState::Start),
    GrappleAnimFrame(0),GrappleAnimDelay(0),GrappleAnimGotInvalid(false){
    SuppressionTimer.Start(0);DamageDeliveryTimer.Start(0);
    if(!Array.AddItem(this))std::abort();
}
ParasiteClass::~ParasiteClass(){reset_grapple(*this);Array.Remove(this);}
HRESULT YRPP_STDCALL ParasiteClass::GetClassID(CLSID* output){
    if(!output)return static_cast<HRESULT>(0x80004003u);
    constexpr unsigned words[]{0x1D016B81,0x11D3B24B,0x100016BE,0x6CA1624B};std::memcpy(output,words,sizeof(words));return 0;
}
bool ParasiteClass::CanInfect(FootClass* target) const {
    if(!target||target->InLimbo||!target->IsAlive||!target->Health||target->ParasiteEatingMe
        ||!target->GetTechnoType()->Parasiteable||target->BunkerLinkedItem)return false;
    return !Owner||!Owner->GetTechnoType()->Naval||!target->GetCell()||target->GetCell()->Tile_Is_Water();
}
void ParasiteClass::TryInfect(FootClass* target){
    reset_grapple(*this);DamageDeliveryTimer.Start(0);
    if(CanInfect(target)){
        Owner->Locomotor->Force_Track(-1,target->Location);
        target->ParasiteEatingMe=Owner;Victim=target;
    }else {
        const auto at=MapClass::Instance.GetCellAt(Owner->LastMapCoords)->GetCoords();
        if(Owner->Unlimbo(at,DirType::North)){
            Owner->vt_entry_48C(false,0,false,nullptr);
            auto where=Owner->GetCoords();MapClass::Instance.RevealArea3(&where,Owner->LastSightRange-3,Owner->LastSightRange+2,false);
            if(Owner->MegaMission==Mission::None){Owner->SetTarget(nullptr);Owner->SetDestination(nullptr,true);}
            Owner->EnterIdleMode(false,true);
        }else Owner->UnInit();
    }
}
void ParasiteClass::Update(){
    if(!Victim)return;
    auto* type=Owner->GetTechnoType();if(type->Naval&&type->Organic){UpdateSquid();return;}
    auto* weapon=Owner->GetWeapon(0)->WeaponType;
    if(DamageDeliveryTimer.GetTimeLeft())return;
    DamageDeliveryTimer.Start(weapon->ROF);Victim->ParalysisTimer.Start(weapon->Warhead->Paralyzes);
    if(Victim->WhatAmI()==AbstractType::Infantry){
        int lethal=Victim->Health;
        Victim->ReceiveDamage(&lethal,0,weapon->Warhead,Owner,true,true,nullptr);
    }else {
        // The shared non-infantry parasite branch remains on its original
        // particle/rocking dependencies; dogs' verses reject these targets.
        const auto at=Victim->Location;
        if(void* memory=YRMemory::Allocate(sizeof(ParticleSystemClass)))
            ::new(memory) ParticleSystemClass(RulesClass::Instance->DefaultSparkSystem,at,nullptr,nullptr,CoordStruct::Empty,nullptr);
        const auto facing=Victim->PrimaryFacing.Current();
        if(weapon->Anim.Count)if(auto* animation=weapon->Anim[facing.GetValue<3>()])
            if(void* memory=YRMemory::Allocate(sizeof(AnimClass)))::new(memory) AnimClass(animation,at,0,1,0x600,0,false);
        const int radius=ScenarioClass::Instance->Random.RandomRanged(0,1)?-2:2;
        const short turned=short(facing.Raw-0x3FFF);const double angle=(int(turned)-0x3FFF)*-0.00009587672516830327;
        const CoordStruct impact{rule_integer(at.X+Math::cos(angle)*radius),rule_integer(at.Y-Math::sin(angle)*radius),at.Z};
        Victim->vt_entry_3D8(&impact,1.5f,false);
        int damage=weapon->Damage;Victim->ReceiveDamage(&damage,0,weapon->Warhead,Owner,false,true,nullptr);
    }
}
bool ParasiteClass::CanExistOnVictimCell() const {
    if(!Victim||Victim->IsInAir())return false;
    auto* cell=Victim->GetCell();
    if(!Owner->GetTechnoType()->Naval){
        if(Owner->WhatAmI()==AbstractType::Unit&&cell->GetTerrain(false))return false;
        if(water_without_bridge(*cell,*cell))return false;
    }
    return !cell->GetBuilding();
}
CoordStruct* ParasiteClass::GetExitCoords(CoordStruct* output){
    *output=CoordStruct::Empty;if(!Victim)return output;
    auto* from=Victim->GetCell();
    if(CanExistOnVictimCell()){
        *output=Victim->Location;
        if(Owner&&Owner->WhatAmI()==AbstractType::Unit){*output=from->GetCoords();if(Victim->OnBridge)output->Z+=CellClass::BridgeHeight;}
        Owner->OnBridge=Victim->OnBridge;return output;
    }
    const auto delta=Unsorted::AdjacentCell[Victim->PrimaryFacing.Current().GetValue<3>()];
    auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(from->MapCoords.X+delta.X),short(from->MapCoords.Y+delta.Y)});
    if(!Owner->GetTechnoType()->Naval&&water_without_bridge(*cell,*from))return output;
    auto at=cell->FindInfantrySubposition(cell->GetCoords(),false,false,false);
    if(Owner&&Owner->WhatAmI()==AbstractType::Unit)at=MapClass::Instance.GetCellAt(at)->GetCoords();
    Owner->OnBridge=false;
    if(unsigned(MapClass::Instance.GetCellAt(at)->Flags)&0x100){
        if(Victim->OnBridge||at.Z<from->GetCoords().Z){at.Z+=CellClass::BridgeHeight;Owner->OnBridge=true;}
    }
    *output=at;return output;
}
void ParasiteClass::PointerExpired(AbstractClass* object,bool){
    if(object==Owner){Owner=nullptr;return;}
    if(object==Victim){
        if(!Unsorted::ScenarioStarted){Victim=nullptr;return;}
        if(SuppressionTimer.GetTimeLeft()>0&&!Owner->IsIronCurtained()){
            Victim->ParasiteEatingMe=nullptr;Victim=nullptr;Owner->UnInit();return;
        }
        CoordStruct at;GetExitCoords(&at);
        ++Unsorted::ScenarioInit;
        const auto facing=DirType(Victim->PrimaryFacing.Current().GetValue<8>());
        const bool returned=at!=CoordStruct::Empty&&Owner->Unlimbo(at,facing);
        --Unsorted::ScenarioInit;
        if(!returned){Owner->Health=0;Owner->UnInit();Victim=nullptr;return;}
        resume_owner(*Owner,true);Victim=nullptr;return;
    }
    if(object==GrappleAnim){GrappleAnim=nullptr;GrappleAnimGotInvalid=true;}
}
void ParasiteClass::ExitUnit(){
    if(!Victim)return;
    const bool naval=Owner->GetTechnoType()->Naval;
    if(!naval&&SuppressionTimer.GetTimeLeft()){
        Owner->Health=0;Victim->ParalysisTimer.Start(0);Victim->ParasiteEatingMe=nullptr;
        Owner->UnInit();Victim=nullptr;return;
    }
    const auto victimFacing=Victim->PrimaryFacing.Current();
    const short facing=short(victimFacing.Raw+(victimFacing.GetValue<3>()>2?-0x3FFF:0x3FFF));
    auto where=Victim->GetMapCoords();
    if(naval){const auto delta=Unsorted::AdjacentCell[DirStruct(facing).GetValue<3>()];where.X+=delta.X;where.Y+=delta.Y;}
    auto* type=Owner->GetTechnoType();auto& map=MapClass::Instance;
    if(!map.GetCellAt(where)->IsClearToMove(type->SpeedType,false,false,map.GetMovementZoneType(where,type->MovementZone,false),type->MovementZone,-1,true))
        Owner->NearbyLocation(&where,nullptr);
    const auto at=where==CellStruct::Empty?CoordStruct::Empty:map.GetCellAt(where)->GetCoords();
    if(at==CoordStruct::Empty||!CanExistOnVictimCell()||!Owner->Unlimbo(at,DirType(DirStruct(facing).GetValue<8>()))){Owner->Health=0;Owner->UnInit();}
    else {resume_owner(*Owner,true);Owner->ParalysisTimer.Start(3*Owner->GetWeapon(0)->WeaponType->ROF);reset_grapple(*this);}
    Victim->AngleRotatedSideways=0;Victim->ParalysisTimer.Start(0);Victim->ParasiteEatingMe=nullptr;Victim=nullptr;
}
