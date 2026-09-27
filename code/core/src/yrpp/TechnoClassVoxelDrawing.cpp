// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 techno.cpp Draw_Voxel / Techno_Draw_Voxel_Shadow;
// YR 0x00706640 / 0x00706BD0. Hosts own the derived raster/cache allocation.
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AirstrikeClass.h"
#include "techno_drawing.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"

void TechnoClass::DrawVoxel(const VoxelStruct& voxel,DWORD frame,int key,
        const IndexClass<VoxelIndexKey,VoxelCacheStruct*>&,const RectangleStruct& clip,const Point2D& point,
        const Matrix3D& matrix,int intensity,DWORD tint,DWORD excluded) {
    auto* d=game::techno_drawing();if(!d||!game::techno_drawing_complete(d->status))return;
    unsigned flags=0x2000;
    if(!excluded){
        switch(VisualCharacter(false,nullptr)){
        case VisualType::Hidden:return;
        case VisualType::Indistinct:flags|=2;break;
        case VisualType::Darken:case VisualType::Shadowy:flags|=4;break;
        case VisualType::Ripple:flags|=CloakProgress.Value?0xC:0xA;break;
        default:break;
        }
    }
    const auto* unit=WhatAmI()==AbstractType::Unit?static_cast<const UnitClass*>(this):nullptr;
    const auto* building=WhatAmI()==AbstractType::Building?static_cast<const BuildingClass*>(this):nullptr;
    const bool composite=unit&&unit->TerrainPalette;
    if(!composite&&(IsBeingWarpedOut()||IsWarpingIn()))flags|=building&&building->Type->DoubleThick?6:4;
    if(!composite&&Owner&&Owner->IsControlledByCurrentPlayer()&&IsDisguised())flags=GetDisguiseFlags(flags);
    flags=(flags|0x800)&~excluded;
    intensity=GetFlashingIntensity(intensity);
    if(IsIronCurtained()||(building&&building->Airstrike&&building->Airstrike->Target==building))intensity=GetEffectTintIntensity(intensity);
    if(GetTechnoType()->DisableVoxelCache)key=-1;
    if(!voxel.VXL||!voxel.HVA)return;
    if(composite&&key!=-1)flags|=0x20;
    game::TechnoVoxelRequest r;r.object=this;r.resource=&voxel;r.frame=int(frame);r.cache_key=key;
    r.matrix=matrix;r.local_matrix=d->local_voxel_matrix;r.position=point;r.clip=clip;r.flags=flags;r.intensity=intensity;r.tint=tint;
    r.depth=GetZAdjustment();r.use_buffer=building&&building->Type->UseBuffer;
    auto kind=composite?game::TechnoPalette::eight_bit:game::TechnoPalette::house;
    auto* cell=game::techno_drawing_cell(*d,*this);
    if(building&&building->Type->TerrainPalette)kind=game::TechnoPalette::cell;
    if(!game::techno_drawing_palette(*d,*this,kind,cell,Owner,r.palette))return;
    game::record_techno_drawing(*d,d->voxel?d->voxel(d->context,r):game::DrawingStatus::unavailable);
}
void TechnoClass::DrawVoxelShadow(VoxelStruct* voxel,int layer,VoxelIndexKey key,
        IndexClass<ShadowVoxelIndexKey,VoxelCacheStruct*>*,RectangleStruct* clip,Point2D* point,
        Matrix3D* matrix,bool,Surface*,Point2D) {
    auto* d=game::techno_drawing();if(!d||!game::techno_drawing_complete(d->status))return;
    if(!voxel||!voxel->VXL||!voxel->HVA||!clip||!point||!matrix){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
    if(CloakState!=::CloakState::Uncloaked||GetTechnoType()->NoShadow||voxel->VXL->Initialized)return;
    game::TechnoVoxelRequest r;r.object=this;r.resource=voxel;r.matrix=*matrix;r.local_matrix=d->local_voxel_matrix;r.position=*point;r.clip=*clip;
    r.shadow=true;r.shadow_layer=layer;r.cache_key=GetTechnoType()->DisableShadowCache?-1:key.Value;
    r.flags=0x2001;r.intensity=1000;r.depth=GetZAdjustment();
    r.half_shadow=WhatAmI()==AbstractType::Aircraft||GetTechnoType()->ConsideredAircraft;
    if(!game::techno_drawing_palette(*d,*this,game::TechnoPalette::house,game::techno_drawing_cell(*d,*this),Owner,r.palette))return;
    game::record_techno_drawing(*d,d->voxel?d->voxel(d->context,r):game::DrawingStatus::unavailable);
}
