// OpenTS 44fac744 techtype.cpp Read_INI movement fields, YR 0x00712170.
// GPL-3.0-or-later; EA Section 7 terms: third_party/opents/LICENSE.md.
// The rules prefix is restored in target order; later rules/ART remain staged.
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "type_resources.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "RulesClassReaders.hpp"
#include <cstdio>
#include "yrpp/HouseTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/ColorScheme.h"
#include <cstddef>
#include <cmath>
#if !defined(RA2_YRPP_GAME)
byte TechnoTypeClass::LoadPalette() noexcept {
    if (!*PaletteFile) return 0;
    Palette = ColorScheme::GeneratePalette(PaletteFile);
    return static_cast<byte>(reinterpret_cast<std::uintptr_t>(Palette));
}
AbilitiesStruct AbilitiesStruct::operator=(const AbilitiesStruct& value) noexcept {
    if (this != &value) std::memcpy(this, &value, sizeof(*this));
    return *this;
}
int TurretControl::SetTravel(int value) noexcept { return Travel = value; }
int TurretControl::GetTravel() const noexcept { return Travel; }
int TurretControl::SetCompressFrames(int value) noexcept { return CompressFrames = value <= 0 ? 1 : value; }
int TurretControl::GetCompressFrames() const noexcept { return CompressFrames; }
int TurretControl::SetRecoverFrames(int value) noexcept { return RecoverFrames = value <= 0 ? 1 : value; }
int TurretControl::GetRecoverFrames() const noexcept { return RecoverFrames; }
int TurretControl::SetHoldFrames(int value) noexcept { return HoldFrames = value <= 0 ? 1 : value; }
int TurretControl::GetHoldFrames() const noexcept { return HoldFrames; }
#endif
bool TechnoTypeClass::LoadFromINI(CCINIClass* ini) {
    try {
        if (!ini) return false;
        ini->Reset();
        if (!ObjectTypeClass::LoadFromINI(ini)) return false;
        // 0x712170..0x7125E8: preserve the target's read order and old-value defaults.
        LandTargeting =
            static_cast<::LandTargetingType>(ini->ReadInteger(ID, "LandTargeting", static_cast<int>(LandTargeting)));
        NavalTargeting =
            static_cast<::NavalTargetingType>(ini->ReadInteger(ID, "NavalTargeting", static_cast<int>(NavalTargeting)));
        SpeedType = static_cast<::SpeedType>(ini->ReadSpeedType(ID, "SpeedType", static_cast<int>(SpeedType)));
        // 0x7121F8 reads ART using ID, independently of the image section.
        CanBeHidden = game::type_art_ini().ReadBool(ID, "CanBeHidden", CanBeHidden);
        TypeImmune = ini->ReadBool(ID, "TypeImmune", TypeImmune);
        WalkRate = ini->ReadInteger(ID, "WalkRate", WalkRate);
        IdleRate = ini->ReadInteger(ID, "IdleRate", IdleRate);
        MoveToShroud = ini->ReadBool(ID, "MoveToShroud", MoveToShroud);
        IsTrain = ini->ReadBool(ID, "IsTrain", IsTrain);
        DoubleOwned = ini->ReadBool(ID, "DoubleOwned", DoubleOwned);
        // Distance conversion changes the floating-point control mode before angles.
        GuardRange = read_rule_distance(*ini, ID, "GuardRange", GuardRange);
        Explodes = ini->ReadBool(ID, "Explodes", Explodes);
        if (!game::type_resources().combat_unavailable)
            read_rule_type(*ini, ID, "DeathWeapon", AbstractType::WeaponType, DeathWeapon);
        DeathWeaponDamageModifier =
            static_cast<float>(ini->ReadDouble(ID, "DeathWeaponDamageModifier", DeathWeaponDamageModifier));
        FlightLevel = ini->ReadInteger(ID, "FlightLevel", FlightLevel);
        IsDropship = ini->ReadBool(ID, "IsDropship", IsDropship);
        // 0x71238B / 0x7123BC: INI degrees become radians; -1 retains the old angle.
        constexpr double radiansPerDegree = 0x1.1df46a2529d39p-6;
        const double pitch = ini->ReadDouble(ID, "PitchAngle", -1.0);
        if (pitch != -1.0) PitchAngle = pitch * radiansPerDegree;
        const double roll = ini->ReadDouble(ID, "RollAngle", -1.0);
        if (roll != -1.0) RollAngle = roll * radiansPerDegree;
        PitchSpeed = ini->ReadDouble(ID, "PitchSpeed", PitchSpeed);
        char locomotor[128]{};
        if (ini->ReadString(ID, "Locomotor", "", locomotor, sizeof(locomotor))) read_rule_guid(locomotor, Locomotor);
        CloakingSpeed = ini->ReadInteger(ID, "CloakingSpeed", CloakingSpeed);
        ThreatAvoidanceCoefficient = ini->ReadDouble(ID, "ThreatAvoidanceCoefficient", ThreatAvoidanceCoefficient);
        SlowdownDistance = ini->ReadInteger(ID, "SlowdownDistance", SlowdownDistance);
        DecelerationFactor = ini->ReadDouble(ID, "DeaccelerationFactor", DecelerationFactor);
        AccelerationFactor = ini->ReadDouble(ID, "AccelerationFactor", AccelerationFactor);
        Weight = ini->ReadDouble(ID, "Weight", Weight);
        PhysicalSize = ini->ReadDouble(ID, "PhysicalSize", PhysicalSize);
        Size = ini->ReadDouble(ID, "Size", Size);
        SizeLimit = ini->ReadDouble(ID, "SizeLimit", SizeLimit);
        HoverAttack = ini->ReadBool(ID, "HoverAttack", HoverAttack);
        VHPScan = ini->ReadVHPScan(ID, "VHPScan", VHPScan);
        MaxDebris = ini->ReadInteger(ID, "MaxDebris", MaxDebris);
        MinDebris = ini->ReadInteger(ID, "MinDebris", MinDebris);
        if (MinDebris < 0) MinDebris = 0;
        if (MaxDebris < MinDebris) MaxDebris = MinDebris;
        read_rule_type_list(*ini, ID, "DebrisTypes", AbstractType::VoxelAnimType, DebrisTypes);
        read_rule_integer_list(*ini, ID, "DebrisMaximums", DebrisMaximums);
        read_rule_type_list(*ini, ID, "DebrisAnims", AbstractType::AnimType, DebrisAnims);
        // OpenTS Primary/Secondary slots, calibrated to YR 0x712170: elite slots
        // are independent (GetWeapon supplies the fallback), not TS's third slot.
        HasTurretTooltips = ini->ReadBool(ID, "HasTurretTooltips", HasTurretTooltips);
        if (!game::type_resources().combat_unavailable) {
            TurretCount = ini->ReadInteger(ID, "TurretCount", TurretCount);
            WeaponCount = ini->ReadInteger(ID, "WeaponCount", WeaponCount);
        }
        IsChargeTurret = ini->ReadBool(ID, "IsChargeTurret", IsChargeTurret);
        if (!game::type_resources().combat_unavailable) {
            ClearAllWeapons = ini->ReadBool(ID, "ClearAllWeapons", ClearAllWeapons);
            if (TurretCount <= 0) {
                if (!ClearAllWeapons) {
                    read_rule_type(*ini, ID, "Primary", AbstractType::WeaponType, Weapon[0].WeaponType);
                    read_rule_type(*ini, ID, "Secondary", AbstractType::WeaponType, Weapon[1].WeaponType);
                    read_rule_type(*ini, ID, "ElitePrimary", AbstractType::WeaponType, EliteWeapon[0].WeaponType);
                    read_rule_type(*ini, ID, "EliteSecondary", AbstractType::WeaponType, EliteWeapon[1].WeaponType);
                }
            } else {
                if (WeaponCount > MaxWeapons) return false;  // never reproduce a fixed-array overrun
                for (int i = 0; i < WeaponCount; ++i) {
                    char key[32];
                    std::snprintf(key, sizeof(key), "Weapon%d", i + 1);
                    read_rule_type(*ini, ID, key, AbstractType::WeaponType, Weapon[i].WeaponType);
                    std::snprintf(key, sizeof(key), "EliteWeapon%d", i + 1);
                    read_rule_type(*ini, ID, key, AbstractType::WeaponType, EliteWeapon[i].WeaponType);
                }
            }
            if (ClearAllWeapons) {
                Weapon[0].WeaponType = Weapon[1].WeaponType = nullptr;
                EliteWeapon[0].WeaponType = EliteWeapon[1].WeaponType = nullptr;
            }
        }
        // 0x712ACC..0x71399A: sound reads interleave with type relationships.
#define SOUND_LIST(field) read_rule_sound_list(*ini, ID, #field, field)
#define SOUND_VALUE(field) read_rule_sound(*ini, ID, #field, field)
        if (!game::type_resources().audio_unavailable) {
            SOUND_LIST(VoiceMove);
            SOUND_LIST(VoiceSelect);
            SOUND_LIST(VoiceSelectEnslaved);
            SOUND_LIST(VoiceSelectDeactivated);
            SOUND_LIST(VoiceAttack);
            SOUND_LIST(VoiceSpecialAttack);
            SOUND_LIST(VoiceDie);
            SOUND_LIST(VoiceFeedback);
            SOUND_VALUE(AuxSound1);
            SOUND_VALUE(AuxSound2);
            SOUND_VALUE(CreateSound);
            SOUND_VALUE(DamageSound);
            SOUND_VALUE(ImpactWaterSound);
            SOUND_VALUE(ImpactLandSound);
            SOUND_VALUE(CrashingSound);
            SOUND_VALUE(SinkingSound);
            SOUND_VALUE(VoiceFalling);
            SOUND_VALUE(VoiceCrashing);
            SOUND_VALUE(VoiceSinking);
            SOUND_VALUE(VoiceEnter);
            SOUND_VALUE(VoiceCapture);
        }
        CloakStop = ini->ReadBool(ID, "CloakStop", CloakStop);
        Storage = ini->ReadInteger(ID, "Storage", Storage);
        BuildLimit = ini->ReadInteger(ID, "BuildLimit", BuildLimit);
        Category = static_cast<::Category>(ini->ReadCategory(ID, "Category", static_cast<int>(Category)));
        read_rule_type_list(*ini, ID, "Dock", AbstractType::BuildingType, Dock);
        read_rule_type(*ini, ID, "DeploysInto", AbstractType::BuildingType, DeploysInto);
        read_rule_type(*ini, ID, "UndeploysInto", AbstractType::UnitType, UndeploysInto);
        read_rule_type(*ini, ID, "PowersUnit", AbstractType::UnitType, PowersUnit);
        PoweredUnit = ini->ReadBool(ID, "PoweredUnit", PoweredUnit);
        LightningRod = ini->ReadBool(ID, "LightningRod", LightningRod);
        ManualReload = ini->ReadBool(ID, "ManualReload", ManualReload);
        TurretSpins = ini->ReadBool(ID, "TurretSpins", TurretSpins);
        TiltCrashJumpjet = ini->ReadBool(ID, "TiltCrashJumpjet", TiltCrashJumpjet);
        Turret = ini->ReadBool(ID, "Turret", Turret);
        if (!game::type_resources().audio_unavailable) {
            SOUND_VALUE(TurretRotateSound);
            SOUND_VALUE(EnterTransportSound);
            SOUND_VALUE(LeaveTransportSound);
            SOUND_LIST(MoveSound);
            SOUND_LIST(DieSound);
            SOUND_VALUE(DeploySound);
            SOUND_VALUE(UndeploySound);
            SOUND_VALUE(ChronoInSound);
            SOUND_VALUE(ChronoOutSound);
            SOUND_VALUE(VoiceHarvest);
            SOUND_VALUE(VoicePrimaryWeaponAttack);
            SOUND_VALUE(VoicePrimaryEliteWeaponAttack);
            SOUND_VALUE(VoiceSecondaryWeaponAttack);
            SOUND_VALUE(VoiceSecondaryEliteWeaponAttack);
            SOUND_VALUE(VoiceDeploy);
            SOUND_VALUE(VoiceUndeploy);
            SOUND_VALUE(EnterGrinderSound);
            SOUND_VALUE(LeaveGrinderSound);
            SOUND_VALUE(EnterBioReactorSound);
            SOUND_VALUE(LeaveBioReactorSound);
            SOUND_VALUE(ActivateSound);
            SOUND_VALUE(DeactivateSound);
            SOUND_VALUE(MindClearedSound);
        }
#undef SOUND_VALUE
#undef SOUND_LIST
        read_rule_type_list(*ini, ID, "Explosion", AbstractType::AnimType, Explosion);
        read_rule_type_list(*ini, ID, "DestroyAnim", AbstractType::AnimType, DestroyAnim);
        // 0x713B9A..0x71419E: particles use the integer-token coordinate reader.
        read_rule_type(*ini, ID, "NaturalParticleSystem", AbstractType::ParticleSystemType, NaturalParticleSystem);
        read_rule_type(*ini, ID, "RefinerySmokeParticleSystem", AbstractType::ParticleSystemType,
                       RefinerySmokeParticleSystem);
        if (!ini->ReadIntegerCoord(NaturalParticleSystemLocation, ID, "NaturalParticleLocation",
                                   NaturalParticleSystemLocation))
            return false;
        read_rule_type_list(*ini, ID, "DamageParticleSystems", AbstractType::ParticleSystemType, DamageParticleSystems);
        read_rule_type_list(*ini, ID, "DestroyParticleSystems", AbstractType::ParticleSystemType,
                            DestroyParticleSystems);
        if (!ini->ReadIntegerCoord(DamageSmokeOffset, ID, "DamageSmokeOffset", DamageSmokeOffset)) return false;
        DamSmkOffScrnRel = ini->ReadBool(ID, "DamSmkOffScrnRel", DamSmkOffScrnRel);
        if (!ini->ReadIntegerCoord(DestroySmokeOffset, ID, "DestroySmokeOffset", DestroySmokeOffset)) return false;
        if (!ini->ReadIntegerCoord(RefinerySmokeOffsetOne, ID, "RefinerySmokeOffsetOne", RefinerySmokeOffsetOne))
            return false;
        if (!ini->ReadIntegerCoord(RefinerySmokeOffsetTwo, ID, "RefinerySmokeOffsetTwo", RefinerySmokeOffsetTwo))
            return false;
        if (!ini->ReadIntegerCoord(RefinerySmokeOffsetThree, ID, "RefinerySmokeOffsetThree", RefinerySmokeOffsetThree))
            return false;
        if (!ini->ReadIntegerCoord(RefinerySmokeOffsetFour, ID, "RefinerySmokeOffsetFour", RefinerySmokeOffsetFour))
            return false;
        Nominal = ini->ReadBool(ID, "Nominal", Nominal);
        DontScore = ini->ReadBool(ID, "DontScore", DontScore);
        DamageSelf = ini->ReadBool(ID, "DamageSelf", DamageSelf);
        Cloakable = ini->ReadBool(ID, "Cloakable", Cloakable);
        GapGenerator = ini->ReadBool(ID, "GapGenerator", GapGenerator);
        GapRadiusInCells =
            static_cast<char>(ini->ReadInteger(ID, "GapRadiusInCells", static_cast<signed char>(GapRadiusInCells)));
        SuperGapRadiusInCells = static_cast<char>(
            ini->ReadInteger(ID, "SuperGapRadiusInCells", static_cast<signed char>(SuperGapRadiusInCells)));
        Teleporter = ini->ReadBool(ID, "Teleporter", Teleporter);
        Sensors = ini->ReadBool(ID, "Sensors", Sensors);
        IsGattling = ini->ReadBool(ID, "IsGattling", IsGattling);
        WeaponStages = ini->ReadInteger(ID, "WeaponStages", WeaponStages);
        RateUp = ini->ReadInteger(ID, "RateUp", RateUp);
        RateDown = ini->ReadInteger(ID, "RateDown", RateDown);
        // YR 0x712170: Gattling stage thresholds are read independently from
        // TurretCount/WeaponCount; stage transitions use the previous stage's limit.
        if (IsGattling && WeaponStages > 1) {
            if (WeaponStages > 6) return false;
            for (int i = 0; i < WeaponStages; ++i) {
                char key[32];
                std::snprintf(key, sizeof(key), "Stage%d", i + 1);
                WeaponStage[i] = ini->ReadInteger(ID, key, WeaponStage[i]);
                std::snprintf(key, sizeof(key), "EliteStage%d", i + 1);
                EliteStage[i] = ini->ReadInteger(ID, key, EliteStage[i]);
            }
        }
        PipScale = static_cast<::PipScale>(ini->ReadPipScale(ID, "PipScale", static_cast<int>(PipScale)));
        PipsDrawForAll = ini->ReadBool(ID, "PipsDrawForAll", PipsDrawForAll);
        LeptonMindControlOffset = ini->ReadInteger(ID, "LeptonMindControlOffset", LeptonMindControlOffset);
        PixelSelectionBracketDelta = ini->ReadInteger(ID, "PixelSelectionBracketDelta", PixelSelectionBracketDelta);
        PipWrap = ini->ReadInteger(ID, "PipWrap", PipWrap);
        // 0x71419E..0x71469F: prerequisites, admission flags and Airstrike fallbacks.
        read_rule_prerequisites(*ini, ID, "Prerequisite", Prerequisite);
        read_rule_prerequisites(*ini, ID, "PrerequisiteOverride", PrerequisiteOverride);
        Sight = ini->ReadInteger(ID, "Sight", Sight);
        ReselectIfLimboed = ini->ReadBool(ID, "ReselectIfLimboed", ReselectIfLimboed);
        RejoinTeamIfLimboed = ini->ReadBool(ID, "RejoinTeamIfLimboed", RejoinTeamIfLimboed);
        SensorsSight = ini->ReadInteger(ID, "SensorsSight", SensorsSight);
        DetectDisguiseRange = ini->ReadInteger(ID, "DetectDisguiseRange", DetectDisguiseRange);
        BombSight = ini->ReadInteger(ID, "BombSight", BombSight);
        LeadershipRating = ini->ReadInteger(ID, "LeadershipRating", LeadershipRating);
        MindControlRingOffset = ini->ReadInteger(ID, "MindControlRingOffset", MindControlRingOffset);
        BuildTimeMultiplier = static_cast<float>(ini->ReadDouble(ID, "BuildTimeMultiplier", BuildTimeMultiplier));
#define ADMISSION(field) field = ini->ReadBool(ID, #field, field)
        ADMISSION(RevealToAll);
        ADMISSION(Drainable);
        ADMISSION(OpenTopped);
        ADMISSION(ResourceGatherer);
        ADMISSION(ResourceDestination);
        ADMISSION(CanDisguise);
        ADMISSION(PermaDisguise);
        ADMISSION(DetectDisguise);
        ADMISSION(DisguiseWhenStill);
        ADMISSION(CanPassiveAquire);
        ADMISSION(CanRetaliate);
        ADMISSION(CanApproachTarget);
        ADMISSION(CanRecalcApproachTarget);
        ADMISSION(RequiresStolenThirdTech);
        ADMISSION(RequiresStolenSovietTech);
        ADMISSION(RequiresStolenAlliedTech);
#undef ADMISSION
        RequiredHouses =
            static_cast<DWORD>(ini->ReadHouseTypesList(ID, "RequiredHouses", static_cast<int>(RequiredHouses)));
        SecretHouses = static_cast<DWORD>(ini->ReadHouseTypesList(ID, "SecretHouses", static_cast<int>(SecretHouses)));
        ForbiddenHouses =
            static_cast<DWORD>(ini->ReadHouseTypesList(ID, "ForbiddenHouses", static_cast<int>(ForbiddenHouses)));
        TechLevel = ini->ReadInteger(ID, "TechLevel", TechLevel);
        AirstrikeTeam = ini->ReadInteger(ID, "AirstrikeTeam", AirstrikeTeam);
        EliteAirstrikeTeam = ini->ReadInteger(ID, "EliteAirstrikeTeam", AirstrikeTeam);
        read_rule_type(*ini, ID, "AirstrikeTeamType", AbstractType::AircraftType, AirstrikeTeamType);
        // The elite fallback is the updated normal value, not the old elite value.
        EliteAirstrikeTeamType = AirstrikeTeamType;
        read_rule_type(*ini, ID, "EliteAirstrikeTeamType", AbstractType::AircraftType, EliteAirstrikeTeamType);
        AirstrikeRechargeTime = ini->ReadInteger(ID, "AirstrikeRechargeTime", AirstrikeRechargeTime);
        EliteAirstrikeRechargeTime = ini->ReadInteger(ID, "EliteAirstrikeRechargeTime", AirstrikeRechargeTime);
        Speed = read_rule_percentage(*ini, ID, "Speed", Speed);
        // 0x71469F..0x714BC0: costs, firing/reload state and deployment admission.
        Cost = ini->ReadInteger(ID, "Cost", Cost);
        Soylent = ini->ReadInteger(ID, "Soylent", Soylent);
        read_rule_type(*ini, ID, "UnloadingClass", AbstractType::UnitType, UnloadingClass);
        read_rule_type(*ini, ID, "DeployingAnim", AbstractType::AnimType, DeployingAnim);
        InitialAmmo = ini->ReadInteger(ID, "InitialAmmo", InitialAmmo);
        Ammo = ini->ReadInteger(ID, "Ammo", Ammo);
        IFVMode = ini->ReadInteger(ID, "IFVMode", IFVMode);
        AirRangeBonus = read_rule_distance(*ini, ID, "AirRangeBonus", AirRangeBonus);
        RadialFireSegments = ini->ReadInteger(ID, "RadialFireSegments", RadialFireSegments);
        if (!game::type_resources().combat_unavailable) {
            DeployFireWeapon = ini->ReadInteger(ID, "DeployFireWeapon", DeployFireWeapon);
            DeployFire = ini->ReadBool(ID, "DeployFire", DeployFire);
            DeployToLand = ini->ReadBool(ID, "DeployToLand", DeployToLand);
        }
        MobileFire = ini->ReadBool(ID, "MobileFire", MobileFire);
        OpportunityFire = ini->ReadBool(ID, "OpportunityFire", OpportunityFire);
        DistributedFire = ini->ReadBool(ID, "DistributedFire", DistributedFire);
        Reload = ini->ReadInteger(ID, "Reload", Reload);
        EmptyReload = ini->ReadInteger(ID, "EmptyReload", EmptyReload);
        ReloadIncrement = ini->ReadInteger(ID, "ReloadIncrement", ReloadIncrement);
        DamageReducesReadiness = ini->ReadBool(ID, "DamageReducesReadiness", DamageReducesReadiness);
        ReadinessReductionMultiplier =
            static_cast<float>(ini->ReadDouble(ID, "ReadinessReductionMultiplier", ReadinessReductionMultiplier));
#define FIRE_FLAG(field) field = ini->ReadBool(ID, #field, field)
        FIRE_FLAG(BerserkFriendly);
        FIRE_FLAG(SprayAttack);
        FIRE_FLAG(Pushy);
        FIRE_FLAG(Natural);
        FIRE_FLAG(Unnatural);
        FIRE_FLAG(CloseRange);
        FIRE_FLAG(PreventAttackMove);
#undef FIRE_FLAG
        // Both stores use the result read with Points2 as the fallback.
        Points2 = ini->ReadInteger(ID, "Points", Points2);
        Points = Points2;
        ThreatPosed = ini->ReadInteger(ID, "ThreatPosed", ThreatPosed);
        OwnerFlags = static_cast<DWORD>(ini->ReadHouseTypesList(ID, "Owner", static_cast<int>(OwnerFlags)));
        AIBasePlanningSide = ini->ReadInteger(ID, "AIBasePlanningSide", AIBasePlanningSide);
#define TYPE_FLAG(field) field = ini->ReadBool(ID, #field, field)
        TYPE_FLAG(Trainable);
        TYPE_FLAG(Crewed);
        TYPE_FLAG(Gunner);
        TYPE_FLAG(Naval);
        TYPE_FLAG(Repairable);
        TYPE_FLAG(Invisible);
        TYPE_FLAG(RadarVisible);
        TYPE_FLAG(SelfHealing);
        TYPE_FLAG(NoAutoFire);
#undef TYPE_FLAG
        ROT = ini->ReadInteger(ID, "ROT", ROT);
        Passengers = ini->ReadInteger(ID, "Passengers", Passengers);
        FireAngle = ini->ReadInteger(ID, "FireAngle", FireAngle);
        DeployTime = ini->ReadDouble(ID, "DeployTime", DeployTime);
        // The rules/ID lookup also retains Yuri's psychic-wave deployment delay.
        if (!game::type_resources().combat_unavailable)
            UndeployDelay = ini->ReadInteger(ID, "UndeployDelay", UndeployDelay);
        // 0x714BC0..0x715269: immunity, spawn/slave and Jumpjet state.
#define STATE_FLAG(field) field = ini->ReadBool(ID, #field, field)
        STATE_FLAG(Disableable);
        STATE_FLAG(ToProtect);
        STATE_FLAG(TiberiumHeal);
        STATE_FLAG(ImmuneToVeins);
        STATE_FLAG(AllowedToStartInMultiplayer);
        STATE_FLAG(StupidHunt);
        STATE_FLAG(TargetLaser);
        STATE_FLAG(HunterSeeker);
        STATE_FLAG(Crusher);
        STATE_FLAG(OmniCrusher);
        STATE_FLAG(OmniCrushResistant);
        STATE_FLAG(AutoCrush);
        STATE_FLAG(ImmuneToRadiation);
        STATE_FLAG(Underwater);
        STATE_FLAG(BalloonHover);
        STATE_FLAG(Slaved);
        read_rule_type(*ini, ID, "Enslaves", AbstractType::InfantryType, Enslaves);
        SlaveRegenRate = ini->ReadInteger(ID, "SlaveRegenRate", SlaveRegenRate);
        SlavesNumber = ini->ReadInteger(ID, "SlavesNumber", SlavesNumber);
        SlaveReloadRate = ini->ReadInteger(ID, "SlaveReloadRate", SlaveReloadRate);
        OpenTransportWeapon = ini->ReadInteger(ID, "OpenTransportWeapon", OpenTransportWeapon);
        STATE_FLAG(Spawned);
        read_rule_type(*ini, ID, "Spawns", AbstractType::AircraftType, Spawns);
        SpawnRegenRate = ini->ReadInteger(ID, "SpawnRegenRate", SpawnRegenRate);
        SpawnsNumber = ini->ReadInteger(ID, "SpawnsNumber", SpawnsNumber);
        SpawnReloadRate = ini->ReadInteger(ID, "SpawnReloadRate", SpawnReloadRate);
        STATE_FLAG(MissileSpawn);
        STATE_FLAG(DefaultToGuardArea);
        STATE_FLAG(Warpable);
        STATE_FLAG(Parasiteable);
        STATE_FLAG(ImmuneToPsionics);
        STATE_FLAG(ImmuneToPsionicWeapons);
        STATE_FLAG(ConsideredAircraft);
        STATE_FLAG(Bunkerable);
        STATE_FLAG(Organic);
        STATE_FLAG(ImmuneToPoison);
        SuppressionThreshold = ini->ReadInteger(ID, "SuppressionThreshold", SuppressionThreshold);
        STATE_FLAG(NoShadow);
        JumpjetTurnRate = ini->ReadInteger(ID, "JumpjetTurnRate", JumpjetTurnRate);
        JumpjetSpeed = ini->ReadInteger(ID, "JumpjetSpeed", JumpjetSpeed);
        JumpjetClimb = static_cast<float>(ini->ReadDouble(ID, "JumpjetClimb", JumpjetClimb));
        JumpjetCrash = static_cast<float>(ini->ReadDouble(ID, "JumpjetCrash", JumpjetCrash));
        JumpjetHeight = ini->ReadInteger(ID, "JumpjetHeight", JumpjetHeight);
        JumpjetAccel = static_cast<float>(ini->ReadDouble(ID, "JumpjetAccel", JumpjetAccel));
        JumpjetWobbles = static_cast<float>(ini->ReadDouble(ID, "JumpjetWobbles", JumpjetWobbles));
        STATE_FLAG(JumpjetNoWobbles);
        JumpjetDeviation = ini->ReadInteger(ID, "JumpjetDeviation", JumpjetDeviation);
        STATE_FLAG(JumpJet);
        STATE_FLAG(Crashable);
        STATE_FLAG(AttackFriendlies);
        STATE_FLAG(AttackCursorOnFriendlies);
#undef STATE_FLAG
        // 0x715269..0x7153DA: apply the original setters, including missing-key clamps.
        TurretRecoil = ini->ReadBool(ID, "TurretRecoil", TurretRecoil);
        TurretAnimData.SetTravel(ini->ReadInteger(ID, "TurretTravel", TurretAnimData.GetTravel()));
        TurretAnimData.SetCompressFrames(
            ini->ReadInteger(ID, "TurretCompressFrames", TurretAnimData.GetCompressFrames()));
        TurretAnimData.SetHoldFrames(ini->ReadInteger(ID, "TurretHoldFrames", TurretAnimData.GetHoldFrames()));
        TurretAnimData.SetRecoverFrames(ini->ReadInteger(ID, "TurretRecoverFrames", TurretAnimData.GetRecoverFrames()));
        // The target copies four integers plus the flag and all three padding bytes.
        std::memcpy(reinterpret_cast<unsigned char*>(this) + offsetof(TechnoTypeClass, BarrelAnimData),
                    reinterpret_cast<const unsigned char*>(this) + offsetof(TechnoTypeClass, TurretAnimData), 0x14);
        BarrelAnimData.SetTravel(ini->ReadInteger(ID, "BarrelTravel", TurretAnimData.GetTravel()));
        BarrelAnimData.SetCompressFrames(
            ini->ReadInteger(ID, "BarrelCompressFrames", TurretAnimData.GetCompressFrames()));
        BarrelAnimData.SetHoldFrames(ini->ReadInteger(ID, "BarrelHoldFrames", TurretAnimData.GetHoldFrames()));
        BarrelAnimData.SetRecoverFrames(ini->ReadInteger(ID, "BarrelRecoverFrames", TurretAnimData.GetRecoverFrames()));
        // 0x7153DA..0x715793: remaining rules and the ART/ID UseBuffer lookup.
        TiltsWhenCrushes = ini->ReadBool(ID, "TiltsWhenCrushes", TiltsWhenCrushes);
        Accelerates = ini->ReadBool(ID, "Accelerates", Accelerates);
        ZFudgeCliff = ini->ReadInteger(ID, "ZFudgeCliff", ZFudgeCliff);
        ZFudgeColumn = ini->ReadInteger(ID, "ZFudgeColumn", ZFudgeColumn);
        ZFudgeTunnel = ini->ReadInteger(ID, "ZFudgeTunnel", ZFudgeTunnel);
        ZFudgeBridge = ini->ReadInteger(ID, "ZFudgeBridge", ZFudgeBridge);
        AbilitiesStruct parsed{};
        if (!ini->ReadAbilities(reinterpret_cast<byte*>(&parsed), ID, "VeteranAbilities",
                                reinterpret_cast<byte*>(&VeteranAbilities)))
            return false;
        VeteranAbilities = parsed;
        if (!ini->ReadAbilities(reinterpret_cast<byte*>(&parsed), ID, "EliteAbilities",
                                reinterpret_cast<byte*>(&EliteAbilities)))
            return false;
        EliteAbilities = parsed;
        const auto* defaults = RulesClass::Instance;
        // FCOMP's equality bit also selects the Rules fallback for unordered NaN.
        const auto coefficient = [&](const char* key, double& value, double RulesClass::* field) {
            const bool inherited = value == 0.0 || std::isnan(value);
            // Native previews may not have a Rules object; retain their local fallback.
            const double fallback = inherited && defaults ? defaults->*field : value;
            value = ini->ReadDouble(ID, key, fallback);
        };
        coefficient("MyEffectivenessCoefficient", MyEffectivenessCoefficient,
                    &RulesClass::MyEffectivenessCoefficientDefault);
        coefficient("TargetEffectivenessCoefficient", TargetEffectivenessCoefficient,
                    &RulesClass::TargetEffectivenessCoefficientDefault);
        coefficient("TargetSpecialThreatCoefficient", TargetSpecialThreatCoefficient,
                    &RulesClass::TargetSpecialThreatCoefficientDefault);
        coefficient("TargetStrengthCoefficient", TargetStrengthCoefficient,
                    &RulesClass::TargetStrengthCoefficientDefault);
        coefficient("TargetDistanceCoefficient", TargetDistanceCoefficient,
                    &RulesClass::TargetDistanceCoefficientDefault);
        SpecialThreatValue = ini->ReadDouble(ID, "SpecialThreatValue", SpecialThreatValue);
        IsSelectableCombatant = ini->ReadBool(ID, "IsSelectableCombatant", IsSelectableCombatant);
        UseBuffer = game::type_art_ini().ReadBool(ID, "UseBuffer", UseBuffer);
        // 0x715793..0x715850: derive primary state before image replacement and ART.
        HasPrimary = Weapon[0].WeaponType && Weapon[0].WeaponType->Damage > 0;
        char image_filename[260];
        std::snprintf(image_filename, sizeof(image_filename), "%s.SHP", ImageFile);
        if (ImageAllocated && Image) {
            YRMemory::Deallocate(Image);
            Image = nullptr;
            ImageAllocated = false;
        }
        // Non-building types acquire again, independent of Voxel and the parent's path.
        if (WhatAmI() != AbstractType::BuildingType)
            Image = static_cast<SHPStruct*>(FileSystem::LoadFile(image_filename, false));
        auto& art = game::type_art_ini();
        art.ReadString(ImageFile, "Palette", "", PaletteFile, sizeof(PaletteFile));
        // 0x715850..0x7159AC: attach the palette, enforce the loaded-palette limit,
        // then restore ART scalars in target order. Native failure stays at this loader.
        LoadPalette();
        const int loaded_palettes = ColorScheme::CountLoadedPalettes();
        if (game::last_type_resource_status() == game::TypeResourceStatus::failure) return false;
        if (loaded_palettes > 1) {
            game::type_resource_failure(ID);
            return false;
        }
        TurretOffset = art.ReadInteger(ImageFile, "TurretOffset", TurretOffset);
        RotCount = art.ReadInteger(ImageFile, "RotCount", RotCount);
        Remapable = art.ReadBool(ImageFile, "Remapable", Remapable);
        Normalized = art.ReadBool(ImageFile, "Normalized", Normalized);
        VisibleLoad = art.ReadBool(ImageFile, "VisibleLoad", VisibleLoad);
        ShadowIndex = art.ReadInteger(ImageFile, "ShadowIndex", ShadowIndex);
        DisableVoxelCache = art.ReadBool(ImageFile, "DisableVoxelCache", DisableVoxelCache);
        DisableShadowCache = art.ReadBool(ImageFile, "DisableShadowCache", DisableShadowCache);
        // 0x715990..0x715B10: nonempty keys update names, existing icons stay attached,
        // and only the ordinary cameo falls back to XXICON. Ownership is unchanged.
        char cameo_name[0x19];
        art.ReadString(ImageFile, "Cameo", "", cameo_name, sizeof(cameo_name));
        if (*cameo_name) {
            std::strcpy(CameoFile, cameo_name);
            char filename[0x104];
            std::snprintf(filename, sizeof(filename), "%s.SHP", CameoFile);
            if (!Cameo) Cameo = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
        }
        if (!Cameo) Cameo = static_cast<SHPStruct*>(FileSystem::LoadFile("XXICON.SHP", false));
        art.ReadString(ImageFile, "AltCameo", "", cameo_name, sizeof(cameo_name));
        if (*cameo_name) {
            std::strcpy(AltCameoFile, cameo_name);
            char filename[0x104];
            std::snprintf(filename, sizeof(filename), "%s.SHP", AltCameoFile);
            if (!AltCameo) AltCameo = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
        }
        // 0x715B10..0x716090: weapon ART precedes adjacent rendering/spawn fields
        // and the rules movement zone. Elite defaults use the updated normal slot.
        const auto weapon_art = [&](WeaponStruct& slot, const char* flh, const char* length, const char* thickness,
                                    const WeaponStruct& fallback) {
            auto at = fallback.FLH;
            art.ReadPoint3D(slot.FLH, ImageFile, flh, at);
            slot.BarrelLength = art.ReadInteger(ImageFile, length, fallback.BarrelLength);
            slot.BarrelThickness = art.ReadInteger(ImageFile, thickness, fallback.BarrelThickness);
        };
        if (!game::type_resources().combat_unavailable && !HasMultipleTurrets()) {
            weapon_art(Weapon[0], "PrimaryFireFLH", "PBarrelLength", "PBarrelThickness", Weapon[0]);
            weapon_art(Weapon[1], "SecondaryFireFLH", "SBarrelLength", "SBarrelThickness", Weapon[1]);
            weapon_art(EliteWeapon[0], "ElitePrimaryFireFLH", "ElitePBarrelLength", "ElitePBarrelThickness", Weapon[0]);
            weapon_art(EliteWeapon[1], "EliteSecondaryFireFLH", "EliteSBarrelLength", "EliteSBarrelThickness",
                       Weapon[1]);
        } else if (!game::type_resources().combat_unavailable) {
            for (int i = 0; i < WeaponCount; ++i) {
                char flh[64], length[64], thickness[64], locked[64];
                for (int elite = 0; elite < 2; ++elite) {
                    const char* prefix = elite ? "EliteWeapon" : "Weapon";
                    std::snprintf(flh, sizeof(flh), "%s%dFLH", prefix, i + 1);
                    std::snprintf(length, sizeof(length), "%s%dBarrelLength", prefix, i + 1);
                    std::snprintf(thickness, sizeof(thickness), "%s%dBarrelThickness", prefix, i + 1);
                    std::snprintf(locked, sizeof(locked), "%s%dTurretLocked", prefix, i + 1);
                    auto& slot = elite ? EliteWeapon[i] : Weapon[i];
                    weapon_art(slot, flh, length, thickness, Weapon[i]);
                    slot.TurretLocked = art.ReadBool(ImageFile, locked, Weapon[i].TurretLocked);
                }
            }
        }
        if (!game::type_resources().combat_unavailable)
            for (int i = 0; i < 5; ++i) {
                char key[32];
                std::snprintf(key, sizeof(key), "AlternateFLH%d", i);
                auto fallback = Weapon[0].FLH;
                art.ReadPoint3D(AlternativeFLH[i], ImageFile, key, fallback);
            }
        TurretNotExportedOnGround = art.ReadBool(ImageFile, "TurretNotExportedOnGround", TurretNotExportedOnGround);
        art.ReadPoint3D(SecondSpawnOffset, ImageFile, "SecondSpawnOffset", SecondSpawnOffset);
        MovementZone =
            static_cast<::MovementZone>(ini->ReadMovementZone(ID, "MovementZone", static_cast<int>(MovementZone)));
        IsSubterranean = MovementZone == ::MovementZone::Subterrannean;
        // DamageSparks belongs to Infantry's Cyborg-derived state, not a Techno INI key.
        // YR 0x71609C: a voxel type loads its VXL/HVA pairs once these fields are read.
        if (Voxel)
            LoadVoxel();
        else if (Turret)
            LoadSpriteTurretVoxels();
        // 0x7160B4..0x71611D: rocking lift uses the first limb's unsigned dimensions.
        if (MainVoxel.VXL && !MainVoxel.VXL->Initialized) {
            VoxelScaleX = static_cast<unsigned char>(MainVoxel.VXL->leaSectionTailer(0, 0)->size_Y) * 0.5 + 0.5;
            VoxelScaleY = static_cast<unsigned char>(MainVoxel.VXL->leaSectionTailer(0, 0)->size_X) * 0.5;
        }
        return true;
    } catch (...) {
        return false;
    }
}
