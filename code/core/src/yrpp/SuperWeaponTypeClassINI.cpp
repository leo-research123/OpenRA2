// Original SuperWeaponTypeClass::LoadFromINI, YR 0x006CEA20.
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "RulesClassReaders.hpp"
#include <cstdio>

bool SuperWeaponTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if (!ini || !AbstractTypeClass::LoadFromINI(ini)) return false;
        read_rule_type(*ini, ID, "WeaponType", AbstractType::WeaponType, WeaponType);
        Action = ::Action(ini->ReadSWAction(ID, "Action", int(Action)));
#define B(f) f = ini->ReadBool(ID, #f, f)
        B(IsPowered);
        B(DisableableFromShell);
        FlashSidebarTabFrames = ini->ReadInteger(ID, "FlashSidebarTabFrames", FlashSidebarTabFrames);
        B(AIDefendAgainst);
        B(PreClick);
        B(PostClick);
        B(ShowTimer);
        read_rule_sound(*ini, ID, "SpecialSound", SpecialSound);
        read_rule_sound(*ini, ID, "StartSound", StartSound);
        Range = float(ini->ReadDouble(ID, "Range", Range));
        LineMultiplier = ini->ReadInteger(ID, "LineMultiplier", LineMultiplier);
        constexpr const char* names[] = {"MultiMissile", "IronCurtain",      "LightningStorm", "ChronoSphere",
                                         "ChronoWarp",   "ParaDrop",         "AmerParaDrop",   "PsychicDominator",
                                         "SpyPlane",     "GeneticConverter", "ForceShield",    "PsychicReveal"};
        char text[40];
        ini->ReadString(ID, "Type", "", text, sizeof(text));
        for (int i = 0; i < 12; ++i)
            if (!_strcmpi(text, names[i])) {
                Type = SuperWeaponType(i);
                break;
            }
        ini->ReadString(ID, "PreDependent", "", text, sizeof(text));
        for (int i = 0; i < 12; ++i)
            if (!_strcmpi(text, names[i])) {
                PreDependent = i;
                break;
            }
        read_rule_type(*ini, ID, "AuxBuilding", AbstractType::BuildingType, AuxBuilding);
        B(UseChargeDrain);
        B(ManualControl);
#undef B
        const double minutes = ini->ReadDouble(ID, "RechargeTime", 0.0);
        if (minutes != 0.0) RechargeTime = rule_scaled_integer(minutes, 900);
        char source[256], filename[512];
        ini->ReadString(ID, "SidebarImage", SidebarImageFile, source, sizeof(source));
        std::strncpy(SidebarImageFile, source, sizeof(SidebarImageFile));
        zero_E4 = 0;
        std::snprintf(filename, sizeof(filename), "%s.SHP", SidebarImageFile);
        SidebarImage = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
        if (!SidebarImage) SidebarImage = static_cast<SHPStruct*>(FileSystem::LoadFile("XXICON.SHP", false));
        return true;
    } catch (...) {
        return false;
    }
}
