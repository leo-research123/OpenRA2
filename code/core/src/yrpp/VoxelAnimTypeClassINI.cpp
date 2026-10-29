// VoxelAnimTypeClass INI reader, YR 0x0074B050.
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/Memory.h"
#include "RulesClassReaders.hpp"
#include "x87_integer.hpp"
#include <cstdio>

namespace {
// Degrees to radians as the original stores the factor (0x0074B10F).
constexpr double DegreesToRadians = 0.01745277777777778;
}

bool VoxelAnimTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini || !ObjectTypeClass::LoadFromINI(ini)) return false;
    // A pair borrowed by an earlier read belongs to its source type.
    const bool borrowed = SourceShared;
    Normalized = ini->ReadBool(ID, "Normalized", Normalized);
    Translucent = ini->ReadBool(ID, "Translucent", Translucent);
    IsTiberium = ini->ReadBool(ID, "IsTiberium", IsTiberium);
    IsMeteor = ini->ReadBool(ID, "IsMeteor", IsMeteor);
    Elasticity = ini->ReadDouble(ID, "Elasticity", Elasticity);
    // An absent key reads as -1 degree, which is nonzero and is stored.
    const double minimum = ini->ReadDouble(ID, "MinAngularVelocity", -1.0);
    if (minimum != 0.0) MinAngularVelocity = game::x87_truncated_product(minimum, DegreesToRadians);
    const double maximum = ini->ReadDouble(ID, "MaxAngularVelocity", -1.0);
    if (maximum != 0.0) MaxAngularVelocity = game::x87_truncated_product(maximum, DegreesToRadians);
    Duration = ini->ReadInteger(ID, "Duration", Duration);
    MinZVel = ini->ReadDouble(ID, "MinZVel", MinZVel);
    MaxZVel = ini->ReadDouble(ID, "MaxZVel", MaxZVel);
    MaxXYVel = ini->ReadDouble(ID, "MaxXYVel", MaxXYVel);
    read_rule_type(*ini, ID, "Spawns", AbstractType::VoxelAnimType, Spawns);
    SpawnCount = ini->ReadInteger(ID, "SpawnCount", SpawnCount);
    const bool shareBody = ini->ReadBool(ID, "ShareBodyData", false);
    const bool shareTurret = ini->ReadBool(ID, "ShareTurretData", false);
    const bool shareBarrel = ini->ReadBool(ID, "ShareBarrelData", false);
    VoxelIndex = ini->ReadInteger(ID, "VoxelIndex", VoxelIndex);
    read_rule_sound(*ini, ID, "StartSound", StartSound);
    read_rule_sound(*ini, ID, "StopSound", StopSound);
    read_rule_type(*ini, ID, "BounceAnim", AbstractType::AnimType, BounceAnim);
    read_rule_type(*ini, ID, "ExpireAnim", AbstractType::AnimType, ExpireAnim);
    read_rule_type(*ini, ID, "TrailerAnim", AbstractType::AnimType, TrailerAnim);
    Damage = ini->ReadInteger(ID, "Damage", Damage);
    DamageRadius = ini->ReadInteger(ID, "DamageRadius", DamageRadius);
    read_rule_type(*ini, ID, "Warhead", AbstractType::WarheadType, Warhead);
    read_rule_type(*ini, ID, "AttachedSystem", AbstractType::ParticleSystemType, AttachedSystem);
    SourceShared = shareBody || shareBarrel || shareTurret;
    if (SourceShared) {
        // The shared pair is borrowed from the source type; body wins over
        // turret, and turret over barrel. A missing source clears the pair.
        auto* source = ini->GetTechnoType(ID, "ShareSource");
        if (!source)
            MainVoxel = VoxelStruct{};
        else if (shareBody)
            MainVoxel = source->MainVoxel;
        else if (shareTurret)
            MainVoxel = source->TurretVoxel;
        else
            MainVoxel = source->BarrelVoxel;
        return true;
    }
    // Only the VXL is replaced; the HVA member is left as it was. The original
    // also deletes a VXL borrowed by an earlier read, which frees the source's
    // object; that pair is dropped instead.
    char path[512];
    std::snprintf(path, sizeof(path), "%s.VXL", ImageFile);
    CCFileClass file(path);
    if (borrowed)
        MainVoxel = VoxelStruct{};
    else
        GameDelete(MainVoxel.VXL);
    MainVoxel.VXL = GameCreate<VoxLib>(&file, false);
    return true;
}
