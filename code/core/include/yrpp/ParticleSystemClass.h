/*
    ParticleSystems
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ParticleClass.h"

class NOVTABLE ParticleSystemClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::ParticleSystem;
    /// Global VA: 0x00A8ED78.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(ParticleSystemClass*, DefaultSystem, 0xA8ED78u)

    // Static
    /// Global VA: 0x00A80208.
    DEFINE_REFERENCE(DynamicVectorClass<ParticleSystemClass*>, Array, 0xA80208u)
#else
    static ParticleSystemClass*& DefaultSystem;
    static DynamicVectorClass<ParticleSystemClass*>& Array;
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
    /// VA: 0x0062E070
    ~ParticleSystemClass() override;

    // AbstractClass
    /// VA: 0x00630210
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x00630200
    int Size() const override { return sizeof(*this); }
    /// VA: 0x00630220
    ObjectTypeClass* GetType() const override { return Type; }
    /// VA: 0x0062FE80
    Layer InWhichLayer() const override { return Layer::Ground; }
    /// VA: 0x0062FE60
    bool IsDead() const override { return TimeToDie&&!Particles.Count; }
    /// VA: 0x006301E0
    void UnInit() override { TimeToDie=true; }
    /// VA: 0x0062FE90
    void PointerExpired(AbstractClass* object,bool removed) override;
    /// VA: 0x0062FD60
    void Update() override;
    /// VA: 0x0062E840
    void SparkAI();
    /// VA: 0x0062ED40
    void SmokeAI();
    /// VA: 0x0062E280
    void DrawIt(Point2D* point,RectangleStruct* bounds) const override;

    /// VA: 0x0062E380.
    ParticleClass* SpawnParticle(const CoordStruct& coords1, const CoordStruct& coords2)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x62E380); }
#else
        ;
#endif

    /// VA: 0x0062E430.
    ParticleClass* SpawnParticle(ParticleTypeClass* pType, const CoordStruct& coords)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x62E430); }
#else
        ;
#endif

    // Constructor
    /// VA: 0x0062DC50.
    ParticleSystemClass(
        ParticleSystemTypeClass* pParticleSystemType,
        const CoordStruct& coords,
        AbstractClass* pTarget,
        ObjectClass* pOwner,
        const CoordStruct& targetCoords,
        HouseClass* pOwnerHouse) noexcept
#if defined(RA2_YRPP_GAME)
        : ParticleSystemClass(noinit_t())
            { JMP_THIS(0x62DC50); }
#else
        ;
#endif

protected:
    explicit __forceinline ParticleSystemClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    ParticleSystemTypeClass* Type;
    CoordStruct  SpawnDistanceToOwner;
    DECLARE_PROPERTY(DynamicVectorClass<ParticleClass*>, Particles);
    CoordStruct TargetCoords;
    ObjectClass* Owner;
    AbstractClass* Target; // CellClass or TechnoClass
    float        SpawnFrames; // 0x62DC50 converts the type's integer; instance offset 0xE8
    int          Lifetime; //from ParSysTypeClass
    int          SparkSpawnFrames; //from ParSysTypeClass
    int          SpotlightRadius; //defaults to 29
    bool         TimeToDie;
    bool         unknown_bool_F9;
    HouseClass*  OwnerHouse;
};
