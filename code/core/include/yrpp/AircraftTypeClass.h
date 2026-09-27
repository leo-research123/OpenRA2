#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TechnoTypeClass.h"

class AircraftTypeClass : public TechnoTypeClass
{
public:
    /// VA: 0x0041CE20; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x0041CE20); }
    /// VA: 0x0041CE90; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x0041CE90); }
    /// VA: 0x0041CDB0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x0041CDB0); }
    /// VA: 0x0041CFD0; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x0041CC20; reference retained, not a local implementation.
#if defined(RA2_YRPP_GAME)
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x0041CC20); }
#else
    bool LoadFromINI(CCINIClass* pINI) override;
#endif

    static const AbstractType AbsID = AbstractType::AircraftType;

    // Array
    static DynamicVectorClass<AircraftTypeClass*>& Array;
    static AircraftTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x0041CEB0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~AircraftTypeClass();

    // AbstractClass
    /// VA: 0x0041CFB0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x0041CFC0.
    virtual int	Size() const;

    // ObjectTypeClass
    /// VA: 0x0041CBE0; local body in core/src/yrpp.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner);
    /// VA: 0x0041CB20; reference retained, not a local implementation.
#if defined(RA2_YRPP_GAME)
    virtual ObjectClass* CreateObject(HouseClass* pOwner) { JMP_THIS(0x0041CB20); }
#else
    ObjectClass* CreateObject(HouseClass* pOwner) override;
#endif

    // TechnoTypeClass

    // Constructor
    /// VA: 0x0041C8B0.
    AircraftTypeClass(const char* pID);

protected:
    explicit __forceinline AircraftTypeClass(noinit_t) noexcept
        : TechnoTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    bool Carryall;
    AnimTypeClass* Trailer;
    int SpawnDelay;
    bool Rotors;
    bool CustomRotor;
    bool Landable;
    bool FlyBy;
    bool FlyBack;
    bool AirportBound;
    bool Fighter;
};
