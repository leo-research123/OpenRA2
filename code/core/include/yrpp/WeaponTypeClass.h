/*
    Weapons
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

// forward declarations
class AnimTypeClass;
class BulletTypeClass;
class ParticleSystemTypeClass;
class WarheadTypeClass;

class WeaponTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x00772AE0
    void ComputeCRC(CRCEngine& crc) const override;
    /// VA: 0x00772080
    bool LoadFromINI(CCINIClass* pINI) override;

    static const AbstractType AbsID = AbstractType::WeaponType;

    // Array
    static DynamicVectorClass<WeaponTypeClass*>& Array;
    static WeaponTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    /// VA: 0x00772FA0.
    static WeaponTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);

    // IPersist
    /// VA: 0x00772C90.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00772CD0
    HRESULT YRPP_STDCALL Load(IStream* pStm) override;
    /// VA: 0x00772EB0
    HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) override;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~WeaponTypeClass();

    // AbstractClass
    /// VA: 0x007730E0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x007730D0.
    virtual int Size() const;

    // AbstractTypeClass

    /// VA: 0x007729F0
    void CalculateSpeed();
    /// VA: 0x00773070
    int GetSpeed(int distance) const;

    /// VA: 0x00772A90
    ThreatType AllowedThreats();
    /// VA: 0x00772AC0
    bool IsWallDestroyer() const;

    // Constructor
    /// VA: 0x00771C70.
    WeaponTypeClass(const char* pID = nullptr);

protected:
    explicit __forceinline WeaponTypeClass(noinit_t)
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    int AmbientDamage;
    int Burst;
    BulletTypeClass* Projectile;
    int Damage;
    int Speed;
    WarheadTypeClass* Warhead;
    int ROF;
    int Range; // int(256 * ini value)
    int MinimumRange; // int(256 * ini value)
    DECLARE_PROPERTY(TypeList<int>, Report);		//sound indices
    DECLARE_PROPERTY(TypeList<int>, DownReport);	//sound indices
    DECLARE_PROPERTY(TypeList<AnimTypeClass*>, Anim);
    AnimTypeClass* OccupantAnim;
    AnimTypeClass* AssaultAnim;
    AnimTypeClass* OpenToppedAnim;
    ParticleSystemTypeClass* AttachedParticleSystem;
    ColorStruct LaserInnerColor;
    ColorStruct LaserOuterColor;
    ColorStruct LaserOuterSpread;
    bool UseFireParticles;
    bool UseSparkParticles;
    bool OmniFire;
    bool DistributedWeaponFire;
    bool IsRailgun;
    bool Lobber;
    bool Bright;
    bool IsSonic;
    bool Spawner;
    bool LimboLaunch;
    bool DecloakToFire;
    bool CellRangefinding;
    bool FireOnce;
    bool NeverUse;
    bool RevealOnFire;
    bool TerrainFire;
    bool SabotageCursor;
    bool MigAttackCursor;
    bool DisguiseFireOnly;
    int DisguiseFakeBlinkTime;
    bool InfiniteMindControl;
    bool FireWhileMoving;
    bool DrainWeapon;
    bool FireInTransport;
    bool Suicide;
    bool TurboBoost;
    bool Supress;
    bool Camera;
    bool Charges;
    bool IsLaser;
    bool DiskLaser;
    bool IsLine;
    bool IsBigLaser;
    bool IsHouseColor;
    char LaserDuration;
    bool IonSensitive;
    bool AreaFire;
    bool IsElectricBolt;
    bool DrawBoltAsLaser;
    bool IsAlternateColor;
    bool IsRadBeam;
    bool IsRadEruption;
    int RadLevel;
    bool IsMagBeam;
};
