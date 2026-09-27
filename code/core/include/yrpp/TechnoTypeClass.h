/*
    TechnoTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/WeaponTypeClass.h"

// forward declarations
class AircraftTypeClass;
class AnimTypeClass;
class BuildingTypeClass;
class InfantryTypeClass;
class ParticleSystemTypeClass;
class VoxelAnimTypeClass;
class UnitTypeClass;

struct AbilitiesStruct
{
    bool FASTER; //0x00
    bool STRONGER; //0x01
    bool FIREPOWER; //0x02
    bool SCATTER; //0x03
    bool ROF; //0x04
    bool SIGHT; //0x05
    bool CLOAK; //0x06
    bool TIBERIUM_PROOF; //0x07
    bool VEIN_PROOF; //0x08
    bool SELF_HEAL; //0x09
    bool EXPLODES; //0x0A
    bool RADAR_INVISIBLE; //0x0B
    bool SENSORS; //0x0C
    bool FEARLESS; //0x0D
    bool C4; //0x0E
    bool TIBERIUM_HEAL; //0x0F
    bool GUARD_AREA; //0x10
    bool CRUSHER; //0x11

    bool& operator[](Ability i)
    {
        return reinterpret_cast<bool*>(this)[static_cast<int>(i)];
    }
};

struct TurretControl
{
    int Travel;
    int CompressFrames;
    int RecoverFrames;
    int HoldFrames;
};

struct WeaponStruct
{
    WeaponTypeClass*  WeaponType;
    CoordStruct       FLH;
    int               BarrelLength;
    int               BarrelThickness;
    bool              TurretLocked;

    WeaponStruct() : WeaponType(nullptr),
        FLH(CoordStruct::Empty),
        BarrelLength(0),
        BarrelThickness(0),
        TurretLocked(false)
    { }

    bool operator == (const WeaponStruct& pWeap) const
        { return false; }
};

class TechnoTypeClass : public ObjectTypeClass
{
public:
    // Original LoadFromINI entry; the local override is not declared here yet.
    /// VA: 0x00712170.

    /// Global VA: 0x00A8EB00.
    static DynamicVectorClass<TechnoTypeClass*>& Array;
    static TechnoTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    static constexpr auto MaxWeapons = 18;

    /// VA: 0x716290
    int GetPipMax() const override;

    // IPersistStream
    /// VA: 0x007162F0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x007162F0); }
    /// VA: 0x00716DC0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x00716DC0); }
    /// VA: 0x007170A0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) { JMP_STD(0x007170A0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TechnoTypeClass();

    /// VA: 0x00712170; native map-display subset, full original reader retained.
#if defined(RA2_YRPP_GAME)
    bool LoadFromINI(CCINIClass* ini) override { JMP_THIS(0x00712170); }
#else
    bool LoadFromINI(CCINIClass* ini) override;
#endif
    // ObjectTypeClass
    /// VA: 0x00711F00
    int GetActualCost(HouseClass* house) const override;
    /// VA: 0x00711EE0
    int GetBuildSpeed() const override;

    /// VA: 0x711EC0
    DWORD GetOwners() const override { return OwnerFlags; }
    /// VA: 0x712040
    SHPStruct* GetCameo() const override;

    // TechnoTypeClass
    /// VA: 0x00711E80; local body in core/src/yrpp.
    virtual bool CanUseWaypoint() const;
    /// VA: 0x00711E90
    virtual bool CanAttackMove() const { return Weapon[0].WeaponType && !PreventAttackMove; }
    /// VA: 0x00716150
#if defined(RA2_YRPP_GAME)
    virtual bool CanCreateHere(const CellStruct& mapCoords, HouseClass* pOwner) const { JMP_THIS(0x00716150); }
#else
    virtual bool CanCreateHere(const CellStruct& mapCoords, HouseClass* pOwner) const;
#endif
    /// VA: 0x00711EB0; reference retained, not a local implementation.
    virtual int GetCost() const { return Cost; }
    /// VA: 0x007120D0
#if defined(RA2_YRPP_GAME)
    virtual int GetRepairStepCost() const { JMP_THIS(0x007120D0); }
#else
    virtual int GetRepairStepCost() const;
#endif
    /// VA: 0x00712120
#if defined(RA2_YRPP_GAME)
    virtual int GetRepairStep() const { JMP_THIS(0x00712120); }
#else
    virtual int GetRepairStep() const;
#endif
    /// VA: 0x00711F60
#if defined(RA2_YRPP_GAME)
    virtual int GetRefund(HouseClass* house, bool full) const { JMP_THIS(0x00711F60); }
#else
    virtual int GetRefund(HouseClass* house, bool full) const;
#endif
    /// VA: 0x00717800; reference retained, not a local implementation.
#if defined(RA2_YRPP_GAME)
    virtual int GetFlightLevel() const { JMP_THIS(0x00717800); }
#else
    virtual int GetFlightLevel() const;
#endif

    // non-virtual
    /// VA: 0x0048DCD0.
    static TechnoTypeClass* YRPP_FASTCALL GetByTypeAndIndex(AbstractType abs,int index);

    bool HasMultipleTurrets() const
    {
        return this->TurretCount > 0;
    }

    /// VA: 0x007178C0.
    CoordStruct* GetParticleSysOffset(CoordStruct* pBuffer) const
        { JMP_THIS(0x7178C0); }

    CoordStruct GetParticleSysOffset() const
    {
        CoordStruct buffer;
        GetParticleSysOffset(&buffer);
        return buffer;
    }

    bool InOwners(DWORD const bitHouseType) const {
        return 0u != (this->GetOwners() & bitHouseType);
    }

    bool InRequiredHouses(DWORD const bitHouseType) const {
        auto const test = this->RequiredHouses;
        if(static_cast<int>(test) == -1) {
            return true;
        }
        return 0u != (test & bitHouseType);
    }

    bool InForbiddenHouses(DWORD const bitHouseType) const {
        auto const test = this->ForbiddenHouses;
        if(static_cast<int>(test) == -1) {
            return false;
        }
        return 0u != (test & bitHouseType);
    }

    // weapon related
    /// VA: 0x007177C0.
    WeaponStruct* GetWeapon(int index)
    { JMP_THIS(0x7177C0); }

    /// VA: 0x007177E0.
    WeaponStruct* GetEliteWeapon(int index)
    { JMP_THIS(0x7177E0); }

    // weapon related
    WeaponStruct& GetWeapon(size_t const index, bool const elite) {
        return elite ? this->EliteWeapon[index] : this->Weapon[index];
    }

    WeaponStruct const& GetWeapon(size_t const index, bool const elite) const {
        return elite ? this->EliteWeapon[index] : this->Weapon[index];
    }

    // Constructor
    /// VA: 0x00710AF0.
    TechnoTypeClass(const char* id, SpeedType speedtype);

protected:
    explicit __forceinline TechnoTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    int             WalkRate;
    int             IdleRate;
    AbilitiesStruct VeteranAbilities;
    AbilitiesStruct EliteAbilities;
    double          SpecialThreatValue;
    double          MyEffectivenessCoefficient;
    double          TargetEffectivenessCoefficient;
    double          TargetSpecialThreatCoefficient;
    double          TargetStrengthCoefficient;
    double          TargetDistanceCoefficient;
    double          ThreatAvoidanceCoefficient;
    int             SlowdownDistance;
    DWORD align_2FC;
    double          DecelerationFactor;
    double          AccelerationFactor;
    int             CloakingSpeed;
    TypeList<VoxelAnimTypeClass*> DebrisTypes;
    TypeList<int> DebrisMaximums;
    GUID            Locomotor;
    DWORD align_35C;
    double          VoxelScaleX;
    double          VoxelScaleY;
    double          Weight;
    double          PhysicalSize;
    double          Size;
    double          SizeLimit;
    bool            HoverAttack;
    int             VHPScan;
    int             unknown_int_398;
    DWORD align_39C;
    double          RollAngle;
    double          PitchSpeed;
    double          PitchAngle;
    int             BuildLimit;
    Category        Category;
    DWORD           unknown_3C0;
    DWORD align_3C4;
    double          DeployTime;
    int             FireAngle;
    PipScale        PipScale;
    bool            PipsDrawForAll;
    int             LeptonMindControlOffset;
    int             PixelSelectionBracketDelta;
    int             PipWrap;
    TypeList<BuildingTypeClass*> Dock;
    BuildingTypeClass* DeploysInto;
    UnitTypeClass*  UndeploysInto;
    UnitTypeClass*  PowersUnit;
    bool            PoweredUnit;
    TypeList<int> VoiceSelect;
    TypeList<int> VoiceSelectEnslaved;
    TypeList<int> VoiceSelectDeactivated;
    TypeList<int> VoiceMove;
    TypeList<int> VoiceAttack;
    TypeList<int> VoiceSpecialAttack;
    TypeList<int> VoiceDie;
    TypeList<int> VoiceFeedback;
    TypeList<int> MoveSound;
    TypeList<int> DieSound;
    int             AuxSound1;
    int             AuxSound2;
    int             CreateSound;
    int             DamageSound;
    int             ImpactWaterSound;
    int             ImpactLandSound;
    int             CrashingSound;
    int             SinkingSound;
    int             VoiceFalling;
    int             VoiceCrashing;
    int             VoiceSinking;
    int             VoiceEnter;
    int             VoiceCapture;
    int             TurretRotateSound;
    int             EnterTransportSound;
    int             LeaveTransportSound;
    int             DeploySound;
    int             UndeploySound;
    int             ChronoInSound;
    int             ChronoOutSound;
    int             VoiceHarvest;
    int             VoicePrimaryWeaponAttack;
    int             VoicePrimaryEliteWeaponAttack;
    int             VoiceSecondaryWeaponAttack;
    int             VoiceSecondaryEliteWeaponAttack;
    int             VoiceDeploy;
    int             VoiceUndeploy;
    int             EnterGrinderSound;
    int             LeaveGrinderSound;
    int             EnterBioReactorSound;
    int             LeaveBioReactorSound;
    int             ActivateSound;
    int             DeactivateSound;
    int             MindClearedSound;
    MovementZone    MovementZone;
    int             GuardRange;
    int             MaxDebris;
    int             MinDebris;
    TypeList<AnimTypeClass*> DebrisAnims;
    int             Passengers;
    bool            OpenTopped;
    int             Sight;
    bool            ResourceGatherer;
    bool            ResourceDestination;
    bool            RevealToAll;
    bool            Drainable;
    int             SensorsSight;
    int             DetectDisguiseRange;
    int             BombSight;
    int             LeadershipRating;
    NavalTargetingType NavalTargeting;
    LandTargetingType LandTargeting;
    float           BuildTimeMultiplier;
    int             MindControlRingOffset;
    int             Cost;
    int             Soylent;
    int             FlightLevel;
    int             AirstrikeTeam;
    int             EliteAirstrikeTeam;
    AircraftTypeClass* AirstrikeTeamType;
    AircraftTypeClass* EliteAirstrikeTeamType;
    int             AirstrikeRechargeTime;
    int             EliteAirstrikeRechargeTime;
    int             TechLevel;
    TypeList<int> Prerequisite;
    TypeList<int> PrerequisiteOverride;
    int             ThreatPosed;
    int             Points;
    int             Speed;
    SpeedType       SpeedType;
    int             InitialAmmo;
    int             Ammo;
    int             IFVMode;
    int             AirRangeBonus;
    bool            BerserkFriendly;
    bool            SprayAttack;
    bool            Pushy;
    bool            Natural;
    bool            Unnatural;
    bool            CloseRange;
    int             Reload;
    int             EmptyReload;
    int             ReloadIncrement;
    int             RadialFireSegments;
    int             DeployFireWeapon;
    bool            DeployFire;
    bool            DeployToLand;
    bool            MobileFire;
    bool            OpportunityFire;
    bool            DistributedFire;
    bool            DamageReducesReadiness;
    float           ReadinessReductionMultiplier;
    UnitTypeClass*  UnloadingClass;
    AnimTypeClass*  DeployingAnim;
    bool            AttackFriendlies;
    bool            AttackCursorOnFriendlies;
    int             UndeployDelay;
    bool            PreventAttackMove;
    DWORD           OwnerFlags;
    int             AIBasePlanningSide;
    bool            StupidHunt;
    bool            AllowedToStartInMultiplayer;
    char            CameoFile[0x19];
    PROTECTED_PROPERTY(BYTE,  align_6EF);
    SHPStruct*      Cameo;
    bool            CameoAllocated;
    char            AltCameoFile[0x19];
    PROTECTED_PROPERTY(BYTE,  align_70E[2]);
    SHPStruct*      AltCameo;
    bool            AltCameoAllocated;
    int             RotCount;
    int             ROT;
    int             TurretOffset;
    bool            CanBeHidden;
    int             Points2; //twice
    TypeList<AnimTypeClass*> Explosion;
    TypeList<AnimTypeClass*> DestroyAnim;
    ParticleSystemTypeClass* NaturalParticleSystem;
    CoordStruct NaturalParticleSystemLocation;
    ParticleSystemTypeClass* RefinerySmokeParticleSystem;
    TypeList<ParticleSystemTypeClass*> DamageParticleSystems;
    TypeList<ParticleSystemTypeClass*> DestroyParticleSystems;
    CoordStruct DamageSmokeOffset;
    bool            DamSmkOffScrnRel;
    CoordStruct DestroySmokeOffset;
    CoordStruct RefinerySmokeOffsetOne;
    CoordStruct RefinerySmokeOffsetTwo;
    CoordStruct RefinerySmokeOffsetThree;
    CoordStruct RefinerySmokeOffsetFour;
    int             ShadowIndex;
    int             Storage;
    bool            TurretNotExportedOnGround;
    bool            Gunner;
    bool            HasTurretTooltips;
    int             TurretCount;
    int             WeaponCount;
    bool            IsChargeTurret;
    int             TurretWeapon[MaxWeapons];
    CoordStruct     AlternativeFLH[5];
    WeaponStruct	Weapon[MaxWeapons];
    bool            ClearAllWeapons;
    WeaponStruct	EliteWeapon[MaxWeapons];
    bool            TypeImmune;
    bool            MoveToShroud;
    bool            Trainable;
    bool            DamageSparks; //enabled for Cyborg InfantryTypes
    bool            TargetLaser;
    bool            ImmuneToVeins;
    bool            TiberiumHeal;
    bool            CloakStop;
    bool            IsTrain;
    bool            IsDropship;
    bool            ToProtect;
    bool            Disableable;
    bool            Unbuildable; //always false, if true it cannot be built from sidebar
    bool            DoubleOwned;
    bool            Invisible;
    bool            RadarVisible;
    bool            HasPrimary; //not loaded from the INIs
    bool            Sensors;
    bool            Nominal;
    bool            DontScore;
    bool            DamageSelf;
    bool            Turret;
    bool            TurretRecoil;
    TurretControl   TurretAnimData;
    bool            unknown_bool_CB4; //always false?
    TurretControl   BarrelAnimData;
    bool            unknown_bool_CC8; //always false?

protected:
    BYTE align_CC9, align_CCA, align_CCB;

public:
    bool            Repairable;
    bool            Crewed;
    bool            Naval;
    bool            Remapable;
    bool            Cloakable;
    bool            GapGenerator;
    char            GapRadiusInCells;
    char            SuperGapRadiusInCells;
    bool            Teleporter;
    bool            IsGattling;
    int             WeaponStages;
    int WeaponStage [6];
    int EliteStage [6];
    int             RateUp;
    int             RateDown;
    bool            SelfHealing;
    bool            Explodes;
    WeaponTypeClass* DeathWeapon;
    float           DeathWeaponDamageModifier;
    bool            NoAutoFire;
    bool            TurretSpins;
    bool            TiltCrashJumpjet;
    bool            Normalized;
    bool            ManualReload;
    bool            VisibleLoad;
    bool            LightningRod;
    bool            HunterSeeker;
    bool            Crusher;
    bool            OmniCrusher;
    bool            OmniCrushResistant;
    bool            TiltsWhenCrushes;
    bool            IsSubterranean;
    bool            AutoCrush;
    bool            Bunkerable;
    bool            CanDisguise;
    bool            PermaDisguise;
    bool            DetectDisguise;
    bool            DisguiseWhenStill;
    bool            CanApproachTarget;
    bool            CanRecalcApproachTarget;
    bool            ImmuneToPsionics;
    bool            ImmuneToPsionicWeapons;
    bool            ImmuneToRadiation;
    bool            Parasiteable;
    bool            DefaultToGuardArea;
    bool            Warpable;
    bool            ImmuneToPoison;
    bool            ReselectIfLimboed;
    bool            RejoinTeamIfLimboed;
    bool            Slaved;
    InfantryTypeClass* Enslaves;
    int             SlavesNumber;
    int             SlaveRegenRate;
    int             SlaveReloadRate;
    int             OpenTransportWeapon;
    bool            Spawned;
    AircraftTypeClass* Spawns;
    int             SpawnsNumber;
    int             SpawnRegenRate;
    int             SpawnReloadRate;
    bool            MissileSpawn;
    bool            Underwater;
    bool            BalloonHover;
    int             SuppressionThreshold;
    int             JumpjetTurnRate;
    int             JumpjetSpeed;
    float           JumpjetClimb;
    float           JumpjetCrash;
    int             JumpjetHeight;
    float           JumpjetAccel;
    float           JumpjetWobbles;
    bool            JumpjetNoWobbles;
    int             JumpjetDeviation;
    bool            JumpJet;
    bool            Crashable;
    bool            ConsideredAircraft;
    bool            Organic;
    bool            NoShadow;
    bool            CanPassiveAquire;
    bool            CanRetaliate;
    bool            RequiresStolenThirdTech;
    bool            RequiresStolenSovietTech;
    bool            RequiresStolenAlliedTech;
    DWORD           RequiredHouses;
    DWORD           ForbiddenHouses;
    DWORD           SecretHouses;
    bool            UseBuffer;
    CoordStruct SecondSpawnOffset;
    bool            IsSelectableCombatant;
    bool            Accelerates;
    bool            DisableVoxelCache;
    bool            DisableShadowCache;
    int             ZFudgeCliff;
    int             ZFudgeColumn;
    int             ZFudgeTunnel;
    int             ZFudgeBridge;
    char            PaletteFile[0x20];
    DynamicVectorClass<ColorScheme*>*           Palette; //no... idea....
    DWORD           align_DF4;
};
