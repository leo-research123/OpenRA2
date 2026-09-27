/*
 * Adapts fields/section-reader structure from EA REDALERT/RULES.CPP, fixed
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc. GPL-3.0-or-later with terms in third_party/ea/LICENSE.TXT.
 * YR's field types, query order, conversions and type references are
 * calibrated against the fixed EXE entry 66BBB0.
 */
#include "yrpp/RulesClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "RulesClassReaders.hpp"

bool RulesClass::Read_CombatDamage(CCINIClass* ini) {
    const char* section = "CombatDamage";
    if (!ini || !ini->GetSection(section)) return false;
    AmmoCrateDamage = ini->ReadInteger(section, "AmmoCrateDamage", AmmoCrateDamage);
    IonCannonDamage = ini->ReadInteger(section, "IonCannonDamage", IonCannonDamage);
    RailgunDamageRadius = ini->ReadInteger(section, "RailgunDamageRadius", RailgunDamageRadius);
    TiberiumExplosionDamage = ini->ReadInteger(section, "TiberiumExplosionDamage", TiberiumExplosionDamage);
    TiberiumStrength = ini->ReadInteger(section, "TiberiumStrength", TiberiumStrength);
    read_rule_type_list(*ini, section, "Scorches", AbstractType::SmudgeType, Scorches);
    read_rule_type_list(*ini, section, "Scorches1", AbstractType::SmudgeType, Scorches1);
    read_rule_type_list(*ini, section, "Scorches2", AbstractType::SmudgeType, Scorches2);
    read_rule_type_list(*ini, section, "Scorches3", AbstractType::SmudgeType, Scorches3);
    read_rule_type_list(*ini, section, "Scorches4", AbstractType::SmudgeType, Scorches4);
    read_rule_type_list(*ini, section, "SplashList", AbstractType::AnimType, SplashList);
    read_rule_type(*ini, section, "FlameDamage", AbstractType::WarheadType, FlameDamage);
    read_rule_type(*ini, section, "FlameDamage2", AbstractType::WarheadType, FlameDamage2);
    read_rule_type(*ini, section, "C4Warhead", AbstractType::WarheadType, C4Warhead);
    read_rule_type(*ini, section, "CrushWarhead", AbstractType::WarheadType, CrushWarhead);
    read_rule_type(*ini, section, "V3Warhead", AbstractType::WarheadType, V3Warhead);
    read_rule_type(*ini, section, "DMislWarhead", AbstractType::WarheadType, DMislWarhead);
    auto* v3_elite = V3Warhead; // Original fallback is the regular warhead.
    read_rule_type(*ini, section, "V3EliteWarhead", AbstractType::WarheadType, v3_elite);
    V3EliteWarhead = v3_elite;
    auto* dmisl_elite = DMislWarhead;
    read_rule_type(*ini, section, "DMislEliteWarhead", AbstractType::WarheadType, dmisl_elite);
    DMislEliteWarhead = dmisl_elite;
    read_rule_type(*ini, section, "CMislWarhead", AbstractType::WarheadType, CMislWarhead);
    auto* cmisl_elite = CMislWarhead;
    read_rule_type(*ini, section, "CMislEliteWarhead", AbstractType::WarheadType, cmisl_elite);
    CMislEliteWarhead = cmisl_elite;
    read_rule_type(*ini, section, "IvanWarhead", AbstractType::WarheadType, IvanWarhead);
    CanDetonateTimeBomb = ini->ReadBool(section, "CanDetonateTimeBomb", CanDetonateTimeBomb);
    CanDetonateDeathBomb = ini->ReadBool(section, "CanDetonateDeathBomb", CanDetonateDeathBomb);
    read_rule_type(*ini, section, "DeathWeapon", AbstractType::WeaponType, DeathWeapon);
    IvanDamage = ini->ReadInteger(section, "IvanDamage", IvanDamage);
    IvanTimedDelay = ini->ReadInteger(section, "IvanTimedDelay", IvanTimedDelay);
    read_rule_shape("BOMBCURS.SHP", BOMBCURS_SHP);
    read_rule_shape("CHRONOSK.SHP", CHRONOSK_SHP);
    IvanIconFlickerRate = ini->ReadInteger(section, "IvanIconFlickerRate", IvanIconFlickerRate);
    IronCurtainDuration = ini->ReadInteger(section, "IronCurtainDuration", IronCurtainDuration);
    PsychicRevealRadius = ini->ReadInteger(section, "PsychicRevealRadius", PsychicRevealRadius);
    OccupyDamageMultiplier = static_cast<float>(ini->ReadDouble(section, "OccupyDamageMultiplier", OccupyDamageMultiplier));
    OccupyROFMultiplier = static_cast<float>(ini->ReadDouble(section, "OccupyROFMultiplier", OccupyROFMultiplier));
    OccupyWeaponRange = ini->ReadInteger(section, "OccupyWeaponRange", OccupyWeaponRange);
    BunkerDamageMultiplier = static_cast<float>(ini->ReadDouble(section, "BunkerDamageMultiplier", BunkerDamageMultiplier));
    BunkerROFMultiplier = static_cast<float>(ini->ReadDouble(section, "BunkerROFMultiplier", BunkerROFMultiplier));
    BunkerWeaponRangeBonus = ini->ReadInteger(section, "BunkerWeaponRangeBonus", BunkerWeaponRangeBonus);
    OpenToppedDamageMultiplier = static_cast<float>(ini->ReadDouble(section, "OpenToppedDamageMultiplier", OpenToppedDamageMultiplier));
    OpenToppedRangeBonus = ini->ReadInteger(section, "OpenToppedRangeBonus", OpenToppedRangeBonus);
    OpenToppedWarpDistance = ini->ReadInteger(section, "OpenToppedWarpDistance", OpenToppedWarpDistance);
    read_rule_integer_list(*ini, section, "OverloadCount", OverloadCount);
    read_rule_integer_list(*ini, section, "OverloadDamage", OverloadDamage);
    read_rule_integer_list(*ini, section, "OverloadFrames", OverloadFrames);
    MindControlAttackLineFrames = ini->ReadInteger(section, "MindControlAttackLineFrames", MindControlAttackLineFrames);
    read_rule_type(*ini, section, "DrainAnimationType", AbstractType::AnimType, DrainAnimationType);
    DrainMoneyFrameDelay = ini->ReadInteger(section, "DrainMoneyFrameDelay", DrainMoneyFrameDelay);
    DrainMoneyAmount = ini->ReadInteger(section, "DrainMoneyAmount", DrainMoneyAmount);
    FallingDamageMultiplier = static_cast<float>(ini->ReadDouble(section, "FallingDamageMultiplier", FallingDamageMultiplier));
    CurrentStrengthDamage = ini->ReadBool(section, "CurrentStrengthDamage", CurrentStrengthDamage);
    read_rule_type(*ini, section, "ControlledAnimationType", AbstractType::AnimType, ControlledAnimationType);
    read_rule_type(*ini, section, "PermaControlledAnimationType", AbstractType::AnimType, PermaControlledAnimationType);
    read_rule_type(*ini, section, "IonCannonWarhead", AbstractType::WarheadType, IonCannonWarhead);
    read_rule_type(*ini, section, "DefaultLargeGreySmokeSystem", AbstractType::ParticleSystemType, DefaultLargeGreySmokeSystem);
    read_rule_type(*ini, section, "DefaultSmallGreySmokeSystem", AbstractType::ParticleSystemType, DefaultSmallGreySmokeSystem);
    read_rule_type(*ini, section, "DefaultSparkSystem", AbstractType::ParticleSystemType, DefaultSparkSystem);
    read_rule_type(*ini, section, "DefaultLargeRedSmokeSystem", AbstractType::ParticleSystemType, DefaultLargeRedSmokeSystem);
    read_rule_type(*ini, section, "DefaultSmallRedSmokeSystem", AbstractType::ParticleSystemType, DefaultSmallRedSmokeSystem);
    read_rule_type(*ini, section, "DefaultDebrisSmokeSystem", AbstractType::ParticleSystemType, DefaultDebrisSmokeSystem);
    read_rule_type(*ini, section, "DefaultFireStreamSystem", AbstractType::ParticleSystemType, DefaultFireStreamSystem);
    read_rule_type(*ini, section, "DefaultTestParticleSystem", AbstractType::ParticleSystemType, DefaultTestParticleSystem);
    read_rule_type(*ini, section, "DefaultRepairParticleSystem", AbstractType::ParticleSystemType, DefaultRepairParticleSystem);
    BerzerkAllowed = ini->ReadBool(section, "BerzerkAllowed", BerzerkAllowed);
    TurboBoost = ini->ReadDouble(section, "TurboBoost", TurboBoost);
    AtomDamage = ini->ReadInteger(section, "AtomDamage", AtomDamage);
    BallisticScatter = read_rule_distance(*ini, section, "BallisticScatter", BallisticScatter);
    BridgeStrength = ini->ReadInteger(section, "BridgeStrength", BridgeStrength);
    C4Delay = ini->ReadDouble(section, "C4Delay", C4Delay);
    Crush = read_rule_distance(*ini, section, "Crush", Crush);
    ExpSpread = ini->ReadDouble(section, "ExpSpread", ExpSpread);
    FireSupress = read_rule_distance(*ini, section, "FireSupress", FireSupress);
    HomingScatter = read_rule_distance(*ini, section, "HomingScatter", HomingScatter);
    MaxDamage = ini->ReadInteger(section, "MaxDamage", MaxDamage);
    MinDamage = ini->ReadInteger(section, "MinDamage", MinDamage);
    TiberiumExplosive = ini->ReadBool(section, "TiberiumExplosive", TiberiumExplosive);
    PlayerAutoCrush = ini->ReadBool(section, "PlayerAutoCrush", PlayerAutoCrush);
    PlayerReturnFire = ini->ReadBool(section, "PlayerReturnFire", PlayerReturnFire);
    PlayerScatter = ini->ReadBool(section, "PlayerScatter", PlayerScatter);
    TreeTargeting = ini->ReadBool(section, "TreeTargeting", TreeTargeting);
    Incoming = read_rule_percentage(*ini, section, "Incoming", Incoming);
    CollapseChance = ini->ReadInteger(section, "CollapseChance", CollapseChance);
    return true;
}
