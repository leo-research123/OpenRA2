// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 foot.h::Draw_Object / foot.cpp::Draw_Voxel;
// YR 0x0041C090 / 0x004DAF10.
#include "yrpp/FootClass.h"
#include "techno_drawing.hpp"
ZGradient FootClass::GetZGradient() const { return Locomotor?Locomotor->Z_Gradient():ZGradient::Deg90; }
void FootClass::Draw_A_SHP(SHPStruct* image,int frame,Point2D* point,RectangleStruct* clip,
        DWORD rotation,DWORD scale,DWORD z,ZGradient gradient,DWORD write_depth,int intensity,
        DWORD tint,SHPStruct* depth_image,DWORD depth_frame,DWORD depth_x,DWORD depth_y,DWORD excluded) {
    DrawObject(image,frame,point,clip,int(rotation),int(scale),int(z),gradient,int(write_depth),intensity,int(tint),
        depth_image,int(depth_frame),int(depth_x),int(depth_y),int(excluded));
}
void FootClass::Draw_A_VXL(VoxelStruct* voxel,int frame,int key,IndexClass<VoxelIndexKey,VoxelCacheStruct*>* cache,
        RectangleStruct* clip,Point2D* point,Matrix3D* matrix,int intensity,BlitterFlags flags,DWORD tint) {
    auto* d=game::techno_drawing();if(!d)return;
    if(!voxel||!cache||!clip||!point||!matrix){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
    auto position=*point;
    if(Locomotor){const auto delta=Locomotor->Draw_Point();position.X+=delta.X;position.Y+=delta.Y;}
    TechnoClass::DrawVoxel(*voxel,DWORD(frame),key,*cache,*clip,position,*matrix,intensity,tint,DWORD(flags));
}
void FootClass::DrawVoxelShadow(VoxelStruct* voxel,int layer,VoxelIndexKey key,
        IndexClass<ShadowVoxelIndexKey,VoxelCacheStruct*>* cache,RectangleStruct* clip,
        Point2D* point,Matrix3D* matrix,bool force,Surface* surface,Point2D shadowPoint) {
    if(!Locomotor||!Locomotor->Is_To_Have_Shadow())return;
    if(!point){if(auto* d=game::techno_drawing())game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
    auto position=*point;const auto delta=Locomotor->Shadow_Point();position.X+=delta.X;position.Y+=delta.Y;
    TechnoClass::DrawVoxelShadow(voxel,layer,key,cache,clip,&position,matrix,force,surface,shadowPoint);
}
