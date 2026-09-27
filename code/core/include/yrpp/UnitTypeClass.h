#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TechnoTypeClass.h"

class TechnoClass;

class UnitTypeClass : public TechnoTypeClass
{
public:
    /// VA: 0x00747F70; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00747F70); }
    /// VA: 0x00748180; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x747620
#if defined(RA2_YRPP_GAME)
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x00747620); }
#else
    bool LoadFromINI(CCINIClass* ini) override;
#endif

    static const AbstractType AbsID = AbstractType::UnitType;

    // Array
    static DynamicVectorClass<UnitTypeClass*>& Array;
    static UnitTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x00747F30.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00748010; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x00748010); }
    /// VA: 0x007480B0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x007480B0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~UnitTypeClass();

    // AbstractClass
    /// VA: 0x00748170.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00748160.
    virtual int Size() const;

    // AbstractTypeClass

    // ObjectTypeClass
    /// VA: 0x7474B0
#if defined(RA2_YRPP_GAME)
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner) { JMP_THIS(0x007474B0); }
#else
    bool SpawnAtMapCoords(CellStruct* cell, HouseClass* owner) override;
#endif
    /// VA: 0x747560
#if defined(RA2_YRPP_GAME)
    virtual ObjectClass* CreateObject(HouseClass* pOwner) { JMP_THIS(0x00747560); }
#else
    ObjectClass* CreateObject(HouseClass* owner) override;
#endif

    // TechnoTypeClass

    // Constructor
    /// VA: 0x007470D0.
    UnitTypeClass(const char* pID);

protected:
    explicit __forceinline UnitTypeClass(noinit_t) noexcept
        : TechnoTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    LandType MovementRestrictedTo;
    CoordStruct HalfDamageSmokeLocation;
    bool Passive;
    bool CrateGoodie;
    bool Harvester;
    bool Weeder;
    bool unknown_E10;
    bool HasTurret; //not read from the INIs
    bool DeployToFire;
    bool IsSimpleDeployer;
    bool IsTilter;
    bool UseTurretShadow;
    bool TooBigToFitUnderBridge;
    bool CanBeach;
    bool SmallVisceroid;
    bool LargeVisceroid;
    bool CarriesCrate;
    bool NonVehicle;
    int StandingFrames;
    int DeathFrames;
    int DeathFrameRate;
    int StartStandFrame;
    int StartWalkFrame;
    int StartFiringFrame;
    int StartDeathFrame;
    int MaxDeathCounter;
    int Facings;
    int FiringSyncFrame0;
    int FiringSyncFrame1;
    int BurstDelay0;
    int BurstDelay1;
    int BurstDelay2;
    int BurstDelay3;
    SHPStruct* AltImage;
    char WalkFrames;
    char FiringFrames;
    char AltImageFile [0x19];
};
