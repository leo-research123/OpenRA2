// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 teleport.cpp, calibrated to YR 0x718000..0x71A1B0.
// Native ordinary teleport arm, including the harvester's zero-delay return.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include <cstring>
#include <cmath>

TeleportLocomotionClass::TeleportLocomotionClass() noexcept
 :MovingDestination{},LastCoords{},Moving(false),unknown_bool_35(false),unknown_bool_36(false),State(0),Timer{},Piggybackee(nullptr){Timer.Start(0);}
TeleportLocomotionClass::~TeleportLocomotionClass(){if(Piggybackee)Piggybackee->Release();}
HRESULT YRPP_STDCALL TeleportLocomotionClass::QueryInterface(REFIID iid,void** output){
 const auto result=LocomotionClass::QueryInterface(iid,output);if(result!=static_cast<HRESULT>(0x80004002u))return result;
 constexpr GUID piggy{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
 if(std::memcmp(&iid,&piggy,sizeof(iid)))return result;*output=static_cast<IPiggyback*>(this);AddRef();return 0;
}
ULONG YRPP_STDCALL TeleportLocomotionClass::AddRef(){return LocomotionClass::AddRef();}
ULONG YRPP_STDCALL TeleportLocomotionClass::Release(){return LocomotionClass::Release();}
HRESULT YRPP_STDCALL TeleportLocomotionClass::GetClassID(CLSID* output){if(!output)return static_cast<HRESULT>(0x80004003u);*output=CLSIDs::Teleport;return 0;}
HRESULT YRPP_STDCALL TeleportLocomotionClass::Begin_Piggyback(ILocomotion* pointer){if(!pointer)return static_cast<HRESULT>(0x80004003u);if(Piggybackee)return static_cast<HRESULT>(0x80004005u);Piggybackee=pointer;pointer->AddRef();return 0;}
HRESULT YRPP_STDCALL TeleportLocomotionClass::End_Piggyback(ILocomotion** pointer){if(!pointer)return static_cast<HRESULT>(0x80004003u);if(!Piggybackee)return 1;*pointer=Piggybackee;Piggybackee=nullptr;return 0;}
bool YRPP_STDCALL TeleportLocomotionClass::Is_Piggybacking(){return Piggybackee!=nullptr;}
bool YRPP_STDCALL TeleportLocomotionClass::Is_Ok_To_End(){return !Moving&&Piggybackee&&!unknown_bool_35&&!State&&!LinkedTo->IsBeingWarpedOut()&&!LinkedTo->IsAttackedByLocomotor;}
HRESULT YRPP_STDCALL TeleportLocomotionClass::Piggyback_CLSID(GUID* id){
 if(!id)return static_cast<HRESULT>(0x80004003u);if(!Piggybackee){*id=CLSIDs::Teleport;return 0;}
 constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};IPersist* persist=nullptr;
 if(Piggybackee->QueryInterface(iid,reinterpret_cast<void**>(&persist))<0||!persist)return static_cast<HRESULT>(0x80004005u);
 const auto result=persist->GetClassID(id);persist->Release();return result;
}
bool YRPP_STDCALL TeleportLocomotionClass::Is_Moving(){return Moving;}
CoordStruct YRPP_STDCALL TeleportLocomotionClass::Destination(){return Moving?MovingDestination:LinkedTo->Location;}
void YRPP_STDCALL TeleportLocomotionClass::Stop_Moving(){MovingDestination=CoordStruct::Empty;Moving=false;unknown_bool_36=false;}
void YRPP_STDCALL TeleportLocomotionClass::Do_Turn(DirStruct dir){LinkedTo->PrimaryFacing.SetCurrent(dir);}
Layer YRPP_STDCALL TeleportLocomotionClass::In_Which_Layer(){return Layer::Ground;}
void YRPP_STDCALL TeleportLocomotionClass::Mark_All_Occupation_Bits(MarkType mark){if(mark==MarkType::Up)LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);}
void YRPP_STDCALL TeleportLocomotionClass::Move_To(CoordStruct to){
 if(LinkedTo->IsBeingWarpedOut()||LinkedTo->IsWarpingIn()){LinkedTo->Destination=nullptr;return;}
 auto& map=MapClass::Instance;auto* cell=map.TryGetCellAt(to);
 if(!cell||!map.IsWithinUsableArea(cell->MapCoords,true))return;
 if(LinkedTo->IsCellOccupied(cell,FacingType(-1),-1,nullptr,false)!=Move::OK){
  const auto next=map.NearByLocation(cell->MapCoords,LinkedTo->GetTechnoType()->SpeedType,-1,LinkedTo->GetTechnoType()->MovementZone,false,1,1,false,true,false,true,CellStruct::Empty,false,false);
  if(next==CellStruct::Empty){LinkedTo->Destination=nullptr;return;}cell=map.GetCellAt(next);
 }
 LastCoords=MovingDestination=cell->GetCoords();MovingDestination.Z=LastCoords.Z=map.GetCellFloorHeight(LastCoords);Moving=true;
}
bool YRPP_STDCALL TeleportLocomotionClass::Process(){
 if(!Moving)return false;
 const auto from=LinkedTo->Location,to=LastCoords;
 if(from==to||to==CoordStruct::Empty){Stop_Moving();LinkedTo->Destination=nullptr;return false;}
 // In YR's normal teleport arm the position changes in this update. Miners
 // explicitly clear the distance-based materialization delay.
 // 0x7192F0 uses Rules +0x33C (WarpOut) at BOTH ends. WarpIn is
 // +0x338 and names different art; it is not the miner's arrival effect.
 const auto effect=[&](CoordStruct at){if(auto* type=RulesClass::Instance->WarpOut)if(void* memory=YRMemory::Allocate(sizeof(AnimClass)))::new(memory) AnimClass(type,at,0,1,0x600,0,false);};
 effect(from);LinkedTo->Mark(MarkType::Up);LinkedTo->SetLocation(to);
 LinkedTo->OnBridge=(unsigned(MapClass::Instance.GetCellAt(to)->Flags)&0x100u)!=0;
 LinkedTo->Mark(MarkType::Down);Stop_Moving();LinkedTo->Destination=nullptr;
 LinkedTo->UpdatePosition(PCPType::End);effect(to);return false;
}
