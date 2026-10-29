#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TeamTypeClass.h"
#include "yrpp/Timer.h"

class HouseClass;
class FootClass;
class CellClass;
class ScriptClass;
class TagClass;
class BuildingClass;
class BuildingTypeClass;
struct ScriptActionNode;

class NOVTABLE TeamClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Team;

    // Static
    /// Global VA: 0x008B40E8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TeamClass*>, Array, 0x8B40E8u)
#else
    static DynamicVectorClass<TeamClass*>& Array;
#endif

    /// VA: 0x6EC250
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL SuspendTeams(int priority, HouseClass* owner) { JMP_STD(0x6EC250); }
#else
    static void YRPP_FASTCALL SuspendTeams(int priority, HouseClass* owner);
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
#if defined(RA2_YRPP_GAME)
    void GetTaskForceMissingMemberTypes(DynamicVectorClass<TechnoTypeClass*>& dest) const JMP_THIS(0x6EF4D0);
#else
    void GetTaskForceMissingMemberTypes(DynamicVectorClass<TechnoTypeClass*>& dest) const;
#endif

    /// VA: 0x006EA870.
    void LiberateMember(FootClass* pFoot, int idx = -1, byte count = 0)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6EA870);
    }
#else
        ;
#endif

    // the living, on-map member with the highest LeadershipRating
    /// VA: 0x006EB380
#if defined(RA2_YRPP_GAME)
    void MemberTookDamage(FootClass* member, DamageState state, TechnoClass* source) { JMP_THIS(0x6EB380); }
#else
    void MemberTookDamage(FootClass* member, DamageState state, TechnoClass* source);
#endif

    /// VA: 0x006EC3D0.
    FootClass* FetchALeader() const
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6EC3D0);
    }
#else
        ;
#endif

    // if bKeepQuantity is false, this will not change the quantity of each techno member
    /// VA: 0x006EA500.
    bool AddMember(FootClass* pFoot, bool bForce)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6EA500);
    }
#else
        ;
#endif

    /// VA: 0x6EA610
#if defined(RA2_YRPP_GAME)
    bool CanAddMember(FootClass* foot, int* entry, bool force) const { JMP_THIS(0x6EA610); }
#else
    bool CanAddMember(FootClass* foot, int* entry, bool force) const;
#endif

    /// VA: 0x006E9050.
    void AssignMissionTarget(AbstractClass* pTarget)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E9050);
    }
#else
        ;
#endif

    /// VA: 0x6EB490
#if defined(RA2_YRPP_GAME)
    void CoordinateAttack() { JMP_THIS(0x6EB490); }
#else
    void CoordinateAttack();
#endif
    /// VA: 0x6EC9A0
#if defined(RA2_YRPP_GAME)
    void AttackWaypoint(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EC9A0); }
#else
    void AttackWaypoint(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6ECE60
#if defined(RA2_YRPP_GAME)
    void Spy(const ScriptActionNode& action, bool first) { JMP_THIS(0x6ECE60); }
#else
    void Spy(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6ECCE0
#if defined(RA2_YRPP_GAME)
    void Patrol(const ScriptActionNode& action, bool first) { JMP_THIS(0x6ECCE0); }
#else
    void Patrol(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6ED7E0
#if defined(RA2_YRPP_GAME)
    void DoMission(const ScriptActionNode& action, bool first) { JMP_THIS(0x6ED7E0); }
#else
    void DoMission(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EDCA0
#if defined(RA2_YRPP_GAME)
    void ForceFacing(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EDCA0); }
#else
    void ForceFacing(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EE310
#if defined(RA2_YRPP_GAME)
    void AttackBuilding(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EE310); }
#else
    void AttackBuilding(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EE3F0
#if defined(RA2_YRPP_GAME)
    void MoveToBuilding(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EE3F0); }
#else
    void MoveToBuilding(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EE5C0
#if defined(RA2_YRPP_GAME)
    void MoveToFriendlyBuilding(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EE5C0); }
#else
    void MoveToFriendlyBuilding(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6ECA70
#if defined(RA2_YRPP_GAME)
    void GarrisonWaypoint(const ScriptActionNode& action, bool first) { JMP_THIS(0x6ECA70); }
#else
    void GarrisonWaypoint(const ScriptActionNode& action, bool first);
#endif

    /// VA: 0x6EEBD0
#if defined(RA2_YRPP_GAME)
    static BuildingClass* YRPP_FASTCALL PickBuilding(BuildingTypeClass* type, HouseClass* enemy, FootClass* origin,
                                                     int property, bool onlyEnemy) {
        JMP_STD(0x6EEBD0);
    }
#else
    static BuildingClass* YRPP_FASTCALL PickBuilding(BuildingTypeClass* type, HouseClass* enemy, FootClass* origin,
                                                     int property, bool onlyEnemy);
#endif
    /// VA: 0x6EEEA0
#if defined(RA2_YRPP_GAME)
    static BuildingClass* YRPP_FASTCALL PickFriendlyBuilding(BuildingTypeClass* type, void* reserved, FootClass* origin,
                                                             int property, int reserved2) {
        JMP_STD(0x6EEEA0);
    }
#else
    static BuildingClass* YRPP_FASTCALL PickFriendlyBuilding(BuildingTypeClass* type, void* reserved, FootClass* origin,
                                                             int property, int reserved2);
#endif
    /// VA: 0x6EF610
#if defined(RA2_YRPP_GAME)
    void PlayAnimation(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EF610); }
#else
    void PlayAnimation(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EE800
#if defined(RA2_YRPP_GAME)
    void Scout(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EE800); }
#else
    void Scout(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EF700
#if defined(RA2_YRPP_GAME)
    void GatherAtEnemy(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EF700); }
#else
    void GatherAtEnemy(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EFA10
#if defined(RA2_YRPP_GAME)
    void GatherAtBase(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EFA10); }
#else
    void GatherAtBase(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EFC70
#if defined(RA2_YRPP_GAME)
    void AskForSW(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EFC70); }
#else
    void AskForSW(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EFE60
#if defined(RA2_YRPP_GAME)
    void ChronoshiftToBuilding(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EFE60); }
#else
    void ChronoshiftToBuilding(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6F0130
#if defined(RA2_YRPP_GAME)
    void ChronoshiftToTarget(const ScriptActionNode& action, bool first) { JMP_THIS(0x6F0130); }
#else
    void ChronoshiftToTarget(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EC7D0
#if defined(RA2_YRPP_GAME)
    void MoveToWaypoint(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EC7D0); }
#else
    void MoveToWaypoint(const ScriptActionNode& action, bool first);
#endif
    /// VA: 0x6EBAD0
#if defined(RA2_YRPP_GAME)
    void CoordinateMove() { JMP_THIS(0x6EBAD0); }
#else
    void CoordinateMove();
#endif

    /// VA: 0x6ED200
#if defined(RA2_YRPP_GAME)
    void TMission_LOAD(const ScriptActionNode& action, bool first) { JMP_THIS(0x6ED200); }
#else
    void TMission_LOAD(const ScriptActionNode& action, bool first);
#endif

    /// VA: 0x6EF110
#if defined(RA2_YRPP_GAME)
    void TransportUnload(const ScriptActionNode& action, bool first) { JMP_THIS(0x6EF110); }
#else
    void TransportUnload(const ScriptActionNode& action, bool first);
#endif

    /// VA: 0x6ED4D0
#if defined(RA2_YRPP_GAME)
    void Unload(const ScriptActionNode& action, bool first) { JMP_THIS(0x6ED4D0); }
#else
    void Unload(const ScriptActionNode& action, bool first);
#endif

    /// VA: 0x6EC3A0
    void ScanLimit()
#if defined(RA2_YRPP_GAME)
        JMP_THIS(0x6EC3A0);
#else
        ;
#endif

    // AbstractClass
    /// VA: 0x006EAA90
#if defined(RA2_YRPP_GAME)
    int Recruit(int entry) { JMP_THIS(0x6EAA90); }
#else
    int Recruit(int entry);
#endif
    /// VA: 0x006EA3E0
#if defined(RA2_YRPP_GAME)
    bool RecalculateStrength() { JMP_THIS(0x6EA3E0); }
#else
    bool RecalculateStrength();
#endif
    /// VA: 0x6EA0D0
#if defined(RA2_YRPP_GAME)
    void Retreat() { JMP_THIS(0x6EA0D0); }
#else
    void Retreat();
#endif

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: unknown (legacy placeholder).
    int Size() const override { return sizeof(*this); }

    /// VA: 0x006EAE60
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x6EAE60); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    // Constructor
    /// VA: 0x006E8A90.
#if defined(RA2_YRPP_GAME)
    TeamClass(TeamTypeClass* pType, HouseClass* pOwner, int _unknown_44) noexcept : TeamClass(noinit_t()) {
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
    explicit __forceinline TeamClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties
public:
    TeamTypeClass* Type;
    ScriptClass* CurrentScript;
    HouseClass* Owner;
    HouseClass* Target;
    CellClass* SpawnCell;
    FootClass* ClosestMember;
    AbstractClass* QueuedFocus;
    AbstractClass* Focus;
    int unknown_44;
    int TotalObjects;
    int TotalThreatValue;
    int CreationFrame;
    FootClass* FirstUnit;
    CDTimerClass GuardAreaTimer;
    CDTimerClass SuspendTimer;
    TagClass* Tag;
    bool IsTransient;
    bool NeedsReGrouping;
    bool GuardSlowerIsNotUnderStrength;
    bool IsForcedActive;

    bool IsHasBeen;
    bool IsFullStrength;
    bool IsUnderStrength;
    bool IsReforming;

    bool IsLagging;
    bool NeedsToDisappear;
    bool JustDisappeared;
    bool IsMoving;

    bool StepCompleted;  // can proceed to the next step of the script
    bool TargetNotAssigned;
    bool IsLeavingMap;
    bool IsSuspended;

    bool AchievedGreatSuccess;  // executed script action 49, 0

    int CountObjects[6];  // counts of each object specified in the Type
};
