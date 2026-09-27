/*
    Infantry
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FootClass.h"
#include "yrpp/InfantryTypeClass.h"

class NOVTABLE InfantryClass : public FootClass
{
public:
    /// VA: 0x0051F2C0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x0051F2C0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    // Native map-reader status overload; no exceptions escape. Rejected records
    // are counted; a failure may retain earlier placements. Signature differs from the original entry.
    /// VA: 0x51FB00
    static bool ReadINI(CCINIClass& ini,unsigned int& rejectedRecords,int firstHouse = 0) noexcept;
    /// VA: 0x0051BAB0
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x51BAB0); }
#else
    void Update() override;
#endif
    /// VA: 0x0051B350
    void Tunnel_AI() { JMP_THIS(0x51B350); }
    /// VA: 0x005202F0
#if defined(RA2_YRPP_GAME)
    bool Theft_AI() { JMP_THIS(0x5202F0); }
#else
    bool Theft_AI();
#endif
    /// VA: 0x005200B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void Fear_AI() { JMP_THIS(0x5200B0); }
#else
    void Fear_AI();
#endif
    /// VA: 0x005206B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void Firing_AI() { JMP_THIS(0x5206B0); }
#else
    void Firing_AI();
#endif
    /// VA: 0x0051DF60
#if defined(RA2_YRPP_GAME)
    BulletClass* Fire(AbstractClass* target,int index) override { JMP_THIS(0x51DF60); }
#else
    BulletClass* Fire(AbstractClass* target,int index) override;
#endif
    // Locomotor creation/link portion of the original constructor (0x517A50).
    // The native loader calls this before map placement. Unsupported CLSIDs
    // report failure; no fallback driver is substituted. No exceptions escape.
    bool InitializeLocomotor() noexcept;
    static const AbstractType AbsID = AbstractType::Infantry;

    // Static
    /// Global VA: 0x00A83DE8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<InfantryClass*>, Array, 0xA83DE8u)
#else
    static DynamicVectorClass<InfantryClass*>& Array;
#endif

    // IPersist
    /// VA: 0x00523300
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~InfantryClass() RX;
#else
    virtual ~InfantryClass();
#endif

    // AbstractClass
    /// VA: 0x0051AA10
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x51AA10); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: 0x00523340
    virtual AbstractType WhatAmI() const override;
    /// VA: 0x005232F0
    virtual int Size() const override;
#if !defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetType() const override;
#endif
    /// VA: 0x00518D80
    int GetCurrentFrame() const;
    /// VA: 0x00518F90
    void DrawIt(Point2D* point, RectangleStruct* bounds) const override;

    // ObjectClass
    /// VA: 0x00522640
#if defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetDisguise(bool againstAllies) const override { JMP_THIS(0x522640); }
#else
    ObjectTypeClass* GetDisguise(bool againstAllies) const override;
#endif
    /// VA: 0x005226C0
#if defined(RA2_YRPP_GAME)
    HouseClass* GetDisguiseHouse(bool againstAllies) const override { JMP_THIS(0x5226C0); }
#else
    HouseClass* GetDisguiseHouse(bool againstAllies) const override;
#endif
    /// VA: 0x0051BF90
#if defined(RA2_YRPP_GAME)
    Move IsCellOccupied(CellClass* destination, FacingType facing, int level,
        CellClass* source, bool alt) const override { JMP_THIS(0x51BF90); }
#else
    Move IsCellOccupied(CellClass* destination, FacingType facing, int level,
        CellClass* source, bool alt) const override;
#endif
    /// VA: 0x005227F0
#if defined(RA2_YRPP_GAME)
    bool IsDisguisedAs(HouseClass* house) const override { JMP_THIS(0x5227F0); }
#else
    bool IsDisguisedAs(HouseClass* house) const override;
#endif

    /// VA: 0x005217C0
#if defined(RA2_YRPP_GAME)
    void MarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x5217C0); }
#else
    void MarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    /// VA: 0x00521850
#if defined(RA2_YRPP_GAME)
    void UnmarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x521850); }
#else
    void UnmarkAllOccupationBits(const CoordStruct& coords) override;
#endif

    /// VA: 0x0051E3B0.
    virtual Action MouseOverObject(ObjectClass const* pObject, bool ignoreForce = false) const override;
    /// VA: 0x0051E140
    AbstractClass* GreatestThreat(ThreatType threat,CoordStruct* coords,bool onlyTargetHouseEnemy) override;
    /// VA: 0x0051F800
#if defined(RA2_YRPP_GAME)
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override { JMP_THIS(0x51F800); }
#else
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override;
#endif
    /// VA: 0x0051F250
#if defined(RA2_YRPP_GAME)
    bool CellClickedAction(Action action,CellStruct* cell,CellStruct* follow,bool ignoreForce) override { JMP_THIS(0x51F250); }
#else
    bool CellClickedAction(Action action,CellStruct* cell,CellStruct* follow,bool ignoreForce) override;
#endif
    /// VA: 0x0051F190
#if defined(RA2_YRPP_GAME)
    bool ObjectClickedAction(Action action,ObjectClass* target,bool ignoreForce) override { JMP_THIS(0x51F190); }
#else
    bool ObjectClickedAction(Action action,ObjectClass* target,bool ignoreForce) override;
#endif

    // TechnoClass
    /// VA: 0x005224D0
    bool IsEngineer() const override { return Type->Engineer; }
    /// VA: 0x005216D0
#if defined(RA2_YRPP_GAME)
    bool IsItTimeForIdleActionYet() const override { JMP_THIS(0x5216D0); }
#else
    bool IsItTimeForIdleActionYet() const override;
#endif
    /// VA: 0x0051CDB0
#if defined(RA2_YRPP_GAME)
    bool UpdateIdleAction() override { JMP_THIS(0x51CDB0); }
#else
    bool UpdateIdleAction() override;
#endif
    /// VA: 0x00521C60
#if defined(RA2_YRPP_GAME)
    void PlayIdleAnim(int which) override { JMP_THIS(0x521C60); }
#else
    void PlayIdleAnim(int which) override;
#endif
    /// VA: 0x0051B1F0
#if defined(RA2_YRPP_GAME)
    void SetTarget(AbstractClass* target) override { JMP_THIS(0x51B1F0); }
#else
    void SetTarget(AbstractClass* target) override;
#endif
    /// VA: 0x00522CB0
#if defined(RA2_YRPP_GAME)
    bool IsPlayingDeathSequence() const { JMP_THIS(0x522CB0); }
#else
    bool IsPlayingDeathSequence() const;
#endif
    /// VA: 0x0051C8B0.
#if defined(RA2_YRPP_GAME)
    FireError GetFireError(AbstractClass* target, int weapon, bool checkRange) const override { JMP_THIS(0x51C8B0); }
#else
    FireError GetFireError(AbstractClass* target, int weapon, bool checkRange) const override;
#endif
    /// VA: 0x005218E0
#if defined(RA2_YRPP_GAME)
    int SelectWeapon(AbstractClass* target) const override { JMP_THIS(0x5218E0); }
#else
    int SelectWeapon(AbstractClass* target) const override;
#endif

    // FootClass movement-animation callbacks.
    /// VA: 0x00519630
#if defined(RA2_YRPP_GAME)
    void UpdatePosition(PCPType how) override { JMP_THIS(0x519630); }
#else
    void UpdatePosition(PCPType how) override;
#endif
    /// VA: 0x0051DF10
    bool Limbo() override;
    /// VA: 0x00521760
#if defined(RA2_YRPP_GAME)
    bool SpawnParachuted(const CoordStruct& at) override { JMP_THIS(0x521760); }
#else
    bool SpawnParachuted(const CoordStruct& at) override;
#endif
    /// VA: 0x0051DFF0
    bool Unlimbo(const CoordStruct& where,DirType facing) override;
    /// VA: 0x0051D0D0
    void Scatter(const CoordStruct& at,bool ignoreMission,bool ignoreDestination) override;
    /// VA: 0x00522910
    void Garrison(BuildingClass* building);
    /// VA: 0x00517FA0
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) override;
    /// VA: 0x0051CBA0
#if defined(RA2_YRPP_GAME)
    bool EnterIdleMode(bool initial,bool resume) override { JMP_THIS(0x51CBA0); }
#else
    bool EnterIdleMode(bool initial,bool resume) override;
#endif
    /// VA: 0x00521B60
#if defined(RA2_YRPP_GAME)
    bool ReadyToNextMission() const override { JMP_THIS(0x521B60); }
#else
    bool ReadyToNextMission() const override;
#endif
    /// VA: 0x0051F3E0
#if defined(RA2_YRPP_GAME)
    int Mission_Attack() override { JMP_THIS(0x51F3E0); }
#else
    int Mission_Attack() override;
#endif
    /// VA: 0x00522340
#if defined(RA2_YRPP_GAME)
    AbstractClass* ApproachTarget(DWORD queryOnly) override { JMP_THIS(0x522340); }
#else
    AbstractClass* ApproachTarget(DWORD queryOnly) override;
#endif
    /// VA: 0x0051F660
#if defined(RA2_YRPP_GAME)
    int Mission_Move() override { JMP_THIS(0x51F660); }
#else
    int Mission_Move() override;
#endif
    /// VA: 0x00520AE0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void Doing_AI() { JMP_THIS(0x520AE0); }
#else
    void Doing_AI();
#endif
    /// VA: 0x00520F40
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void Movement_AI() { JMP_THIS(0x520F40); }
#else
    void Movement_AI();
#endif
    /// VA: 0x0051AA40
    void SetDestination(AbstractClass* destination,bool immediate) override;
    /// VA: 0x0051DAF0
    bool StopMoving() override;
    /// VA: 0x00521DD0
    void vt_entry_4F4() override;
    /// VA: 0x00521EB0
    bool vt_entry_4F8() override;
    /// VA: 0x005221D0
    bool ShouldJumpJetFly(const CellStruct& from,const CellStruct& to) { JMP_THIS(0x5221D0); }
    /// VA: 0x00521D80
    int GetCurrentSpeed() const override;
    /// VA: 0x00521B20
    void vt_entry_548() override {
        if (SequenceAnim == Sequence::Walk || SequenceAnim == Sequence::Crawl || SequenceAnim == Sequence::Swim)
            SequenceAnim = Sequence::Nothing;
    }
    /// VA: 0x00521B40
    void vt_entry_54C() override {
        if (ShouldDeploy) { ShouldDeploy = false; PlayAnim(Sequence::Deploy); }
    }

    // InfantryClass
    /// VA: 0x0051F6E0
#if defined(RA2_YRPP_GAME)
    int Mission_Unload() override { JMP_THIS(0x51F6E0); }
#else
    int Mission_Unload() override;
#endif
    /// VA: 0x0051F620
#if defined(RA2_YRPP_GAME)
    int Mission_Guard() override { JMP_THIS(0x51F620); }
#else
    int Mission_Guard() override;
#endif
    /// VA: 0x0051F640
#if defined(RA2_YRPP_GAME)
    int Mission_AreaGuard() override { JMP_THIS(0x51F640); }
#else
    int Mission_AreaGuard() override;
#endif
    /// VA: 0x00521320
#if defined(RA2_YRPP_GAME)
    int Guard_Deploy_AI() { JMP_THIS(0x521320); }
#else
    int Guard_Deploy_AI();
#endif
    /// VA: 0x0051F330
#if defined(RA2_YRPP_GAME)
    void vt_entry_428() override { JMP_THIS(0x51F330); }
#else
    void vt_entry_428() override;
#endif
    /// VA: 0x005228D0
    virtual bool IsDeployed() const;
    /// VA: 0x700D50
    bool CanDeploySlashUnload() const override;
    /// VA: 0x0051D6F0
#if defined(RA2_YRPP_GAME)
    virtual bool PlayAnim(Sequence index, bool force = false, bool randomStartFrame = false) { JMP_THIS(0x51D6F0); }
#else
    virtual bool PlayAnim(Sequence index, bool force = false, bool randomStartFrame = false);
#endif

    // Constructor
    /// VA: 0x00517A50
#if defined(RA2_YRPP_GAME)
    InfantryClass(InfantryTypeClass* pType, HouseClass* pOwner) noexcept
        : InfantryClass(noinit_t())
    { JMP_THIS(0x517A50); }
#else
    InfantryClass(InfantryTypeClass* pType, HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline InfantryClass(noinit_t) noexcept
        : FootClass(noinit_t())
    { }

    // Properties

public:

    InfantryTypeClass* Type;
    Sequence SequenceAnim; //which is currently playing
    CDTimerClass unknown_Timer_6C8;
    DWORD          PanicDurationLeft; // set in ReceiveDamage on panicky units
    bool           PermanentBerzerk; // set by script action, not cleared anywhere
    bool           Technician;
    bool           unknown_bool_6DA;
    bool           Crawling;
    bool           unknown_bool_6DC;
    bool           unknown_bool_6DD;
    DWORD          unknown_6E0;
    bool           ShouldDeploy;
    int            unknown_int_6E8;
    PROTECTED_PROPERTY(DWORD, unused_6EC); //??
};
