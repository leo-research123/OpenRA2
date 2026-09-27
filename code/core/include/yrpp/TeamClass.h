#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TeamTypeClass.h"
#include "yrpp/Timer.h"

class HouseClass;
class FootClass;
class CellClass;
class ScriptClass;
class TagClass;

class NOVTABLE TeamClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Team;

    // Static
    /// Global VA: 0x008B40E8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TeamClass*>, Array, 0x8B40E8u)
#else
    static DynamicVectorClass<TeamClass*>& Array;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x006E8DE0
    ~TeamClass() override;

    // Read-only leave-map permission; does not execute team orders.
    /// VA: 0x006F03B0
    int GetStrayDistance() const;
    /// VA: 0x006EC300
#if defined(RA2_YRPP_GAME)
    bool IsLeavingMapNow() const { JMP_THIS(0x6EC300); }
#else
    bool IsLeavingMapNow() const;
#endif

    // fills dest with all types needed to complete this team. each type is
    // included as often as it is needed.
    /// VA: 0x006EF4D0.
    void GetTaskForceMissingMemberTypes(DynamicVectorClass<TechnoTypeClass*>& dest) const
        JMP_THIS(0x6EF4D0);

    /// VA: 0x006EA870.
    void LiberateMember(FootClass* pFoot, int idx = -1, byte count = 0)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x6EA870); }
#else
        ;
#endif

    // the living, on-map member with the highest LeadershipRating
    /// VA: 0x006EB380
#if defined(RA2_YRPP_GAME)
    void MemberTookDamage(FootClass* member,DamageState state,TechnoClass* source) { JMP_THIS(0x6EB380); }
#else
    void MemberTookDamage(FootClass* member,DamageState state,TechnoClass* source);
#endif

    /// VA: 0x006EC3D0.
    FootClass* FetchALeader() const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x6EC3D0); }
#else
        ;
#endif

    // if bKeepQuantity is false, this will not change the quantity of each techno member
    /// VA: 0x006EA500.
    bool AddMember(FootClass* pFoot, bool bForce)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x6EA500); }
#else
        ;
#endif

    /// VA: 0x006E9050.
    void AssignMissionTarget(AbstractClass* pTarget)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x6E9050); }
#else
        ;
#endif

    /// VA: 0x006EC3A0.
    void ScanLimit()
        JMP_THIS(0x6EC3A0);

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    AbstractType WhatAmI() const override {return AbsID;}
    /// VA: unknown (legacy placeholder).
    int Size() const override {return sizeof(*this);}

    /// VA: 0x006EAE60
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object,bool removed) override { JMP_THIS(0x6EAE60); }
#else
    void PointerExpired(AbstractClass* object,bool removed) override;
#endif
    // Constructor
    /// VA: 0x006E8A90.
#if defined(RA2_YRPP_GAME)
    TeamClass(TeamTypeClass* pType, HouseClass* pOwner, int _unknown_44) noexcept
        : TeamClass(noinit_t())
    {
        JMP_THIS(0x6E8A90);
    }
#else
    TeamClass(TeamTypeClass* pType, HouseClass* pOwner, int _unknown_44) noexcept;
#endif
    /// VA: 0x006E9140
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x6E9140); }
#else
    void Update() override;
#endif

protected:
    explicit __forceinline TeamClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties
public:
    TeamTypeClass* Type;
    ScriptClass*   CurrentScript;
    HouseClass*    Owner;
    HouseClass*    Target;
    CellClass*     SpawnCell;
    FootClass*     ClosestMember;
    AbstractClass* QueuedFocus;
    AbstractClass* Focus;
    int            unknown_44;
    int            TotalObjects;
    int            TotalThreatValue;
    int            CreationFrame;
    FootClass*     FirstUnit;
    CDTimerClass   GuardAreaTimer;
    CDTimerClass   SuspendTimer;
    TagClass*      Tag;
    bool           IsTransient;
    bool           NeedsReGrouping;
    bool           GuardSlowerIsNotUnderStrength;
    bool           IsForcedActive;

    bool           IsHasBeen;
    bool           IsFullStrength;
    bool           IsUnderStrength;
    bool           IsReforming;

    bool           IsLagging;
    bool           NeedsToDisappear;
    bool           JustDisappeared;
    bool           IsMoving;

    bool           StepCompleted; // can proceed to the next step of the script
    bool           TargetNotAssigned;
    bool           IsLeavingMap;
    bool           IsSuspended;

    bool           AchievedGreatSuccess; // executed script action 49, 0

    int CountObjects[6]; // counts of each object specified in the Type
};
