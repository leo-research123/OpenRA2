/*
    Particles
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/ParticleTypeClass.h"

// forward declarations
class ParticleSystemClass;

class NOVTABLE ParticleClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::Particle;

    // Static
    /// Global VA: 0x00A83DC8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<ParticleClass*>, Array, 0xA83DC8u)
#else
    static DynamicVectorClass<ParticleClass*>& Array;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x0062D9A0
    ~ParticleClass() override;

    // AbstractClass
    /// VA: 0x0062D980
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x0062D970
    int Size() const override { return sizeof(*this); }
    /// VA: 0x0062D990
    ObjectTypeClass* GetType() const override { return Type; }
    /// VA: 0x0062D770
    Layer InWhichLayer() const override { return Layer::Ground; }
    /// VA: 0x0062CEC0
    void DrawIt(Point2D* point,RectangleStruct* bounds) const override;
    /// VA: 0x0062CE40
    void BehaviorAI();
    /// VA: 0x0062C6E0
    void SparkAI();
    /// VA: 0x0062C540
    void SmokeAI();
    /// VA: 0x0062D3F0
    void SmokeMotionAI();

    // ParticleClass
    /// VA: unknown (legacy placeholder).
    virtual int vt_entry_1E8() R0;

    // Constructor
    /// VA: 0x0062B5E0.
#if defined(RA2_YRPP_GAME)
    ParticleClass(
        ParticleTypeClass* pParticleType, CoordStruct* pCrd1,
        CoordStruct* pCrd2, ParticleSystemClass* pParticleSystem) noexcept
        : ParticleClass(noinit_t())
    { JMP_THIS(0x62B5E0); }
#else
    ParticleClass(ParticleTypeClass* type,CoordStruct* origin,CoordStruct* target,ParticleSystemClass* system) noexcept;
#endif

protected:
    explicit __forceinline ParticleClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    ParticleTypeClass* Type;
    RGBClass Color; // 0xB0
    int ColorIndex; // 0xB4
    double ColorAccum; // 0xB8
    CoordStruct Velocity;
    Vector3D<float> GasVelocity; // 0xCC, default (0,0,-1).
    DWORD  unknown_D8;
    DWORD  unknown_DC;
    DWORD  unknown_E0;
    float  Speed;
    CoordStruct unknown_coords_E8; //Crd2 in CTOR
    CoordStruct unknown_coords_F4; //Crd1 in CTOR
    CoordStruct unknown_coords_100; //{ 0, 0, 0} in CTOR
    Vector3D<float> MovementDirection;
    Vector3D<float> PrecisePosition;
    ParticleSystemClass*   ParticleSystem;
    WORD   RemainingEC;
    WORD   RemainingDC;
    BYTE   StateAIAdvance;
    BYTE   unknown_12D;
    BYTE   StartStateAI;
    BYTE   Translucency;
    BYTE   unknown_130;
    BYTE   IsToDie;
    PROTECTED_PROPERTY(DWORD,        unused_134); //??
};
