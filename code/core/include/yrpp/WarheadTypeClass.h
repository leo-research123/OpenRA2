/*
    Warheads
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

// forward declarations
class AnimTypeClass;
class ParticleSystemTypeClass;
class VoxelAnimTypeClass;

struct WarheadFlags {
    bool ForceFire;
    bool Retaliate;
    bool PassiveAcquire;

    WarheadFlags(bool FF = true, bool Retal = true, bool Acquire = true) : ForceFire(FF), Retaliate(Retal), PassiveAcquire(Acquire) {};
};

class WarheadTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x0075E440; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x0075DEC0
    void ComputeCRC(CRCEngine& crc) const override;
    /// VA: 0x0075D3A0
    bool LoadFromINI(CCINIClass* pINI) override;

    static const AbstractType AbsID = AbstractType::WarheadType;

    // Array
    static DynamicVectorClass<WarheadTypeClass*>& Array;
    static WarheadTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    /// VA: 0x0075E3B0.
    static WarheadTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);

    // IPersist
    /// VA: 0x0075E080.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x0075E0C0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x0075E0C0); }
    /// VA: 0x0075E2C0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x0075E2C0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~WarheadTypeClass();

    // AbstractClass
    /// VA: 0x0075E500.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x0075E4F0.
    virtual int Size() const;

    // AbstractTypeClass

    // Constructor
    /// VA: 0x0075CEC0.
    WarheadTypeClass(const char* pID);

protected:
    explicit __forceinline WarheadTypeClass(noinit_t)
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    double  Deform;

    double Verses [0xB];

    double  ProneDamage;
    int     DeformTreshold;

    TypeList<AnimTypeClass*> AnimList;

    InfDeath InfDeath;
    float   CellSpread;
    float   CellInset;
    float   PercentAtMax;
    bool    CausesDelayKill;
    int     DelayKillFrames;
    float   DelayKillAtMax;
    float   CombatLightSize;
    ParticleSystemTypeClass* Particle; // 0x0075D3A0 resolves through 0x00644890.
    bool    Wall;
    bool    WallAbsoluteDestroyer;
    bool    PenetratesBunker;
    bool    Wood;
    bool    Tiberium;
    bool    unknown_bool_149;
    bool    Sparky;
    bool    Sonic;
    bool    Fire;
    bool    Conventional;
    bool    Rocker;
    bool    DirectRocker;
    bool    Bright;
    bool    CLDisableRed;
    bool    CLDisableGreen;
    bool    CLDisableBlue;
    bool    EMEffect;
    bool    MindControl;
    bool    Poison;
    bool    IvanBomb;
    bool    ElectricAssault;
    bool    Parasite;
    bool    Temporal;
    bool    IsLocomotor;
    GUID    Locomotor;
    bool    Airstrike;
    bool    Psychedelic;
    bool    BombDisarm;
    int     Paralyzes;
    bool    Culling;
    bool    MakesDisguise;
    bool    NukeMaker;
    bool    Radiation;
    bool    PsychicDamage;
    bool    AffectsAllies;
    bool    Bullets;
    bool    Veinhole;
    int     ShakeXlo;
    int     ShakeXhi;
    int     ShakeYlo;
    int     ShakeYhi;

    TypeList<VoxelAnimTypeClass*> DebrisTypes;
    TypeList<int> DebrisMaximums;

    int     MaxDebris;
    int     MinDebris;
    PROTECTED_PROPERTY(DWORD, unused_1CC); //???
};
