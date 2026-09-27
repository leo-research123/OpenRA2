/*
    VoxelAnimTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"

// forward declarations
class AnimTypeClass;
class ParticleSystemTypeClass;
class WarheadTypeClass;

class VoxelAnimTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x0074B810; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x0074B810); }
    /// VA: 0x0074B8D0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x0074B8D0); }
    /// VA: 0x0074B8F0; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x0074B690; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x0074B690); }
    /// VA: 0x0074B050; reference retained, not a local implementation.
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x0074B050); }

    static const AbstractType AbsID = AbstractType::VoxelAnimType;

    // Array
    static DynamicVectorClass<VoxelAnimTypeClass*>& Array;
    static VoxelAnimTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x0074B7D0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // AbstractClass
    /// VA: 0x0074B9F0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x0074BA00.
    virtual int	Size() const;

    // ObjectTypeClass
    /// VA: 0x0074BA10; local body in core/src/yrpp.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner);
    /// VA: 0x0074BA20; local body in core/src/yrpp.
    virtual ObjectClass* CreateObject(HouseClass* owner); // ! this just returns NULL instead of creating the anim, fucking slackers

    // VoxelAnimTypeClass

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~VoxelAnimTypeClass();

    // Constructor
    /// VA: 0x0074AD80.
    VoxelAnimTypeClass(const char* pID);

protected:
    explicit __forceinline VoxelAnimTypeClass(noinit_t)
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    bool Normalized;
    bool Translucent;
    bool SourceShared;
    PROTECTED_PROPERTY(BYTE, unused_297);
    int VoxelIndex;
    int Duration;
    double Elasticity;
    double MinAngularVelocity;
    double MaxAngularVelocity;
    double MinZVel;
    double MaxZVel;
    double MaxXYVel;
    bool IsMeteor;
    PROTECTED_PROPERTY(BYTE, unused_2D1[3]);
    VoxelAnimTypeClass* Spawns;
    int SpawnCount;
    int StartSound;
    int StopSound;
    AnimTypeClass* BounceAnim;
    AnimTypeClass* ExpireAnim;
    AnimTypeClass* TrailerAnim;
    int Damage;
    int DamageRadius;
    WarheadTypeClass* Warhead;
    ParticleSystemTypeClass* AttachedSystem;
    bool IsTiberium;
    PROTECTED_PROPERTY(BYTE, unused_301[3]);
};
