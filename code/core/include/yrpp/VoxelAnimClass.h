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

class NOVTABLE VoxelAnimClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::VoxelAnim;

    // Static
    /// Global VA: 0x00887388.
    DEFINE_REFERENCE(DynamicVectorClass<VoxelAnimClass*>, Array, 0x887388u)

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~VoxelAnimClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int	Size() const R0;

    // ObjectClass
    // VoxelAnimClass

    // Constructor
    /// VA: 0x007493B0.
    VoxelAnimClass(
        VoxelAnimTypeClass* pVoxelAnimType, CoordStruct* pLocation,
        HouseClass* pOwnerHouse) : VoxelAnimClass(noinit_t())
    { JMP_THIS(0x7493B0); }

protected:
    explicit __forceinline VoxelAnimClass(noinit_t)
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    PROTECTED_PROPERTY(DWORD, unused_AC);
    DECLARE_PROPERTY(BounceClass, Bounce);
    int unknown_int_100;
    VoxelAnimTypeClass* Type;
    ParticleSystemClass* AttachedSystem;
    HouseClass* OwnerHouse;
    bool TimeToDie; // remove on next update
    PROTECTED_PROPERTY(BYTE, unused_111[3]);
    DECLARE_PROPERTY(AudioController, Audio3);
    DECLARE_PROPERTY(AudioController, Audio4);
    bool Invisible; // don't draw, but Update state anyway
    PROTECTED_PROPERTY(BYTE, unused_13D[3]);
    int Duration; // counting down to zero
    PROTECTED_PROPERTY(DWORD, unused_144);
};
