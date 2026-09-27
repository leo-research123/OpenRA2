// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit type Read_INI, calibrated to YR 0x747620.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitTypeClass.h"
#include "yrpp/FileSystem.h"
#include "type_resources.hpp"
#include "building_voxel.hpp"
#include <algorithm>
#include <cstdio>

bool UnitTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if(!ini || !TechnoTypeClass::LoadFromINI(ini))return false;
#define B(field) field=ini->ReadBool(ID,#field,field)
        B(CrateGoodie);B(DeployToFire);B(IsSimpleDeployer);B(Harvester);B(Weeder);
        if(SpeedType==::SpeedType(-1))SpeedType=Crusher ? ::SpeedType::Track : ::SpeedType::Wheel;
        SpeedType=::SpeedType(ini->ReadSpeedType(ID,"SpeedType",int(SpeedType)));
        B(IsTilter);B(CarriesCrate);HasTurret=!Voxel;B(TooBigToFitUnderBridge);
        auto smoke=HalfDamageSmokeLocation;ini->ReadPoint3D(HalfDamageSmokeLocation,ID,"HalfDamageSmokeLocation",smoke);
        Storage=ini->ReadInteger(ID,"Storage",Harvester||Weeder?10:15);
        auto& art=game::type_art_ini();UseTurretShadow=art.ReadBool(ImageFile,"UseTurretShadow",UseTurretShadow);
        WalkFrames=char(art.ReadInteger(ImageFile,"WalkFrames",WalkFrames));
        FiringFrames=char(art.ReadInteger(ImageFile,"FiringFrames",FiringFrames));
        B(Passive);MovementRestrictedTo=LandType(ini->ReadLandType(ID,"MovementRestrictedTo",int(MovementRestrictedTo)));
        B(CanBeach);B(SmallVisceroid);B(LargeVisceroid);B(NonVehicle);
#undef B
        if(FiringFrames>0)StandingFrames=1;
#define I(field) field=art.ReadInteger(ImageFile,#field,field)
        I(StandingFrames);I(DeathFrames);I(DeathFrameRate);DeathFrameRate=std::max(DeathFrameRate,1);
        if(!FiringFrames&&!Voxel)Facings=1;
        I(Facings);
        if(StartWalkFrame==-1)StartWalkFrame=0;
        if(StartStandFrame==-1)StartStandFrame=StandingFrames?Facings*WalkFrames:StartWalkFrame;
        if(StartFiringFrame==-1)StartFiringFrame=FiringFrames?Facings*(StandingFrames+WalkFrames):StartStandFrame;
        if(StartDeathFrame==-1){StartDeathFrame=DeathFrames?Facings*(FiringFrames+WalkFrames+1):-1;MaxDeathCounter=StartDeathFrame+DeathFrames;}
        I(StartStandFrame);I(StartWalkFrame);I(StartFiringFrame);I(StartDeathFrame);I(MaxDeathCounter);
        I(FiringSyncFrame0);I(FiringSyncFrame1);
#undef I
        BurstDelay0=ini->ReadInteger(ID,"BurstDelay0",BurstDelay0);BurstDelay1=ini->ReadInteger(ID,"BurstDelay1",BurstDelay1);
        BurstDelay2=ini->ReadInteger(ID,"BurstDelay2",BurstDelay2);BurstDelay3=ini->ReadInteger(ID,"BurstDelay3",BurstDelay3);
        ini->ReadString(ID,"AltImage",AltImageFile,AltImageFile,sizeof(AltImageFile));
        char name[64];std::snprintf(name,sizeof(name),"%s.SHP",AltImageFile);
        AltImage=static_cast<SHPStruct*>(FileSystem::LoadFile(name,false));
        // YR 0x747BD2..0x747EAC reads these legacy mappings only for FV.
        // A missing weapon index is -1; do not reproduce its array underflow.
        if(!_strcmpi(ID,"FV")){
            constexpr const char* modes[]={"Normal","Repair","MachineGun","Flak","Pistol","Sniper","Shock","Explode","BrainBlast","RadCannon","Chrono","TerroristExplode","Cow","Initiate","Virus","YuriPrime","Guardian"};
            for(int i=0;i<17;++i){
                char key[64];std::snprintf(key,sizeof(key),"%sTurretIndex",modes[i]);
                const int turret=ini->ReadInteger(ID,key,i<4?i:0);
                std::snprintf(key,sizeof(key),"%sTurretWeapon",modes[i]);
                const int weapon=ini->ReadInteger(ID,key,-1);
                if(weapon>=0&&weapon<MaxWeapons)TurretWeapon[weapon]=turret;
                else if(weapon!=-1)return false;
            }
        }
        return game::load_object_voxels(*this);
    } catch(...) {return false;}
}
