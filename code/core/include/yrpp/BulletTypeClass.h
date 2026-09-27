/*
    Projectiles
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"

// forward declarations
class AnimTypeClass;
class BulletClass;
class ColorScheme;
class CellClass;
class TechnoClass;
class WeaponTypeClass;
class WarheadTypeClass;

class BulletTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x0046C6A0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x0046C6A0); }
    /// VA: 0x0046C730; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x0046C730); }
    /// VA: 0x0046C820; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x0046C560
    virtual void ComputeCRC(CRCEngine& crc) const override;
    /// VA: 0x0046BEE0
    virtual bool LoadFromINI(CCINIClass* pINI) override;

    static const AbstractType AbsID = AbstractType::BulletType;

    // Array
    static DynamicVectorClass<BulletTypeClass*>& Array;
    static BulletTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x0046C790.
    static BulletTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
    // IPersist
    /// VA: 0x0046C750.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~BulletTypeClass();

    // AbstractClass
    /// VA: 0x0046C850.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x0046C860.
    virtual int Size() const;

    // AbstractTypeClass
    // ObjectTypeClass
    /// VA: 0x0046C870; local body in core/src/yrpp.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords,HouseClass* pOwner);
    /// VA: 0x0046C880; local body in core/src/yrpp.
    virtual ObjectClass* CreateObject(HouseClass* owner);

    bool Rotates() const {
        return !this->NoRotate;
    }

    void SetScaledSpawnDelay(int delay) {
        // JMP_THIS(0x46C840);
        this->ScaledSpawnDelay = delay;
    }

    /// VA: 0x0046B050
    BulletClass* YRPP_FASTCALL CreateBullet(
        AbstractClass* Target,
        TechnoClass* Owner,
        int Damage,
        WarheadTypeClass *WH,
        int Speed,
        bool Bright)
#if defined(RA2_YRPP_GAME)
        { JMP_STD(0x46B050); }
#else
        ;
#endif

    // Constructor
    /// VA: 0x0046BBC0.
    BulletTypeClass(const char* pID);

protected:
    explicit __forceinline BulletTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    bool Airburst;
    bool Floater;
    bool SubjectToCliffs;
    bool SubjectToElevation;
    bool SubjectToWalls;
    bool VeryHigh;
    bool Shadow;
    bool Arcing;
    bool Dropping;
    bool Level;
    bool Inviso;
    bool Proximity;
    bool Ranged;
    bool NoRotate; // actually has opposite meaning of Rotates. false means Rotates=yes.
    bool Inaccurate;
    bool FlakScatter;
    bool AA;
    bool AG;
    bool Degenerates;
    bool Bouncy;
    bool AnimPalette;
    bool FirersPalette;
    int Cluster;
    WeaponTypeClass* AirburstWeapon;
    WeaponTypeClass* ShrapnelWeapon;
    int ShrapnelCount;
    int DetonationAltitude;
    bool Vertical;
    double Elasticity;
    int Acceleration;
    int Color; // ColorScheme array index, read by 0x00474A90.
    AnimTypeClass* Trailer;
    int ROT;
    int CourseLockDuration;
    int SpawnDelay;
    int ScaledSpawnDelay;
    bool Scalable;
    int Arm;
    byte AnimLow;
    byte AnimHigh;
    byte AnimRate;
    bool Flat;
};
