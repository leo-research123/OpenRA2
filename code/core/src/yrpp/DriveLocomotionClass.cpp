// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 drive.cpp; YR 0x4AF540..0x4B4DE0 COM and drive state.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

DriveLocomotionClass::DriveLocomotionClass() noexcept
 :CurrentRamp(0),PreviousRamp(0),SlopeTimer{},DestinationCoord{},HeadToCoord{},SpeedAccum(0),
 movementspeed_50(0),TrackNumber(-1),TrackIndex(-1),IsOnShortTrack(false),IsTurretLockedDown(0),
 IsRotating(false),IsDriving(false),IsRocking(false),UnLocked(true),Piggybackee(nullptr){SlopeTimer.Start(0);}
DriveLocomotionClass::~DriveLocomotionClass(){if(Piggybackee)Piggybackee->Release();}
HRESULT YRPP_STDCALL DriveLocomotionClass::QueryInterface(REFIID iid,void** output){
 const auto result=LocomotionClass::QueryInterface(iid,output);
 if(result!=static_cast<HRESULT>(0x80004002u))return result;
 constexpr GUID piggy{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
 if(std::memcmp(&iid,&piggy,sizeof(iid)))return result;
 *output=static_cast<IPiggyback*>(this);AddRef();return 0;
}
ULONG YRPP_STDCALL DriveLocomotionClass::AddRef(){return LocomotionClass::AddRef();}
ULONG YRPP_STDCALL DriveLocomotionClass::Release(){return LocomotionClass::Release();}
HRESULT YRPP_STDCALL DriveLocomotionClass::GetClassID(CLSID* output){if(!output)return static_cast<HRESULT>(0x80004003u);*output=CLSIDs::Drive;return 0;}
HRESULT YRPP_STDCALL DriveLocomotionClass::Begin_Piggyback(ILocomotion* pointer){
 if(!pointer)return static_cast<HRESULT>(0x80004003u);
 if(Piggybackee)return static_cast<HRESULT>(0x80004005u);
 Piggybackee=pointer;try{pointer->AddRef();return 0;}catch(...){Piggybackee=nullptr;return static_cast<HRESULT>(0x80004005u);}
}
HRESULT YRPP_STDCALL DriveLocomotionClass::End_Piggyback(ILocomotion** pointer){
 if(!pointer)return static_cast<HRESULT>(0x80004003u);
 if(!Piggybackee)return 1;
 *pointer=Piggybackee;Piggybackee=nullptr;return 0;
}
bool YRPP_STDCALL DriveLocomotionClass::Is_Ok_To_End(){return !Is_Moving()&&Piggybackee&&UnLocked&&!LinkedTo->IsAttackedByLocomotor;}
HRESULT YRPP_STDCALL DriveLocomotionClass::Piggyback_CLSID(GUID* id){
 if(!id)return static_cast<HRESULT>(0x80004003u);
 constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};IPersist* persist=nullptr;
 try{
  auto* object=Piggybackee?Piggybackee:static_cast<ILocomotion*>(this);
  if(object->QueryInterface(iid,reinterpret_cast<void**>(&persist))<0||!persist)return static_cast<HRESULT>(0x80004005u);
  auto result=persist->GetClassID(id);auto* held=persist;persist=nullptr;held->Release();return result;
 }catch(...){if(persist)try{persist->Release();}catch(...){}return static_cast<HRESULT>(0x80004005u);}
}
bool YRPP_STDCALL DriveLocomotionClass::Is_Piggybacking(){return Piggybackee!=nullptr;}
bool YRPP_STDCALL DriveLocomotionClass::Is_Moving(){
 return DestinationCoord!=CoordStruct::Empty || (HeadToCoord!=CoordStruct::Empty
  &&(HeadToCoord.X!=LinkedTo->Location.X||HeadToCoord.Y!=LinkedTo->Location.Y));
}
bool YRPP_STDCALL DriveLocomotionClass::Is_Moving_Now(){return LinkedTo->PrimaryFacing.IsRotating()
 ||(Is_Moving()&&HeadToCoord!=CoordStruct::Empty&&LinkedTo->GetCurrentSpeed()>0);}
bool YRPP_STDCALL DriveLocomotionClass::Is_Really_Moving_Now(){return Is_Moving_Now();}
CoordStruct YRPP_STDCALL DriveLocomotionClass::Destination(){return DestinationCoord;}
CoordStruct YRPP_STDCALL DriveLocomotionClass::Head_To_Coord(){return HeadToCoord!=CoordStruct::Empty?HeadToCoord:LinkedTo->Location;}
void YRPP_STDCALL DriveLocomotionClass::Move_To(CoordStruct to){
 if(LinkedTo->IsUnderEMP()||LinkedTo->IsParalyzed()||LinkedTo->IsBeingWarpedOut()||LinkedTo->IsWarpingIn())return;
 DestinationCoord=to;
 if(to!=CoordStruct::Empty&&(unsigned(MapClass::Instance.GetCellAt(to)->Flags)&0x100u))DestinationCoord.Z+=CellClass::BridgeHeight;
}
void YRPP_STDCALL DriveLocomotionClass::Stop_Moving(){
 if(HeadToCoord!=CoordStruct::Empty&&LinkedTo->GetTechnoType()->IsTrain){
  auto* unit=static_cast<UnitClass*>(LinkedTo);
  if(!unit->IsFollowerCar)for(auto* next=unit->FollowerCar;next;){
   next->Locomotor->Stop_Moving();next=next->FollowerCar;if(next&&next==next->FollowerCar)break;
  }
 }
 movementspeed_50=std::min(movementspeed_50,double(0.3f));DestinationCoord=CoordStruct::Empty;
}
void YRPP_STDCALL DriveLocomotionClass::Do_Turn(DirStruct dir){LinkedTo->PrimaryFacing.SetDesired(dir);}
void DriveLocomotionClass::Set_Slope(int ramp){if(ramp!=CurrentRamp){PreviousRamp=CurrentRamp;CurrentRamp=ramp;SlopeTimer.Start(3);}}
void YRPP_STDCALL DriveLocomotionClass::Force_New_Slope(int ramp){PreviousRamp=CurrentRamp=ramp;SlopeTimer.Start(0);}
void YRPP_STDCALL DriveLocomotionClass::Unlimbo(){Force_New_Slope(LinkedTo->GetCell()->SlopeIndex);}
Layer YRPP_STDCALL DriveLocomotionClass::In_Which_Layer(){return Layer::Ground;}
int YRPP_STDCALL DriveLocomotionClass::Z_Adjust(){return 0;}
ZGradient YRPP_STDCALL DriveLocomotionClass::Z_Gradient(){return LocomotionClass::Z_Gradient();}
void YRPP_STDCALL DriveLocomotionClass::Lock(){UnLocked=false;}
void YRPP_STDCALL DriveLocomotionClass::Unlock(){UnLocked=true;}
int YRPP_STDCALL DriveLocomotionClass::Get_Track_Number(){return TrackNumber;}
int YRPP_STDCALL DriveLocomotionClass::Get_Track_Index(){return TrackIndex;}
int YRPP_STDCALL DriveLocomotionClass::Get_Speed_Accum(){return SpeedAccum;}

bool DriveLocomotionClass::Stop_Driver(){if(HeadToCoord==CoordStruct::Empty)return false;HeadToCoord=CoordStruct::Empty;IsDriving=false;return true;}
bool DriveLocomotionClass::Start_Driver(const CoordStruct& head){
 Stop_Driver();if(head==CoordStruct::Empty)return false;
 HeadToCoord=head;IsDriving=true;
 if(MapClass::Instance.GetCellAt(head)->CollectCrate(LinkedTo)&&!LinkedTo->InLimbo){Mark_Track(head,MarkType::Down);return true;}
 if(!LinkedTo->IsAlive)return false;
 HeadToCoord=CoordStruct::Empty;IsDriving=false;return false;
}
bool DriveLocomotionClass::Abandon_Navigation(){
 if(!LinkedTo->NavQueue.Count){LinkedTo->SetDestination(nullptr,true);return false;}
 // 0x4DF0D0 clears the two navigation pointers, not NavQueue itself.
 LinkedTo->unknown_5A0=nullptr;LinkedTo->Destination=nullptr;
 return LinkedTo->EnterIdleMode(false,true);
}
void YRPP_STDCALL DriveLocomotionClass::Force_Track(int track,CoordStruct coord){
 TrackNumber=track;TrackIndex=0;
 if(coord!=CoordStruct::Empty&&Start_Driver(coord)){DestinationCoord=coord;movementspeed_50=1.0;}
}
Point2D DriveLocomotionClass::Smooth_Turn(const Point2D& offset,int& direction){
 auto point=offset;unsigned face=unsigned(direction);const int flags=TurnTrack[TrackNumber].Flag;
 if(flags&1){std::swap(point.X,point.Y);face=(-64u-face)&255u;}
 if(flags&2){point.X=-point.X;face=(0u-face)&255u;}
 if(flags&4){point.Y=-point.Y;face=(128u-face)&255u;}
 direction=int(face);
 point.X+=HeadToCoord.X;point.Y+=HeadToCoord.Y;return point;
}
void DriveLocomotionClass::Mark_Track(const CoordStruct& head,MarkType mark){
 if(head==CoordStruct::Empty)return;
 const auto apply=[&](const CoordStruct& at){if(mark==MarkType::Up)LinkedTo->UnmarkAllOccupationBits(at);
  else if(mark==MarkType::Down||mark==MarkType::ChangeRedraw)LinkedTo->MarkAllOccupationBits(at);};
 if(!IsOnShortTrack&&TrackNumber!=-1){
  const int raw=TurnTrack[TrackNumber].NormalTrackStructIndex;
  if(raw&&RawTrack[raw].CellIndex>-1&&TrackIndex<RawTrack[raw].CellIndex){
   const auto& point=RawTrack[raw].TrackPoint[RawTrack[raw].CellIndex];
   int face=point.Face;
   const auto at=Smooth_Turn(point.Point,face);apply({at.X,at.Y,LinkedTo->GetZ()});
  }
 }
 apply(head);
}
void YRPP_STDCALL DriveLocomotionClass::Mark_All_Occupation_Bits(MarkType mark){if(HeadToCoord!=CoordStruct::Empty)Mark_Track(HeadToCoord,mark);}
bool YRPP_STDCALL DriveLocomotionClass::Is_Moving_Here(CoordStruct to){
 const auto head=Head_To_Coord();if(head==CoordStruct::Empty)return false;
 const auto same=[&](const CoordStruct& at){return short(at.X/256)==short(to.X/256)&&short(at.Y/256)==short(to.Y/256)
  &&std::abs(static_cast<long long>(at.Z)-to.Z)<=Unsorted::LevelHeight;};
 if(!IsOnShortTrack&&TrackNumber!=-1){const int raw=TurnTrack[TrackNumber].NormalTrackStructIndex;
  if(raw&&RawTrack[raw].CellIndex>-1&&TrackIndex<RawTrack[raw].CellIndex){
   const auto& point=RawTrack[raw].TrackPoint[RawTrack[raw].CellIndex];int face=point.Face;
   const auto at=Smooth_Turn(point.Point,face);if(same({at.X,at.Y,LinkedTo->Location.Z}))return true;
  }
 }
 return same(head);
}
bool YRPP_STDCALL DriveLocomotionClass::Will_Jump_Tracks(){
 const int next=LinkedTo->PathDirections[0];if(next<0||next>=8)return false;
 const auto& control=TurnTrack[TrackNumber];const int raw=IsOnShortTrack?control.ShortTrackStructIndex:control.NormalTrackStructIndex;
 const int face=((unsigned(control.Face)+16u)>>5)&7u;
 if(face==next||!TrackIndex||RawTrack[raw].JumpIndex!=TrackIndex)return false;
 const int nextRaw=TurnTrack[face*8+next].NormalTrackStructIndex;return nextRaw&&RawTrack[nextRaw].EntryIndex;
}
