// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 warhead.cpp::Read_INI, adapted to all YR 0x0075D3A0 fields.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "RulesClassReaders.hpp"
#include <cstdlib>
#include <cstring>

namespace {
bool read_guid(const char* text,GUID& result) {
    // Canonical rule-file CLSID form. Invalid input retains the previous GUID,
    // like the target CLSIDFromString failure branch (0x00527920).
    if (std::strlen(text)!=38 || text[0]!='{' || text[37]!='}') return false;
    GUID value{};
    auto hex=[&](unsigned start,unsigned count,std::uint32_t& output) {
        output=0;
        for(unsigned i=0;i<count;++i) {
            const unsigned char c=text[start+i];unsigned n;
            if(c>='0'&&c<='9')n=c-'0';else if(c>='a'&&c<='f')n=c-'a'+10;
            else if(c>='A'&&c<='F')n=c-'A'+10;else return false;
            output=(output<<4)|n;
        }
        return true;
    };
    if(text[9]!='-'||text[14]!='-'||text[19]!='-'||text[24]!='-')return false;
    std::uint32_t part;
    if(!hex(1,8,part))return false;value.Data1=part;
    if(!hex(10,4,part))return false;value.Data2=static_cast<WORD>(part);
    if(!hex(15,4,part))return false;value.Data3=static_cast<WORD>(part);
    for(unsigned i=0;i<8;++i) {
        if(!hex(i<2?20+i*2:25+(i-2)*2,2,part))return false;
        value.Data4[i]=static_cast<BYTE>(part);
    }
    result=value;return true;
}
}
bool WarheadTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if(!ini || !ini->GetSection(ID))return false;
#define F(field) field=static_cast<float>(ini->ReadDouble(ID,#field,field))
#define B(field) field=ini->ReadBool(ID,#field,field)
#define I(field) field=ini->ReadInteger(ID,#field,field)
        F(CellSpread);F(CellInset);F(PercentAtMax);B(CausesDelayKill);I(DelayKillFrames);
        F(DelayKillAtMax);F(CombatLightSize);
        read_rule_type(*ini,ID,"Particle",AbstractType::ParticleSystemType,Particle);
        B(Conventional);B(Wall);B(WallAbsoluteDestroyer);B(PenetratesBunker);B(Wood);B(Tiberium);
        B(Sparky);B(Sonic);B(Rocker);B(DirectRocker);B(Fire);B(Bright);
        B(CLDisableRed);B(CLDisableGreen);B(CLDisableBlue);
        read_rule_type_list(*ini,ID,"AnimList",AbstractType::AnimType,AnimList);
        InfDeath=static_cast<::InfDeath>(ini->ReadInteger(ID,"InfDeath",static_cast<int>(InfDeath)));
        Deform=ini->ReadDouble(ID,"Deform",Deform);
        DeformTreshold=ini->ReadInteger(ID,"DeformThreshhold",DeformTreshold);
        B(EMEffect);B(MindControl);B(Poison);B(IvanBomb);B(ElectricAssault);B(Parasite);B(Temporal);B(IsLocomotor);
        char guid[128]{};
        if(ini->ReadString(ID,"Locomotor","",guid,sizeof(guid))) read_guid(guid,Locomotor);
        B(Airstrike);B(Psychedelic);B(BombDisarm);I(Paralyzes);B(Culling);B(MakesDisguise);B(NukeMaker);
        ProneDamage=ini->ReadDouble(ID,"ProneDamage",ProneDamage);
        B(Radiation);B(PsychicDamage);B(AffectsAllies);B(Bullets);B(Veinhole);
        I(ShakeXlo);I(ShakeXhi);I(ShakeYlo);I(ShakeYhi);I(MaxDebris);I(MinDebris);
        if(MinDebris<0)MinDebris=0;
        if(MaxDebris<MinDebris)MaxDebris=MinDebris;
        read_rule_type_list(*ini,ID,"DebrisTypes",AbstractType::VoxelAnimType,DebrisTypes);
        read_rule_integer_list(*ini,ID,"DebrisMaximums",DebrisMaximums);
        char text[128]{};
        // A missing key resets all eleven verses to 100%, not their old values.
        if(ini->ReadString(ID,"Verses","100%,100%,100%,100%,100%,100%,100%,100%,100%,100%,100%",text,sizeof(text))) {
            double values[11];char* cursor=text;
            for(auto& value:values) {
                char* token=next_rule_token(cursor);
                if(!token)return false; // target dereferences null on short lists
                value=std::strchr(token,'%')?double(rule_decimal(token))*0.01:std::atof(token);
            }
            for(unsigned i=0;i<11;++i)Verses[i]=values[i];
        }
        unknown_bool_149=Verses[4]==0.0 && Verses[6]==0.0;
#undef I
#undef B
#undef F
        return true;
    } catch(...) {return false;}
}
