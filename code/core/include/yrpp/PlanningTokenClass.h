#pragma once

#include "yrpp/AbstractClass.h"
#include "yrpp/EventClass.h"

class TechnoClass;
class EventClass;

// Original-game wrappers use explicit thiscall function pointers. JMP_THIS
// assumes a saved ECX stack slot that optimized no-argument methods and
// destructors do not consistently emit on the supported x86 toolchain.
class PlanningBranchClass
{
public:
    /// VA: 0x00633AC0
#if defined(RA2_YRPP_GAME)
    PlanningBranchClass() noexcept : Packet(noinit_t{}) { reinterpret_cast<void (YRPP_THISCALL*)(PlanningBranchClass*)>(0x633AC0)(this); }
#else
    PlanningBranchClass() noexcept;
#endif
    EventClass Packet;
    int MemberCount;
    int field_74;
};
static_assert(sizeof(PlanningBranchClass) == 0x78);
static_assert(offsetof(PlanningBranchClass, MemberCount) == 0x70);

class PlanningMemberClass
{
public:
    TechnoClass* Owner;
    EventClass* Packet;
    int field_8;
    char field_C;
};
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(PlanningMemberClass) == 0x10);
#endif

class PlanningNodeClass
{
public:
    /// VA: 0x00633D30
#if defined(RA2_YRPP_GAME)
    ~PlanningNodeClass() noexcept { reinterpret_cast<void (YRPP_THISCALL*)(PlanningNodeClass*)>(0x633D30)(this); }
#else
    ~PlanningNodeClass() noexcept;
#endif
    /// VA: 0x00633EA0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) PlanningNodeClass* GetPrevious(TechnoClass* owner) const { return reinterpret_cast<PlanningNodeClass* (YRPP_THISCALL*)(const PlanningNodeClass*,TechnoClass*)>(0x633EA0)(this,owner); }
#else
    PlanningNodeClass* GetPrevious(TechnoClass* owner) const noexcept;
#endif
    /// VA: 0x00633F20
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) PlanningNodeClass* GetNext(TechnoClass* owner) const { return reinterpret_cast<PlanningNodeClass* (YRPP_THISCALL*)(const PlanningNodeClass*,TechnoClass*)>(0x633F20)(this,owner); }
#else
    PlanningNodeClass* GetNext(TechnoClass* owner) const noexcept;
#endif
    /// VA: 0x00633FA0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) int AddMember(TechnoClass* owner,const EventClass* event) { return reinterpret_cast<int (YRPP_THISCALL*)(PlanningNodeClass*,TechnoClass*,const EventClass*)>(0x633FA0)(this,owner,event); }
#else
    int AddMember(TechnoClass* owner,const EventClass* event) noexcept;
#endif
    /// VA: 0x006340B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void RemoveMember(TechnoClass* owner) { return reinterpret_cast<void (YRPP_THISCALL*)(PlanningNodeClass*,TechnoClass*)>(0x6340B0)(this,owner); }
#else
    void RemoveMember(TechnoClass* owner) noexcept;
#endif
    /// VA: 0x006349B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void UpdateLoopBranch() { return reinterpret_cast<void (YRPP_THISCALL*)(PlanningNodeClass*)>(0x6349B0)(this); }
#else
    void UpdateLoopBranch() noexcept;
#endif
    /// VA: 0x00634CC0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) int FindBranch(TechnoClass* owner) const { return reinterpret_cast<int (YRPP_THISCALL*)(const PlanningNodeClass*,TechnoClass*)>(0x634CC0)(this,owner); }
#else
    int FindBranch(TechnoClass* owner) const noexcept;
#endif
    /// VA: 0x00634E10
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) int AddBranch(PlanningMemberClass* member) { return reinterpret_cast<int (YRPP_THISCALL*)(PlanningNodeClass*,PlanningMemberClass*)>(0x634E10)(this,member); }
#else
    int AddBranch(PlanningMemberClass* member) noexcept;
#endif
    /// VA: 0x00635060
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void ReleaseBranch(PlanningMemberClass* member) { return reinterpret_cast<void (YRPP_THISCALL*)(PlanningNodeClass*,PlanningMemberClass*)>(0x635060)(this,member); }
#else
    void ReleaseBranch(PlanningMemberClass* member) noexcept;
#endif
    /// VA: 0x00635120
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void ClearBranches() { return reinterpret_cast<void (YRPP_THISCALL*)(PlanningNodeClass*)>(0x635120)(this); }
#else
    void ClearBranches() noexcept;
#endif
    /// VA: 0x006351E0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) int ReleaseLoopBranch(PlanningMemberClass* member) { return reinterpret_cast<int (YRPP_THISCALL*)(PlanningNodeClass*,PlanningMemberClass*)>(0x6351E0)(this,member); }
#else
    int ReleaseLoopBranch(PlanningMemberClass* member) noexcept;
#endif
    /// VA: 0x00635DB0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void InvalidateDisplay(bool branches) { return reinterpret_cast<void (YRPP_THISCALL*)(PlanningNodeClass*,bool)>(0x635DB0)(this,branches); }
#else
    void InvalidateDisplay(bool branches) noexcept;
#endif
    /// VA: 0x00634290
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) int FindMemberIndex(const TechnoClass* owner) const { return reinterpret_cast<int (YRPP_THISCALL*)(const PlanningNodeClass*,const TechnoClass*)>(0x634290)(this,owner); }
#else
    int FindMemberIndex(const TechnoClass* owner) const noexcept;
#endif
    /// VA: 0x006342D0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) TechnoClass* GetOwner(int index) const { return reinterpret_cast<TechnoClass* (YRPP_THISCALL*)(const PlanningNodeClass*,int)>(0x6342D0)(this,index); }
#else
    TechnoClass* GetOwner(int index) const noexcept;
#endif
    /// VA: 0x006343C0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) CoordStruct* GetCoords(CoordStruct* output) const { return reinterpret_cast<CoordStruct* (YRPP_THISCALL*)(const PlanningNodeClass*,CoordStruct*)>(0x6343C0)(this,output); }
#else
    CoordStruct* GetCoords(CoordStruct* output) const noexcept;
#endif
    /// VA: 0x00634550
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) BOOL IsAt(CoordStruct coords) const { return reinterpret_cast<BOOL (YRPP_THISCALL*)(const PlanningNodeClass*,CoordStruct)>(0x634550)(this,coords); }
#else
    BOOL IsAt(CoordStruct coords) const noexcept;
#endif
    /// VA: 0x006346B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) PlanningMemberClass* FindMember(const TechnoClass* owner) const { return reinterpret_cast<PlanningMemberClass* (YRPP_THISCALL*)(const PlanningNodeClass*,const TechnoClass*)>(0x6346B0)(this,owner); }
#else
    PlanningMemberClass* FindMember(const TechnoClass* owner) const noexcept;
#endif
    /// Global VA: 0x00AC4B30.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<PlanningNodeClass*>, Unknown1, 0xAC4B30u)
    /// Global VA: 0x00AC4C18.
    DEFINE_REFERENCE(DynamicVectorClass<PlanningNodeClass*>, Unknown2, 0xAC4C18u)
    /// Global VA: 0x00AC4C98.
    DEFINE_REFERENCE(DynamicVectorClass<PlanningNodeClass*>, Unknown3, 0xAC4C98u)
#else
    static DynamicVectorClass<PlanningNodeClass*>& Unknown1;
    /// Global VA: 0x00AC4C18.
    static DynamicVectorClass<PlanningNodeClass*>& Unknown2;
    /// Global VA: 0x00AC4C98.
    static DynamicVectorClass<PlanningNodeClass*>& Unknown3;
#endif

    /// Global VA: 0x00AC4CF4.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, PlanningModeActive, 0xAC4CF4u)
#else
    static bool& PlanningModeActive;
#endif

    // Properties

public:

    DynamicVectorClass<PlanningMemberClass*> PlanningMembers;
    int field_18;
    bool field_1C;
    DynamicVectorClass<PlanningBranchClass*> PlanningBranches;
    PlanningBranchClass CommonBranch;
    int field_B0;
    int field_B4;
};
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(PlanningNodeClass) == 0xB8);
static_assert(offsetof(PlanningNodeClass, CommonBranch) == 0x38);
static_assert(offsetof(PlanningNodeClass, field_B4) == 0xB4);
#endif

class PlanningTokenClass
{
public:
    /// VA: 0x00635F20
#if defined(RA2_YRPP_GAME)
    explicit PlanningTokenClass(TechnoClass* owner = nullptr) noexcept : CurrentEvent(noinit_t{}) { reinterpret_cast<void (YRPP_THISCALL*)(PlanningTokenClass*,TechnoClass*)>(0x635F20)(this,owner); }
#else
    explicit PlanningTokenClass(TechnoClass* owner = nullptr) noexcept;
#endif
    /// VA: 0x00635F80
#if defined(RA2_YRPP_GAME)
    ~PlanningTokenClass() noexcept { reinterpret_cast<void (YRPP_THISCALL*)(PlanningTokenClass*)>(0x635F80)(this); }
#else
    ~PlanningTokenClass() noexcept;
#endif
    /// VA: 0x00636120
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void ClearNodes() { reinterpret_cast<void (YRPP_THISCALL*)(PlanningTokenClass*)>(0x636120)(this); }
#else
    void ClearNodes() noexcept;
#endif
    /// VA: 0x00636310
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void Destroy() { reinterpret_cast<void (YRPP_THISCALL*)(PlanningTokenClass*)>(0x636310)(this); }
#else
    void Destroy() noexcept;
#endif
    /// VA: 0x00636570
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) BOOL HasCommittedNodes() const { return reinterpret_cast<BOOL (YRPP_THISCALL*)(const PlanningTokenClass*)>(0x636570)(this); }
#else
    BOOL HasCommittedNodes() const noexcept;
#endif
    /// VA: 0x00636CE0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void Commit() { return reinterpret_cast<void (YRPP_THISCALL*)(PlanningTokenClass*)>(0x636CE0)(this); }
#else
    void Commit() noexcept;
#endif
    /// VA: 0x00636E60
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) PlanningNodeClass* GetNode(int index) const { return reinterpret_cast<PlanningNodeClass* (YRPP_THISCALL*)(const PlanningTokenClass*,int)>(0x636E60)(this,index); }
#else
    PlanningNodeClass* GetNode(int index) const noexcept;
#endif
    /// VA: 0x00636EB0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) PlanningNodeClass* GetLastNode() const { return reinterpret_cast<PlanningNodeClass* (YRPP_THISCALL*)(const PlanningTokenClass*)>(0x636EB0)(this); }
#else
    PlanningNodeClass* GetLastNode() const noexcept;
#endif
    /// VA: 0x00636F00
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) EventClass* GetEvent(EventClass* output, int index) const { return reinterpret_cast<EventClass* (YRPP_THISCALL*)(const PlanningTokenClass*,EventClass*,int)>(0x636F00)(this,output,index); }
#else
    EventClass* GetEvent(EventClass* output, int index) const noexcept;
#endif
    /// Global VA: 0x00AC4C78.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<PlanningTokenClass*>, Array, 0xAC4C78u)
#else
    static DynamicVectorClass<PlanningTokenClass*>& Array;
#endif

    // Properties

public:
    TechnoClass* OwnerUnit;
    DynamicVectorClass<PlanningNodeClass*> PlanningNodes;
    bool field_1C;
    // TODO(RADAR-PLAN-EXEC): first-node consumption at 0x00636590 is deferred
    // by the user and has no native method yet. It snapshots this event before
    // member removal or loop rotation; GetEvent/Commit do not consume nodes.
    // 0x00636590 copies the full 0x6F-byte event here before advancing a node.
    EventClass CurrentEvent;

    int field_8C;
    int ClosedLoopNodeCount;
    int StepsToClosedLoop;
    bool field_98;
    bool field_99;
};
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(PlanningTokenClass) == 0x9C);
static_assert(offsetof(PlanningTokenClass, CurrentEvent) == 0x1D);
static_assert(offsetof(PlanningTokenClass, field_8C) == 0x8C);
#endif
