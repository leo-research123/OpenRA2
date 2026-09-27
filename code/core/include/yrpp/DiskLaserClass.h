/*
    DiskLasers are the floating disks' purple lasers.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

class LaserDrawClass;
class TechnoClass;
class WeaponTypeClass;

class NOVTABLE DiskLaserClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::DiskLaser;

    // static
    /// Global VA: 0x008A0208.
    DEFINE_REFERENCE(DynamicVectorClass<DiskLaserClass*>, Array, 0x8A0208u)

    static constexpr auto Radius = 240;
    /// Global VA: 0x008A0180.
    DEFINE_ARRAY_REFERENCE(Point2D, [16], DrawCoords, 0x8A0180u)

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~DiskLaserClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // non-virtual
    /// VA: 0x004A71A0.
    void Fire(TechnoClass* pOwner, TechnoClass* pTarget, WeaponTypeClass* pWeapon, int nDamage)
        { JMP_THIS(0x4A71A0); }

    /// VA: 0x004A7900.
    void PointerGotInvalid(AbstractClass* pInvalid)
        { JMP_THIS(0x4A7900); }

    // Constructor
    /// VA: 0x004A7A30.
    DiskLaserClass() noexcept
        : DiskLaserClass(noinit_t())
    { JMP_THIS(0x4A7A30); }

protected:
    explicit __forceinline DiskLaserClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    TechnoClass* Owner;
    TechnoClass* Target;
    WeaponTypeClass* Weapon;
    DWORD unknown_30;
    DWORD unknown_34;
    DWORD unknown_38;
    int Damage;
};
