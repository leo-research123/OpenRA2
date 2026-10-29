/*
    DiskLasers are the floating disks' purple lasers.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

class LaserDrawClass;
class TechnoClass;
class WeaponTypeClass;

class NOVTABLE DiskLaserClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::DiskLaser;

    // static
    /// Global VA: 0x008A0208.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<DiskLaserClass*>, Array, 0x8A0208u)
#else
    static DynamicVectorClass<DiskLaserClass*>& Array;
#endif

    static constexpr auto Radius = 240;
    /// Global VA: 0x008A0180.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(Point2D, [16], DrawCoords, 0x8A0180u)
#else
    static Point2D (&DrawCoords)[16];
#endif

    // IPersist
    /// VA: 0x4A7C30
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) {
        return reinterpret_cast<HRESULT(YRPP_STDCALL*)(DiskLaserClass*, CLSID*)>(0x4A7C30)(this, pClassID);
    }
#else
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
#else
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { return static_cast<HRESULT>(0x80004001u); }
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;
#else
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { return static_cast<HRESULT>(0x80004001u); }
#endif

    // Destructor
    /// VA: 0x4A7C90
#if defined(RA2_YRPP_GAME)
    virtual ~DiskLaserClass() RX;
#else
    virtual ~DiskLaserClass();
#endif

    // AbstractClass
    /// VA: 0x4A7C80
    virtual AbstractType WhatAmI() const { return AbsID; }
    /// VA: 0x4A7C70
    virtual int Size() const { return sizeof(*this); }
    /// VA: 0x4A7340
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x4A7340); }
#else
    void Update() override;
#endif

    // non-virtual
    /// VA: 0x004A71A0.
#if defined(RA2_YRPP_GAME)
    void Fire(TechnoClass* pOwner, TechnoClass* pTarget, WeaponTypeClass* pWeapon, int nDamage) { JMP_THIS(0x4A71A0); }
#else
    void Fire(TechnoClass* pOwner, TechnoClass* pTarget, WeaponTypeClass* pWeapon, int nDamage);
#endif

    /// VA: 0x004A7900.
#if defined(RA2_YRPP_GAME)
    void PointerGotInvalid(AbstractClass* pInvalid) { JMP_THIS(0x4A7900); }
#else
    void PointerGotInvalid(AbstractClass* pInvalid);
#endif

    // Constructor
    /// VA: 0x004A7A30.
#if defined(RA2_YRPP_GAME)
    DiskLaserClass() noexcept : DiskLaserClass(noinit_t()) { JMP_THIS(0x4A7A30); }
#else
    DiskLaserClass() noexcept;
#endif

protected:
    explicit __forceinline DiskLaserClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

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
