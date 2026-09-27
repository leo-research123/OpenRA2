/*
    TerrainTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"

class TerrainTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x0071E140; local body in core/src/yrpp.
    virtual void ComputeCRC(CRCEngine& crc) const override;
    static const AbstractType AbsID = AbstractType::TerrainType;

    // Array
    static DynamicVectorClass<TerrainTypeClass*>& Array;
    static TerrainTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x0071E2A0.
    static TerrainTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x0071E1D0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x0071E1D0); }
    /// VA: 0x0071E240; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x0071E240); }

    // Destructor
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual ~TerrainTypeClass();

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual int GetArrayIndex() const override;

    // Local common + derived configuration path; dependencies are explicit.
    // Original foundation lookup table is populated at runtime.
    /// Global VA: 0x00B0EEC0.
    /// VA: 0x0071DEA0.
    virtual bool LoadFromINI(CCINIClass* ini) override;
    /// VA: 0x0071DEA0; retained when the exact foundation table is unavailable.
    bool LoadFromINIOriginal(CCINIClass* ini) { JMP_THIS(0x0071DEA0); }

    // ObjectTypeClass
    /// VA: 0x0071DDD0; reference retained, not a local implementation.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords,HouseClass* pOwner) override { JMP_THIS(0x0071DDD0); }
    /// VA: 0x0071DE10; reference retained, not a local implementation.
    virtual ObjectClass* CreateObject(HouseClass* owner) override { JMP_THIS(0x0071DE10); }

    // Constructor
    /// VA: 0x0071DA80.
    TerrainTypeClass(const char* id) noexcept;

protected:
    explicit __forceinline TerrainTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    int Foundation;
    ColorStruct RadarColor;
    int AnimationRate;
    float AnimationProbability;
    int TemperateOccupationBits;
    int SnowOccupationBits;
    bool WaterBound;
    bool SpawnsTiberium;
    bool IsFlammable;
    bool IsAnimated;
    bool IsVeinhole;
    CellStruct* FoundationData;
};
