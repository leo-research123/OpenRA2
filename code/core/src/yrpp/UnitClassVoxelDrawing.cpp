// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 unit.cpp::Unit_Draw_Voxel; calibrated to YR 0x0073B470.
#include "yrpp/UnitClass.h"
#include "techno_drawing.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "Matrix3DArithmetic.hpp"
#include <bit>
namespace {
bool ready(const VoxelStruct& v){return v.VXL&&!v.VXL->Initialized&&v.VXL->CountHeaders&&v.VXL->BodyData&&v.HVA&&!v.HVA->LoadedFailed&&v.HVA->Matrixes&&v.HVA->FrameCount>0&&v.HVA->LayerCount>=int(v.VXL->CountHeaders);}
}
void UnitClass::DrawAsVXL(Point2D point,RectangleStruct clip,int intensity,int tint) {
    auto* d=game::techno_drawing();if(!d||!Type||!Locomotor)return;
    auto* type=Type;
    if(!IsClearlyVisibleTo(d->player)){auto* disguise=GetDisguise(true);if(disguise&&disguise->WhatAmI()==AbstractType::UnitType)type=static_cast<UnitTypeClass*>(disguise);}
    if(!type->Voxel)return;
    if(IsIronCurtained())tint|=int(d->iron_tint);
    if(Berzerk)tint|=int(d->berserk_tint);
    if(auto* cell=game::techno_drawing_cell(*d,*this);cell&&d->shrouded&&d->shrouded(*cell))tint=0;
    VoxelIndexKey key(0);auto body=Locomotor->Draw_Matrix(&key);
    const unsigned body_frame=ready(type->MainVoxel)?unsigned(WalkedFramesSoFar)%unsigned(type->MainVoxel.HVA->FrameCount):0;
    if(key.Is_Valid_Key())key.Value=std::bit_cast<int>((unsigned(key.Value)<<5)|(body_frame&0x1F));
    if(type->DisableVoxelCache)key.Invalidate();
    if(!type->NoShadow&&ready(type->MainVoxel)){
        auto local=Locomotor->Shadow_Matrix(nullptr);auto matrix=drawing_matrix_product(d->camera,local);
        game::VoxelMatrixScope scope(*d,local);
        DrawVoxelShadow(&type->MainVoxel,type->ShadowIndex,VoxelIndexKey(-1),&type->VoxelShadowCache,&clip,&point,&matrix,true,nullptr,{});
    }
    const auto draw=[&](VoxelStruct& voxel,const Matrix3D& local,unsigned frame,IndexClass<VoxelIndexKey,VoxelCacheStruct*>& cache){
        if(!ready(voxel)||!game::techno_drawing_complete(d->status))return;
        auto matrix=drawing_matrix_product(d->camera,local);
        game::VoxelMatrixScope scope(*d,local);
        Draw_A_VXL(&voxel,int(frame%unsigned(voxel.HVA->FrameCount)),key.Value,&cache,&clip,&point,&matrix,intensity,static_cast<BlitterFlags>(0),DWORD(tint));
    };
    draw(type->MainVoxel,body,unsigned(WalkedFramesSoFar),type->VoxelMainCache);
    if(!type->Turret)return;
    const bool multiple=type->HasMultipleTurrets()&&!type->IsGattling;
    const int mode=CurrentTurretNumber;
    if(multiple&&(mode<0||mode>=type->TurretCount||mode>=18)){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
    auto& turret=multiple?type->ChargerTurrets[mode]:type->TurretVoxel;
    auto& barrel=multiple?type->ChargerBarrels[mode]:type->BarrelVoxel;
    unsigned turret_frame=body_frame;
    if(!turret_frame&&ready(type->TurretVoxel))turret_frame=unsigned(TurretAnimFrame)%unsigned(type->TurretVoxel.HVA->FrameCount);
    if(key.Is_Valid_Key()){
        auto value=(unsigned(key.Value)&~0x1Fu)|unsigned(SecondaryFacing.Current().GetValue<5>());
        if(!type->TurretOffset)value&=~0x3E0u;
        key.Value=std::bit_cast<int>(value|((turret_frame&0xFFu)<<16));
        if(multiple)key.Value=std::bit_cast<int>(unsigned(key.Value)|(unsigned(mode)<<24));
    }
    drawing_translate_axis(body,0,matrix_store_float(type->TurretOffset*0.16572815467691362));
    const double secondary=(int(SecondaryFacing.Current().GetValue<5>())-8)*-0.1963495408493621;
    const double primary=(int(PrimaryFacing.Current().GetValue<5>())-8)*-0.1963495408493621;
    auto turret_matrix=body;turret_matrix.RotateZ(matrix_store_float(secondary-primary));
    auto barrel_matrix=turret_matrix;
    const Vector3D<float> pivot{turret_matrix.row[0][3],turret_matrix.row[1][3],turret_matrix.row[2][3]};
    drawing_translate(barrel_matrix,{-pivot.X,-pivot.Y,-pivot.Z});
    const auto direction=((unsigned(BarrelFacing.Current().Raw)>>10)+1u)/2u&31u;
    barrel_matrix.RotateY(-matrix_store_float((8-int(direction))*0.19634954084936207));
    drawing_translate(barrel_matrix,pivot);
    const unsigned quadrant=SecondaryFacing.Current().GetValue<2>();
    if(quadrant==0||quadrant==3)draw(barrel,barrel_matrix,0,type->VoxelTurretBarrelCache);
    draw(turret,turret_matrix,multiple?body_frame:turret_frame,type->VoxelTurretWeaponCache);
    if(quadrant==1||quadrant==2)draw(barrel,barrel_matrix,0,type->VoxelTurretBarrelCache);
}
