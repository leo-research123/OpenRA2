/*
    ParticleSystemTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"

// forward declarations

class ParticleSystemTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x00644700; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00644700); }
    /// VA: 0x006442D0
#if defined(RA2_YRPP_GAME)
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x006442D0); }
#else
    bool LoadFromINI(CCINIClass* ini) override;
#endif

    static const AbstractType AbsID = AbstractType::ParticleSystemType;

    // Array
    static DynamicVectorClass<ParticleSystemTypeClass*>& Array;
    static ParticleSystemTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x00644890.
    static ParticleSystemTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
    // IPersist
    /// VA: 0x006447A0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x006447E0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x006447E0); }
    /// VA: 0x00644830; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x00644830); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~ParticleSystemTypeClass();

    // AbstractClass
    /// VA: 0x00644930.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00644920.
    virtual int Size() const;

    // ObjectTypeClass
    /// VA: 0x00644940; local body in core/src/yrpp.
    virtual bool SpawnAtMapCoords(CellStruct* mcoords, HouseClass* owner);
    /// VA: 0x00644950; local body in core/src/yrpp.
    virtual ObjectClass* CreateObject(HouseClass* owner);

    // Constructor
    /// VA: 0x006440A0.
    ParticleSystemTypeClass(const char* pID);

protected:
    explicit __forceinline ParticleSystemTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    int      HoldsWhat; //ParticleType Array index
    bool     Spawns;
    int      SpawnFrames;
    float    Slowdown;
    int      ParticleCap;
    int      SpawnRadius;
    float    SpawnCutoff;
    float    SpawnTranslucencyCutoff;
    BehavesLike BehavesLike;
    int      Lifetime;
    Vector3D<float> SpawnDirection;
    double   ParticlesPerCoord;
    double   SpiralDeltaPerCoord;
    double   SpiralRadius;
    double   PositionPerturbationCoefficient;
    double   MovementPerturbationCoefficient;
    double   VelocityPerturbationCoefficient;
    double   SpawnSparkPercentage;
    int      SparkSpawnFrames;
    int      LightSize;
    ColorStruct LaserColor;
    bool     Laser;
    bool     OneFrameLight;
};
