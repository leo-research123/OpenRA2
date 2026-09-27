// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9, weapon.cpp::Read_INI.
// Modified for YR 0x00772080: all YR keys and lookup order; no OpenTS Burst clamp.
// EA Section 7 terms and warranty disclaimers: third_party/opents/LICENSE.md.
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "RulesClassReaders.hpp"
#include "type_resources.hpp"

bool WeaponTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if (!ini || !ini->GetSection(ID)) return false;
        // The original does not call AbstractTypeClass::LoadFromINI here.
        AmbientDamage = ini->ReadInteger(ID,"AmbientDamage",AmbientDamage);
#define B(field) field = ini->ReadBool(ID,#field,field)
        B(IsSonic); B(Spawner); B(LimboLaunch); B(DecloakToFire);
        B(CellRangefinding); B(FireOnce); B(NeverUse); B(RevealOnFire);
        B(TerrainFire); B(SabotageCursor); B(MigAttackCursor); B(DisguiseFireOnly);
        B(InfiniteMindControl); B(FireWhileMoving); B(DrainWeapon); B(FireInTransport);
        DisguiseFakeBlinkTime = ini->ReadInteger(ID,"DisguiseFakeBlinkTime",DisguiseFakeBlinkTime);
        B(Suicide); B(Supress);
        Burst = ini->ReadInteger(ID,"Burst",Burst);
        Damage = ini->ReadInteger(ID,"Damage",Damage);
        Speed = read_rule_percentage(*ini,ID,"Speed",Speed);
        ROF = ini->ReadInteger(ID,"ROF",ROF);
        Range = read_rule_distance(*ini,ID,"Range",Range);
        MinimumRange = read_rule_distance(*ini,ID,"MinimumRange",MinimumRange);
        if(!game::type_resources().audio_unavailable){
            read_rule_sound_list(*ini,ID,"Report",Report);
            read_rule_sound_list(*ini,ID,"DownReport",DownReport);
        }
        read_rule_type_list(*ini,ID,"Anim",AbstractType::AnimType,Anim);
        read_rule_type(*ini,ID,"AssaultAnim",AbstractType::AnimType,AssaultAnim);
        read_rule_type(*ini,ID,"OccupantAnim",AbstractType::AnimType,OccupantAnim);
        read_rule_type(*ini,ID,"OpenToppedAnim",AbstractType::AnimType,OpenToppedAnim);
        B(Camera); B(IsLaser); B(DiskLaser); B(IsLine); B(IsHouseColor);
        B(Charges); B(TurboBoost); B(UseFireParticles); B(UseSparkParticles);
        B(OmniFire); B(DistributedWeaponFire); B(IsRailgun); B(Lobber);
        LaserInnerColor = ini->ReadColor(ID,"LaserInnerColor",LaserInnerColor);
        LaserOuterColor = ini->ReadColor(ID,"LaserOuterColor",LaserOuterColor);
        LaserOuterSpread = ini->ReadColor(ID,"LaserOuterSpread",LaserOuterSpread);
        // Preserve the low byte, including the original signed-char fallback.
        LaserDuration = static_cast<char>(ini->ReadInteger(ID,"LaserDuration",static_cast<signed char>(LaserDuration)));
        B(IsBigLaser); B(Bright); B(IonSensitive); B(AreaFire); B(IsElectricBolt);
        B(DrawBoltAsLaser); B(IsAlternateColor); B(IsRadBeam); B(IsRadEruption);
        RadLevel = ini->ReadInteger(ID,"RadLevel",RadLevel);
        B(IsMagBeam);
#undef B
        // Original From_Name returns an allocating registry index, not a pointer;
        // it uses a 19-character name and preserves the field for index -1.
        char particle[20]{};
        ini->ReadString(ID,"AttachedParticleSystem","",particle,sizeof(particle));
        const auto& runtime = game::rules_runtime();
        int index = -1;
        if (!runtime.find_index || !runtime.find_index(runtime.context,
                AbstractType::ParticleSystemType,particle,index)) return false;
        if (index != -1) {
            AbstractTypeClass* type = nullptr;
            if (!game::rules_type_at(AbstractType::ParticleSystemType,index,type)) return false;
            AttachedParticleSystem = static_cast<ParticleSystemTypeClass*>(type);
        }
        read_rule_type(*ini,ID,"Warhead",AbstractType::WarheadType,Warhead);
        read_rule_type(*ini,ID,"Projectile",AbstractType::BulletType,Projectile);
        return true;
    } catch (...) {
        // Native dependency/allocation failures cannot escape the class boundary.
        // Like the original sequential reader, previous assignments remain made.
        return false;
    }
}
