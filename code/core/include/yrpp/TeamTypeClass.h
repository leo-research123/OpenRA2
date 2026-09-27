/*
    [TeamTypes]
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"

// forward declarations
class FootClass;
class TagTypeClass;
class TeamClass;
class TechnoTypeClass;

class TeamTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x006F1030; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x006F1C80; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x006F1C80); }
    /// VA: 0x006F20C0; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x006F1090
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x006F1550; reference retained, not a local implementation.
    virtual bool SaveToINI(CCINIClass* pINI) override { JMP_THIS(0x006F1550); }

    static const AbstractType AbsID = AbstractType::TeamType;

    // Array
    static DynamicVectorClass<TeamTypeClass*>& Array;
    static TeamTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x006F0FC0
    static TeamTypeClass* YRPP_FASTCALL FindByNameOrID(const char* name);
    /// VA: 0x006F1920
    static TeamTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id) noexcept;

    // IPersist
    /// VA: 0x006F1C40.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x006F1BB0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x006F1BB0); }
    /// VA: 0x006F1B90; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x006F1B90); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TeamTypeClass();

    // AbstractClass
    /// VA: 0x006F20A0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x006F20B0.
    virtual int Size() const;

    /// VA: 0x006F19B0
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI, int scope) noexcept;

    /// VA: 0x006F09C0.
    TeamClass * CreateTeam(HouseClass *pHouse)
        { JMP_THIS(0x6F09C0); }

    /// VA: 0x006F0A70.
    void DestroyAllInstances()
        { JMP_THIS(0x6F0A70); }

    /// VA: 0x006F1870.
    int GetGroup() const
        { JMP_THIS(0x6F1870); }

    /// VA: 0x006F18A0.
    CellStruct* GetWaypoint(CellStruct *buffer) const
        { JMP_THIS(0x6F18A0); }

    /// VA: 0x006F18E0.
    CellStruct* GetTransportWaypoint(CellStruct *buffer) const
        { JMP_THIS(0x6F18E0); }

    /// VA: 0x006F1320.
    bool CanRecruitUnit(FootClass* pUnit, HouseClass* pOwner) const
        { JMP_THIS(0x6F1320); }

    /// VA: 0x006F1F30.
    void FlashAllInstances(int Duration)
        { JMP_THIS(0x6F1F30); }

    /// VA: 0x006F1F70.
    TeamClass * FindFirstInstance() const
        { JMP_THIS(0x6F1F70); }

    /// VA: 0x006F1FA0.
    void ProcessTaskForce();

    /// VA: 0x006F2040.
    static void ProcessAllTaskforces();

    /// VA: 0x006F2070.
    HouseClass* GetHouse() const
        { JMP_THIS(0x6F2070); }

    // Constructor
    /// VA: 0x006F06E0.
    TeamTypeClass(const char* pID);

protected:
    explicit __forceinline TeamTypeClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    int      ArrayIndex;
    int      Group;
    int      VeteranLevel;
    bool     Loadable;
    bool     Full;
    bool     Annoyance;
    bool     GuardSlower;
    bool     Recruiter;
    bool     Autocreate;
    bool     Prebuild;
    bool     Reinforce;
    bool     Whiner;
    bool     Aggressive;
    bool     LooseRecruit;
    bool     Suicide;
    bool     Droppod;
    bool     UseTransportOrigin;
    bool     DropshipLoadout;
    bool     OnTransOnly;
    int      Priority;
    int      Max;
    int      field_BC;
    int      MindControlDecision;
    HouseClass *     Owner;
    int      idxHouse; // idx for MP
    int      TechLevel;
    TagTypeClass* Tag;
    int      Waypoint;
    int      TransportWaypoint;
    int      cntInstances;
    ScriptTypeClass*  ScriptType;
    TaskForceClass*   TaskForce;
    int      IsGlobal;
    int      field_EC;
    bool     field_F0;
    bool     field_F1;
    bool     AvoidThreats;
    bool     IonImmune;
    bool     TransportsReturnOnUnload;
    bool     AreTeamMembersRecruitable;
    bool     IsBaseDefense;
    bool     OnlyTargetHouseEnemy;

};
