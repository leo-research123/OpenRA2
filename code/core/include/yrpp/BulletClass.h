/*
    Projectiles
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/Timer.h"

class TechnoClass;
class ObjectClass;
class WarheadTypeClass;

struct BulletData
{
    CDTimerClass UnknownTimer;
    CDTimerClass ArmTimer;
    CoordStruct Location;
    int Distance;
};

// the velocities along the axes, or something like that
using BulletVelocity = Vector3D<double>; // :3 -pd

class NOVTABLE BulletClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::Bullet;

    // Array
    /// Global VA: 0x00A8ED40.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<BulletClass*>, Array, 0xA8ED40u)
    /// Global VA: 0x0089DE18.
    DEFINE_REFERENCE(DynamicVectorClass<BulletClass*>, ScalableBullets, 0x89DE18u)
    /// Global VA: 0x00B0F5B8.
    DEFINE_REFERENCE(DynamicVectorClass<BulletClass*>, NextAnimBullets, 0xB0F5B8u)
#else
    static DynamicVectorClass<BulletClass*>& Array;
    static DynamicVectorClass<BulletClass*>& ScalableBullets;
    static DynamicVectorClass<BulletClass*>& NextAnimBullets;
#endif

    // Allocates an unconfigured original object with its initial COM
    // reference, using the same allocator as Release. No exceptions escape.
#if !defined(RA2_YRPP_GAME)
    static BulletClass* Create() noexcept;
#endif
    /// VA: 0x0046AFD0
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL AddRef() override { JMP_STD(0x46AFD0); }
#else
    ULONG YRPP_STDCALL AddRef() override;
#endif
    /// VA: 0x0046AFF0
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL Release() override { JMP_STD(0x46AFF0); }
#else
    ULONG YRPP_STDCALL Release() override;
#endif

    // IPersist
    /// VA: 0x0046B560
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override { JMP_STD(0x46B560); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;
#endif

    // IPersistStream
    /// VA: 0x0046AE70
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x46AE70); }
    /// VA: 0x0046AFB0
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override { JMP_STD(0x46AFB0); }

    // Destructor
    /// VA: 0x00466560
#if defined(RA2_YRPP_GAME)
    ~BulletClass() override { JMP_THIS(0x466560); }
#else
    ~BulletClass() override;
#endif

    // AbstractClass
    /// VA: 0x0046B550
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x0046B540
    int Size() const override { return sizeof(*this); }
    /// VA: 0x004684E0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object,bool removed) override { JMP_THIS(0x4684E0); }
#else
    void PointerExpired(AbstractClass* object,bool removed) override;
#endif
    /// VA: 0x004666E0
    void Update() override;
    /// VA: 0x004666C0
    bool Mark(MarkType mark) override { return ObjectClass::Mark(mark); }

    /// VA: 0x0046B5B0
    ObjectTypeClass* GetType() const override { return Type; }
    /// VA: 0x00468B90
    Layer InWhichLayer() const override { return Type->Flat ? Layer::Surface : Layer::Air; }
    /// VA: 0x00468090
    void DrawIt(Point2D* location,RectangleStruct* bounds) const override;
    /// VA: 0x004264C0
    Move IsCellOccupied(CellClass*, FacingType, int, CellClass*, bool) const override { return Move::OK; }

    // BulletClass
    /// VA: 0x00468000
#if defined(RA2_YRPP_GAME)
    virtual BYTE GetAnimFrame() const { JMP_THIS(0x468000); }
#else
    virtual BYTE GetAnimFrame() const;
#endif
    /// VA: 0x0046B5A0
    virtual void SetTarget(AbstractClass* target) { Target=target; }
    /// VA: 0x00468670
#if defined(RA2_YRPP_GAME)
    virtual bool MoveTo(const CoordStruct& where, const BulletVelocity& velocity) { JMP_THIS(0x468670); }
#else
    virtual bool MoveTo(const CoordStruct& where, const BulletVelocity& velocity);
#endif

    // non-virtual
    // after CoCreateInstance creates a bullet, this configures it
    /// VA: 0x004664C0
    void Construct(
        BulletTypeClass* pType,
        AbstractClass* pTarget,
        TechnoClass* pOwner,
        int damage,
        WarheadTypeClass* pWarhead,
        int speed,
        bool bright)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x4664C0); }
#else
        ;
#endif

    // calls Detonate with the appropriate coords
    /// VA: 0x00468BB0
    bool IsForcedToExplode(CoordStruct* position) const;

    /// VA: 0x00468D80
    void Explode(bool forced = false)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x468D80); }
#else
        ;
#endif

    // detonate the bullet at specific coords
    /// VA: 0x004690B0
    void Detonate(const CoordStruct& coords);

    // spawns off the proper amount of shrapnel projectiles
    /// VA: 0x0046A310.
    void Shrapnel()
        { JMP_THIS(0x46A310); }

    /// VA: 0x0046ADE0.
    static void ApplyRadiationToCell(CellStruct cell, int radius, int amount)
        { JMP_STD(0x46ADE0); }

    // this bullet will miss and hit the ground instead.
    // if the original target is in air, it will disappear.
    /// VA: 0x00468430.
    void LoseTarget()
        { JMP_THIS(0x468430); }

    bool IsHoming() const
        { return this->Type->ROT > 0; }

    void SetWeaponType(WeaponTypeClass *weapon)
        { this->WeaponType = weapon; }

    WeaponTypeClass * GetWeaponType() const
        { return this->WeaponType; }

    // only called in UnitClass::Fire if Type->Scalable
    /// VA: 0x0046B280.
    void InitScalable()
        { JMP_THIS(0x46B280); }

    // call only after the target, args, etc., have been set
    /// VA: 0x0046B310.
    void NukeMaker()
        { JMP_THIS(0x46B310); }

    // helpers
    CoordStruct GetTargetCoords() const {
        if(this->Target) {
            return this->Target->GetCoords();
        } else {
            return this->GetCoords();
        }
    }

    // Constructor
protected:
    /// VA: 0x00466380
#if defined(RA2_YRPP_GAME)
    BulletClass() noexcept : BulletClass(noinit_t()) { JMP_THIS(0x466380); }
#else
    BulletClass() noexcept;
#endif

protected:
    explicit __forceinline BulletClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    BulletTypeClass* Type;
    TechnoClass* Owner;
    bool unknown_B4;
    BulletData Data;
    bool Bright;
    DWORD unknown_E4;
    BulletVelocity Velocity;
    DWORD unknown_100;
    bool unknown_104;
    bool CourseLock;
    int CourseLockCounter;
    AbstractClass* Target;
    int Speed;
    int InheritedColor;
    DWORD unknown_118;
    DWORD unknown_11C;
    double unknown_120;
    WarheadTypeClass* WH;
    byte AnimFrame;
    byte AnimRateCounter;
    WeaponTypeClass* WeaponType;
    CoordStruct SourceCoords;
    CoordStruct TargetCoords;
    CellStruct LastMapCoords;
    int DamageMultiplier;
    AnimClass* NextAnim;
    bool SpawnNextAnim;
    int Range;
};
