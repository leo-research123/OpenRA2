// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aircraft.cpp lifecycle; YR 0x413D20/0x413F80/0x414310.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "map_world.hpp"
#include "target_registry.hpp"
#include <cstring>

AircraftClass::AircraftClass(AircraftTypeClass* type,HouseClass* owner) noexcept
 :FootClass(owner),Type(type),ShouldLoseAmmo(false),HasPassengers(false),IsKamikaze(false),DockNowHeadingTo(nullptr),
 unknown_bool_6D0(false),unknown_bool_6D1(false),IsLocked(false),NumParadropsLeft(5),IsCarryallNotLanding(true),IsReturningFromAttackRun(true){
    if(Type){
        PrimaryFacing.SetROT(Type->ROT);SecondaryFacing.SetROT(Type->ROT);
        SecondaryFacing.SetCurrent(PrimaryFacing.Current());
        Ammo=Type->InitialAmmo==-1?Type->Ammo:Type->InitialAmmo;
        Health=EstimatedHealth=Type->Strength;
        InitializeLocomotor();
        if(Owner){Owner->AddTracking(this);if(Owner->Type->VeteranAircraft.FindItemIndex(Type)>=0)Veterancy.SetVeteran();}
    }
    Array.AddItem(this);game::register_target_identity(*this);
}
AircraftClass::~AircraftClass(){
    if(Team)Team->LiberateMember(this);
    if(Type&&Owner&&CountedAsOwned)Owner->RemoveTracking(this);
    IsAlive=false;NotifyObjectExpired(true);game::detach_map_object(*this);
    Array.Remove(this);game::unregister_target_identity(*this);
}
void AircraftClass::PointerExpired(AbstractClass* object,bool removed){
    FootClass::PointerExpired(object,removed);
    if(reinterpret_cast<AbstractClass*>(DockNowHeadingTo)==object)DockNowHeadingTo=nullptr;
    if(Type==object)Type=nullptr;
}
ObjectClass* AircraftTypeClass::CreateObject(HouseClass* owner){return GameCreate<AircraftClass>(this,owner);}
ObjectTypeClass* AircraftClass::GetType() const{return Type;}
bool AircraftClass::InitializeLocomotor() noexcept {
    if(Locomotor)return true;
    if(!Type||std::memcmp(&Type->Locomotor,&LocomotionClass::CLSIDs::Fly,sizeof(GUID)))return false;
    try{
        auto* fly=GameCreate<FlyLocomotionClass>();if(!fly)return false;
        fly->Link_To_Object(this);
#if defined(_MSC_VER)
        Locomotor=static_cast<ILocomotion*>(fly);
#else
        fly->AddRef();Locomotor=fly;
#endif
        return true;
    }catch(...){return false;}
}
HRESULT YRPP_STDCALL AircraftClass::QueryInterface(REFIID iid,void** output){
    constexpr GUID fly{0x820F501C,0x4F39,0x11D2,{0x9B,0x70,0,0x10,0x4B,0x97,0x2F,0xE8}};
    if(!output)return static_cast<HRESULT>(0x80004003u);
    if(!std::memcmp(&iid,&fly,sizeof(iid))){*output=static_cast<IFlyControl*>(this);AddRef();return 0;}
    return AbstractClass::QueryInterface(iid,output);
}
ULONG YRPP_STDCALL AircraftClass::AddRef(){return AbstractClass::AddRef();}
ULONG YRPP_STDCALL AircraftClass::Release(){return AbstractClass::Release();}
HRESULT YRPP_STDCALL AircraftClass::GetClassID(CLSID* output){
    if(!output)return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[]{237448642u,298949647u,2684357047u,3517963556u};
    std::memcpy(output,words,sizeof(words));return 0;
}
int YRPP_STDCALL AircraftClass::Landing_Altitude(){return Type->Carryall&&(Passengers.NumPassengers||HasAnyLink())?100:0;}
int YRPP_STDCALL AircraftClass::Landing_Direction(){return HasAnyLink()||Passengers.NumPassengers?int(PrimaryFacing.Current().GetValue<3>()):RulesClass::Instance->PoseDir;}
long YRPP_STDCALL AircraftClass::Is_Loaded(){return Passengers.NumPassengers!=0;}
long YRPP_STDCALL AircraftClass::Is_Strafe(){auto* weapon=GetWeapon(0);return weapon&&weapon->WeaponType&&weapon->WeaponType->Projectile&&weapon->WeaponType->Projectile->ROT<=1&&!weapon->WeaponType->Projectile->Inviso;}
long YRPP_STDCALL AircraftClass::Is_Fighter(){return Type->Fighter;}
long YRPP_STDCALL AircraftClass::Is_Locked(){return IsLocked;}
bool AircraftClass::Unlimbo(const CoordStruct& location,DirType facing){
    if(!InitializeLocomotor())return false;
    auto at=location;
    if(!Type->BalloonHover)at.Z=MapClass::Instance.GetCellFloorHeight(at)+
        ((IsALoaner||!MapClass::Instance.IsWithinUsableArea(CellClass::Coord2Cell(at),true))?Type->GetFlightLevel():0);
    if(!FootClass::Unlimbo(at,facing))return false;
    const auto* weapon=GetWeapon(0);
    if(!Type->Selectable||!Type->Landable||(weapon&&weapon->WeaponType&&weapon->WeaponType->LimboLaunch))IsALoaner=true;
    if(Passengers.NumPassengers)HasPassengers=true;
    PrimaryFacing.SetCurrent(DirStruct(int(facing)<<8));SecondaryFacing.SetCurrent(PrimaryFacing.Current());
    SetSpeedPercentage(GetHeight()==Type->GetFlightLevel()?1.0:0.0);
    return true;
}
