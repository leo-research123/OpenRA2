/*
    SuperWeaponTypes!! =D
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FileSystem.h"
#include "yrpp/AbstractTypeClass.h"

// forward declarations
class BuildingTypeClass;
class ObjectClass;
class WeaponTypeClass;

class SuperWeaponTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x006CE910; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x006CE910); }
    /// VA: 0x006CEA10; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x006CEA20; reference retained, not a local implementation.
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x006CEA20); }

    static const AbstractType AbsID = AbstractType::SuperWeaponType;

    // Array
    static DynamicVectorClass<SuperWeaponTypeClass*>& Array;
    static SuperWeaponTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x006CE7C0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x006CE800; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x006CE800); }
    /// VA: 0x006CE8D0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x006CE8D0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~SuperWeaponTypeClass();

    // AbstractClass
    /// VA: 0x006CE8F0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x006CE900.
    virtual int Size() const;

    // SuperWeaponTypeClass
    /// VA: 0x006CEF80; world/house dependency remains original.
    virtual Action MouseOverObject(CellStruct const& cell, ObjectClass* pObjBelowMouse) const { JMP_THIS(0x006CEF80); }

    // non-virtual
    /// VA: 0x006CEEB0.
#if defined(RA2_YRPP_GAME)
    static SuperWeaponTypeClass * YRPP_FASTCALL FindFirstOfAction(Action Action)
        { JMP_STD(0x6CEEB0); }
#else
    static SuperWeaponTypeClass* YRPP_FASTCALL FindFirstOfAction(::Action action) noexcept;
#endif

    // Constructor
    /// VA: 0x006CE5B0.
    SuperWeaponTypeClass(const char* pID);

protected:
    explicit __forceinline SuperWeaponTypeClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    int     ArrayIndex;
    WeaponTypeClass* WeaponType;

    // I believe these four are the leftover TS sounds
    int     RechargeVoice; // not read, unused
    int     ChargingVoice; // not read, unused
    int     ImpatientVoice; // not read, unused
    int     SuspendVoice; // not read, unused
    //---

    int     RechargeTime; //in frames
    SuperWeaponType Type;
    SHPStruct* SidebarImage;
    Action Action;
    int     SpecialSound;
    int     StartSound;
    BuildingTypeClass* AuxBuilding;
    char SidebarImageFile [0x18];
    PROTECTED_PROPERTY(BYTE, zero_E4);
    bool    UseChargeDrain;
    bool    IsPowered;
    bool    DisableableFromShell;
    int     FlashSidebarTabFrames;
    bool    AIDefendAgainst;
    bool    PreClick;
    bool    PostClick;
    int		PreDependent;
    bool    ShowTimer;
    bool    ManualControl;
    float   Range;
    int     LineMultiplier;

};
