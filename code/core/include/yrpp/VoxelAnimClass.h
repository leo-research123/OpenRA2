/*
    Voxel Animations
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/BounceClass.h"

// forward declarations
class HouseClass;
class ParticleSystemClass;

class NOVTABLE VoxelAnimClass : public ObjectClass {
public:
    static const AbstractType AbsID = AbstractType::VoxelAnim;

    // Static
    /// Global VA: 0x00887388.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<VoxelAnimClass*>, Array, 0x887388u)
#else
    static DynamicVectorClass<VoxelAnimClass*>& Array;
#endif

    // IPersist
    /// VA: 0x0074AAD0
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x0074AA10
    HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x0074AA10);
    }
#else
    {
        return static_cast<HRESULT>(0x80004001u);
    }
#endif

    // Destructor
    /// VA: 0x007499F0
    ~VoxelAnimClass() override;

    // AbstractClass
    /// VA: 0x0074AB20
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x0074AB10
    int Size() const override { return sizeof(*this); }
    /// VA: 0x0074AB30
    ObjectTypeClass* GetType() const override { return Type; }
    /// VA: 0x0074A960
    Layer InWhichLayer() const override { return Layer::Air; }
    /// VA: 0x00749F30
    void Update() override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x00749F30);
    }
#else
        ;
#endif
    /// VA: 0x00749B70
    void DrawIt(Point2D* point, RectangleStruct* bounds) const override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x00749B70);
    }
#else
        ;
#endif
    // The original expiration slot inherits ObjectClass::PointerExpired.

    // ObjectClass
    // VoxelAnimClass

    // Constructor
    /// VA: 0x007493B0.
    VoxelAnimClass(VoxelAnimTypeClass* pVoxelAnimType, CoordStruct* pLocation, HouseClass* pOwnerHouse) noexcept
#if defined(RA2_YRPP_GAME)
        : VoxelAnimClass(noinit_t()) {
        JMP_THIS(0x7493B0);
    }
#else
        ;
#endif

protected:
    explicit __forceinline VoxelAnimClass(noinit_t) : ObjectClass(noinit_t()) {}

    // Properties

public:
    PROTECTED_PROPERTY(DWORD, unused_AC);
    DECLARE_PROPERTY(BounceClass, Bounce);
    int unknown_int_100;
    VoxelAnimTypeClass* Type;
    ParticleSystemClass* AttachedSystem;
    HouseClass* OwnerHouse;
    bool TimeToDie;  // remove on next update
    PROTECTED_PROPERTY(BYTE, unused_111[3]);
    DECLARE_PROPERTY(AudioController, Audio3);
    DECLARE_PROPERTY(AudioController, Audio4);
    bool Invisible;  // don't draw, but Update state anyway
    PROTECTED_PROPERTY(BYTE, unused_13D[3]);
    int Duration;  // counting down to zero
    PROTECTED_PROPERTY(DWORD, unused_144);
};
