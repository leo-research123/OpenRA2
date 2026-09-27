// Type-list enumeration follows pinned EA REDALERT/RULES.CPP::Objects.
// Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later, with the additional
// terms in third_party/ea/LICENSE.TXT. YR uses registry-owned dynamic types,
// 32-byte names and stdcall entry points (672280..672A70).
#include "yrpp/RulesClass.h"
#include "RulesClassReaders.hpp"

namespace {
bool register_types(CCINIClass* ini, const char* section, AbstractType kind) {
    if (!ini) return false;
    const int count = ini->GetKeyCount(section);
    for (int index = 0; index < count; ++index) {
        char name[32];
        if (!ini->ReadString(section, ini->GetKeyName(section, index), "", name, sizeof(name))) continue;
        AbstractTypeClass* result = nullptr;
        if (!game::rules_resolve_type(kind, name, result))
            throw std::runtime_error("RulesClass type registration dependency failed");
        // The original returns whether keys exist, even for blank entries,
        // NONE sentinels and failed allocations. No content section is loaded.
    }
    return count > 0;
}
}

#define RULES_TYPE_LIST(Method, Section, Kind) \
bool YRPP_STDCALL RulesClass::Method(CCINIClass* ini) { \
    return register_types(ini, Section, AbstractType::Kind); \
}
RULES_TYPE_LIST(Read_InfantryTypes, "InfantryTypes", InfantryType)
RULES_TYPE_LIST(Read_Countries, "Countries", HouseType)
RULES_TYPE_LIST(Read_VehicleTypes, "VehicleTypes", UnitType)
RULES_TYPE_LIST(Read_AircraftTypes, "AircraftTypes", AircraftType)
RULES_TYPE_LIST(Read_SuperWeaponTypes, "SuperWeaponTypes", SuperWeaponType)
RULES_TYPE_LIST(Read_BuildingTypes, "BuildingTypes", BuildingType)
RULES_TYPE_LIST(Read_TerrainTypes, "TerrainTypes", TerrainType)
RULES_TYPE_LIST(Read_SmudgeTypes, "SmudgeTypes", SmudgeType)
RULES_TYPE_LIST(Read_OverlayTypes, "OverlayTypes", OverlayType)
RULES_TYPE_LIST(Read_Animations, "Animations", AnimType)
RULES_TYPE_LIST(Read_VoxelAnims, "VoxelAnims", VoxelAnimType)
RULES_TYPE_LIST(Read_Warheads, "Warheads", WarheadType)
RULES_TYPE_LIST(Read_Particles, "Particles", ParticleType)
RULES_TYPE_LIST(Read_ParticleSystems, "ParticleSystems", ParticleSystemType)
#undef RULES_TYPE_LIST
