// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 airctype.cpp Read_INI; YR 0x41CC20.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "type_resources.hpp"
#include "building_voxel.hpp"
#include "RulesClassReaders.hpp"
bool AircraftTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if(!ini||!TechnoTypeClass::LoadFromINI(ini))return false;
#define B(field) field=ini->ReadBool(ID,#field,field)
        B(Landable);B(AirportBound);B(Fighter);B(Carryall);B(FlyBy);B(FlyBack);
#undef B
        auto& art=game::type_art_ini();
        Rotors=art.ReadBool(ImageFile,"Rotors",Rotors);
        CustomRotor=art.ReadBool(ImageFile,"CustomRotor",CustomRotor);
        read_rule_type(art,ImageFile,"Trailer",AbstractType::AnimType,Trailer);
        SpawnDelay=art.ReadInteger(ImageFile,"SpawnDelay",SpawnDelay);
        return game::load_object_voxels(*this);
    }catch(...){return false;}
}
