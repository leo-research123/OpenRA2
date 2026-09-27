// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 bullettype.cpp::Read_INI; YR 0x0046BEE0 fields and resource order.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "RulesClassReaders.hpp"
#include "type_resources.hpp"
#include "building_voxel.hpp"

bool BulletTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if(!ObjectTypeClass::LoadFromINI(ini))return false;
#define I(n) n=ini->ReadInteger(ID,#n,n)
#define B(n) n=ini->ReadBool(ID,#n,n)
        I(Arm);I(ROT);I(CourseLockDuration);Elasticity=ini->ReadDouble(ID,"Elasticity",Elasticity);
        I(Acceleration);Color=ini->ReadColorString(ID,"Color",Color);
        B(Arcing);B(Floater);B(SubjectToCliffs);B(SubjectToElevation);B(SubjectToWalls);B(VeryHigh);B(Shadow);
        B(Dropping);B(Level);B(Inviso);B(Proximity);B(Ranged);B(Inaccurate);B(FlakScatter);B(AA);B(AG);
        B(Degenerates);B(Bouncy);B(Airburst);I(Cluster);B(Scalable);
        auto& art=game::type_art_ini();
        // YR reads Image again with an empty fallback, even after the base reader.
        if(ini->ReadString(ID,"Image","",ImageFile,sizeof(ImageFile))>0) {
            read_rule_type(art,ImageFile,"Trailer",AbstractType::AnimType,Trailer);
            SpawnDelay=art.ReadInteger(ImageFile,"SpawnDelay",SpawnDelay);
            NoRotate=!art.ReadBool(ImageFile,"Rotates",!NoRotate);
            Flat=art.ReadBool(ImageFile,"Flat",Flat);
        }
        read_rule_type(*ini,ID,"AirburstWeapon",AbstractType::WeaponType,AirburstWeapon);
        read_rule_type(*ini,ID,"ShrapnelWeapon",AbstractType::WeaponType,ShrapnelWeapon);
        I(ShrapnelCount);I(DetonationAltitude);B(Vertical);B(FirersPalette);
        AnimLow=static_cast<byte>(art.ReadInteger(ImageFile,"AnimLow",AnimLow));
        AnimHigh=static_cast<byte>(art.ReadInteger(ImageFile,"AnimHigh",AnimHigh));
        AnimRate=static_cast<byte>(art.ReadInteger(ImageFile,"AnimRate",AnimRate));
        AnimPalette=art.ReadBool(ImageFile,"AnimPalette",AnimPalette);
        if(!Inviso && !LoadTypeImage())return false;
        if(Voxel) {
#if defined(RA2_YRPP_GAME)
            LoadVoxel(); // existing ObjectType dependency, still an original entry
#else
            if(!game::load_object_voxels(*this))return false;
#endif
        }
#undef B
#undef I
        return true;
    } catch(...) {game::type_resource_result(game::TypeResourceStatus::failure);return false;}
}
