/*
    AnimTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TechnoTypeClass.h"

struct SubSequenceStruct
{
    int StartFrame;
    int CountFrames;
    int FacingMultiplier;
    SequenceFacing Facing;
    int SoundCount;
    int Sound1StartFrame;
    int Sound1Index; // VocClass
    int Sound2StartFrame;
    int Sound2Index; // VocClass
};

struct SequenceStruct
{
    SubSequenceStruct& GetSequence(Sequence sequence) {
        return this->Sequences[static_cast<int>(sequence)];
    }

    const SubSequenceStruct& GetSequence(Sequence sequence) const {
        return this->Sequences[static_cast<int>(sequence)];
    }

    SubSequenceStruct Sequences[42];
};

class InfantryTypeClass : public TechnoTypeClass
{
public:
    /// VA: 0x00524840; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00524840); }
    /// VA: 0x00524D60; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x005240A0
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x00523D00
    void ReadSequence();

    static const AbstractType AbsID = AbstractType::InfantryType;

    // Array
    static DynamicVectorClass<InfantryTypeClass*>& Array;
    static InfantryTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x00524CB0.
    static InfantryTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);

    // IPersist
    /// VA: 0x00524C70.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00524960; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x00524960); }
    /// VA: 0x00524B60; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x00524B60); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~InfantryTypeClass();

    // AbstractClass
    /// VA: 0x00524D40.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00524D50.
    virtual int	Size() const;

    // ObjectTypeClass
    /// VA: 0x00523B40; reference retained, not a local implementation.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner) { JMP_THIS(0x00523B40); }
    /// VA: 0x00523B10
#if defined(RA2_YRPP_GAME)
    virtual ObjectClass* CreateObject(HouseClass* pOwner) { JMP_THIS(0x00523B10); }
#else
    virtual ObjectClass* CreateObject(HouseClass* pOwner) override;
#endif

    // Constructor
    /// VA: 0x005236A0.
    InfantryTypeClass(const char* pID);

protected:
    explicit __forceinline InfantryTypeClass(noinit_t) noexcept
        : TechnoTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    PipIndex Pip;
    PipIndex OccupyPip;
    WeaponStruct OccupyWeapon;
    WeaponStruct EliteOccupyWeapon;
    SequenceStruct* Sequence;
    int FireUp;
    int FireProne;
    int SecondaryFire;
    int SecondaryProne;
    TypeList<AnimTypeClass*> DeadBodies;
    TypeList<AnimTypeClass*> DeathAnims;
    TypeList<int> VoiceComment;
    int EnterWaterSound;
    int LeaveWaterSound;
    bool Cyborg;
    bool NotHuman;
    bool Ivan; //used for the bomb attack cursor...
    int DirectionDistance;
    bool Occupier;
    bool Assaulter;
    int HarvestRate;
    bool Fearless;
    bool Crawls;
    bool Infiltrate;
    bool Fraidycat;
    bool TiberiumProof;
    bool Civilian;
    bool C4;
    bool Engineer;
    bool Agent;
    bool Thief;
    bool VehicleThief;
    bool Doggie;
    bool Deployer;
    bool DeployedCrushable;
    bool UseOwnName;
    bool JumpJetTurn;
private: DWORD align_ECC;
};
