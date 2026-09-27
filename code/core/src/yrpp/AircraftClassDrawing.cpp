// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 aircraft.cpp::Draw_It; YR 0x004144B0.
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "techno_drawing.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "Matrix3DArithmetic.hpp"
#include <bit>

void AircraftClass::DrawIt(Point2D* location,RectangleStruct* bounds) const {
 auto* d=game::techno_drawing();if(!d)return;
 if(!location||!bounds||!Type||!Locomotor||!d->height){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
 auto& self=*const_cast<AircraftClass*>(this);
 if(!d->debug_map&&d->window_active&&d->fog_of_war&&d->fogged){
  auto destination=Locomotor->Head_To_Coord();destination.Z=Location.Z;
  if(d->fogged(d->context,destination)&&d->fogged(d->context,Location)&&(!Owner||!Owner->IsControlledByCurrentPlayer()))return;
 }
 auto* apparent=static_cast<TechnoTypeClass*>(Type);
 if(!IsClearlyVisibleTo(d->player)){auto* disguise=GetDisguise(true);if(disguise&&disguise->WhatAmI()==AbstractType::AircraftType)apparent=static_cast<AircraftTypeClass*>(disguise);}
 if(!apparent->MainVoxel.HVA||apparent->MainVoxel.HVA->FrameCount<=0)return;
 const unsigned frame=unsigned(WalkedFramesSoFar)%unsigned(apparent->MainVoxel.HVA->FrameCount);
 auto point=*location;const auto offset=Locomotor->Draw_Point();point.X+=offset.X;point.Y+=offset.Y;
 if(Passengers.NumPassengers&&Type->Carryall&&Passengers.FirstPassenger)Passengers.FirstPassenger->DrawIt(&point,bounds);
 if(!game::techno_drawing_complete(d->status)||!Type->Voxel||!Type->MainVoxel.VXL)return;
 auto ground=Location;ground.Z=d->ground_height?d->ground_height(d->context,Location):Location.Z-GetHeight();
 auto shadow=d->project?d->project(d->context,ground):Point2D{location->X,location->Y+d->height(GetHeight())};
 const auto shadow_offset=Locomotor->Shadow_Point();shadow.X+=shadow_offset.X;shadow.Y+=shadow_offset.Y;
 const int saved_height=GetHeight();const bool saved_frozen=FrozenStill;
 struct Restore {AircraftClass& object;int height;bool frozen;~Restore(){object.SetHeight(DWORD(height));object.FrozenStill=frozen;}} restore{self,saved_height,saved_frozen};
 self.FrozenStill=false;
 auto* cell=d->cell_at_world?d->cell_at_world(d->context,Location):nullptr;
 if(!cell){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
 bool raised=false;
 if((!OnBridge&&saved_height>=d->bridge_height)||(OnBridge&&saved_height>=0)){
  const auto flags=unsigned(cell->Flags);
  if(flags&0x100u){const auto* neighbor=cell->GetNeighbourCell((flags&0x800u)?FacingType::North:FacingType::West);raised=neighbor&&(unsigned(neighbor->Flags)&0x100u);}
 }
 self.SetHeight(raised?DWORD(d->bridge_height):0);if(raised)shadow.Y-=d->height(d->bridge_height);
 VoxelIndexKey key(0);auto local=Locomotor->Shadow_Matrix(&key);auto matrix=local;
 if(!Type->NoShadow){game::VoxelMatrixScope scope(*d,local);matrix=drawing_matrix_product(d->camera,local);self.DrawVoxelShadow(&Type->MainVoxel,Type->ShadowIndex,key,&Type->VoxelShadowCache,bounds,&shadow,&matrix,true,nullptr,{});}
 self.SetHeight(DWORD(saved_height));self.FrozenStill=saved_frozen;
 if(!game::techno_drawing_complete(d->status))return;
 if(d->selectable)d->selectable(d->context,self,point);
 const int intensity=std::bit_cast<short>(cell->Intensity_Normal)+d->level_light*(GetHeight()/(2*d->level_height))+d->extra_aircraft_light;
 key.Value=0;local=Locomotor->Draw_Matrix(&key);
 if(key.Is_Valid_Key())key.Value=std::bit_cast<int>((unsigned(key.Value)<<5)|(frame&0x1Fu));
 if(Type->DisableVoxelCache)key.Invalidate();
 matrix=drawing_matrix_product(d->camera,local);game::VoxelMatrixScope scope(*d,local);
 self.Draw_A_VXL(&Type->MainVoxel,int(frame),key.Value,&Type->VoxelMainCache,bounds,&point,&matrix,intensity,static_cast<BlitterFlags>(0),0);
}
