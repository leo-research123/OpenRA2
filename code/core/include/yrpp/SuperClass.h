/*
    Supers
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/Timer.h"

// Forward declarations
class AnimClass;
class HouseClass;

class NOVTABLE SuperClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Super;

    // Static
    /// Global VA: 0x00A83CB8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<SuperClass*>, Array, 0xA83CB8u)

    /// Global VA: 0x00A83D50.
    DEFINE_REFERENCE(DynamicVectorClass<SuperClass*>, ShowTimers, 0xA83D50u)
#else
    static DynamicVectorClass<SuperClass*>& Array;
    static DynamicVectorClass<SuperClass*>& ShowTimers;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~SuperClass() RX;

    // non virtual
    /// VA: 0x006CB3A0.
    void CreateChronoAnim(CoordStruct coords)
        { JMP_THIS(0x6CB3A0); }

    /// VA: 0x006CE0B0.
    void Reset()
        { JMP_THIS(0x6CE0B0); }

    // was the setting successful? (did we have to change the state (true) or was it already in the same hold state(false))
    /// VA: 0x006CB4D0.
#if defined(RA2_YRPP_GAME)
    bool SetOnHold(bool onHold)
        { JMP_THIS(0x6CB4D0); }
#else
    bool SetOnHold(bool onHold);
#endif

    /// VA: 0x006CB560.
    bool Grant(bool oneTime, bool announce, bool onHold)
        { JMP_THIS(0x6CB560); }

    // true if this was ->Granted
    /// VA: 0x006CB7B0.
#if defined(RA2_YRPP_GAME)
    bool Lose()
        { JMP_THIS(0x6CB7B0); }
#else
    bool Lose();
#endif

    /// VA: 0x006CC2A0
    bool IsPowered() const
        { return this->Type->IsPowered; }

    /// VA: 0x006CC390.
    void Launch(const CellStruct& cell, bool isPlayer)
        { JMP_THIS(0x6CC390); }

    /// VA: 0x006CC360.
    char CanFire() const
        { JMP_THIS(0x6CC360); }

    /// VA: 0x006CB820.
    void SetReadiness(bool ready) // this->IsCharged = Ready
        { JMP_THIS(0x6CB820); }

    /// VA: 0x006CB830.
    char StopPreclickAnim(bool isPlayer) // if this is a PostClick SW, firing it does this
        { JMP_THIS(0x6CB830); }

    /// VA: 0x006CB920.
    char ClickFire(bool isPlayer, const CellStruct& cell) // calls Launch after printing Lightning Storm warning and other fluff
        { JMP_THIS(0x6CB920); }

    /// VA: 0x006CBCA0.
    bool HasChargeProgressed(bool isPlayer) // true if the charge has changed (charge overlay on the cameo)
        { JMP_THIS(0x6CBCA0); }               // triggers the EVA Announcement if it's ready

    /// VA: 0x006CBEE0.
    signed int AnimStage() const // which cameo charge overlay frame to show
        { JMP_THIS(0x6CBEE0); }

    /// VA: 0x006CC1E0.
    void SetCharge(int percentage)
        { JMP_THIS(0x6CC1E0); }

    /// VA: 0x006CC260.
    int GetRechargeTime() const // the time it takes this SW to recharge fully
        { JMP_THIS(0x6CC260); }

    /// VA: 0x006CC280.
    void SetRechargeTime(int time) // makes this SW rechange in this many frames, as opposed to [Type]RechargeTime
        { JMP_THIS(0x6CC280); }

    /// VA: 0x006CC290.
    void ResetRechargeTime() // nullifies the previous call
        { JMP_THIS(0x6CC290); }

    /// VA: 0x006CC2B0.
    const wchar_t* NameReadiness() const // the string to be displayed over the SW in the sidebar - "Ready" or ChargeDrain state
        { JMP_THIS(0x6CC2B0); }

    /// VA: 0x006CDE90.
    bool ShouldDrawProgress() const // sidebar
        { JMP_THIS(0x6CDE90); }

    /// VA: 0x006CE1A0.
    bool ShouldFlashTab() const // sidebar
        { JMP_THIS(0x6CE1A0); }

    // Constructor
    /// VA: 0x006CAF90.
    SuperClass(SuperWeaponTypeClass* pSWType, HouseClass* pOwner) noexcept
        : SuperClass(noinit_t())
    { JMP_THIS(0x6CAF90); }

protected:
    explicit __forceinline SuperClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    int CustomChargeTime;
    SuperWeaponTypeClass* Type;
    HouseClass* Owner;
    CDTimerClass RechargeTimer;
    PROTECTED_PROPERTY(DWORD, unused_3C);
    bool BlinkState;
    PROTECTED_PROPERTY(BYTE, unused_41[7]);
    LARGE_INTEGER BlinkTimer;
    int SpecialSoundDuration; // see 0x6CD14F
    CoordStruct SpecialSoundLocation;
    bool CanHold;          // 0x60
    PROTECTED_PROPERTY(BYTE, unused_61);
    CellStruct ChronoMapCoords;  // 0x62
    PROTECTED_PROPERTY(BYTE, unused_66[2]);
    AnimClass* Animation;                // 0x68
    bool AnimationGotInvalid;
    bool IsPresent;
    bool IsOneTime; // remove this SW when it has been fired once
    bool IsReady;
    bool IsSuspended;
    PROTECTED_PROPERTY(BYTE, unused_71[3]);
    int ReadyFrame; // when did it become ready?
    int CameoChargeState;
    ChargeDrainState ChargeDrainState;
};

class LightningStorm
{
public:
    /// Global VA: 0x00B0CDD8.
    DEFINE_REFERENCE(double, CloudHeightFactor, 0xB0CDD8u)
    /// Global VA: 0x00A9F9CC.
    DEFINE_REFERENCE(CellStruct, Coords, 0xA9F9CCu)
    /// Global VA: 0x00A9FACC.
    DEFINE_REFERENCE(HouseClass*, Owner, 0xA9FACCu)
    /// Global VA: 0x00A9FAB8.
    DEFINE_REFERENCE(int, Deferment, 0xA9FAB8u)
    /// Global VA: 0x00827FC4.
    DEFINE_REFERENCE(int, Duration, 0x827FC4u)
    /// Global VA: 0x00827FC0.
    DEFINE_REFERENCE(int, StartTime, 0x827FC0u)
    static bool& Active; // Global VA: 0x00A9FAB4, original or standalone storage.
    /// Global VA: 0x00A9FAD0.
    DEFINE_REFERENCE(bool, TimeToEnd, 0xA9FAD0u)

    /// Global VA: 0x00A9F9D0.
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, CloudsPresent, 0xA9F9D0u)
    /// Global VA: 0x00A9FA60.
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, CloudsManifesting, 0xA9FA60u)
    /// Global VA: 0x00A9FA18.
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, BoltsPresent, 0xA9FA18u)

    static void Start(int duration, int deferment, CellStruct cell, HouseClass* pOwner)
        { PUSH_VAR32(pOwner); PUSH_VAR32(cell); SET_REG32(EDX, deferment); SET_REG32(ECX, duration);
            CALL(0x539EB0); }

    static void RequestStop()
        { CALL(0x53A090); }

    static bool HasDeferment()
        { CALL(0x53A0E0); }

    static void Strike(CellStruct cell)
        { PUSH_VAR32(cell); CALL(0x53A140); }

    /// VA: 0x0053A300.
    static void Strike2(CoordStruct coords)
        { JMP_STD(0x53A300); }

    static void PrintMessage()
        { CALL(0x53AE00); }

    /// VA: 0x0053A6C0.
    static void Update()
        { JMP_STD(0x53A6C0); }

    /// VA: 0x0053AB50.
    static void Init()
        { JMP_STD(0x53AB50); }
};

class PsyDom
{
public:
    static PsychicDominatorStatus& Status;
    /// Global VA: 0x00A9FA48.
    DEFINE_REFERENCE(CellStruct, Coords, 0xA9FA48u)
    /// Global VA: 0x00A9FAC8.
    DEFINE_REFERENCE(HouseClass*, Owner, 0xA9FAC8u)
    /// Global VA: 0x00A9FAC4.
    DEFINE_REFERENCE(AnimClass*, Anim, 0xA9FAC4u)

    static void Start(HouseClass* pOwner, CellStruct coords)
        { PUSH_VAR32(coords); SET_REG32(ECX, pOwner);
            CALL(0x53AE50); }

    static void Update()
        { CALL(0x53AF40); }

    static void Fire()
        { CALL(0x53B080); }

    static void PrintMessage()
        { CALL(0x53B410); }

    static bool Active()
        { return Status != PsychicDominatorStatus::Inactive; }
};

class ChronoScreenEffect
{
public:
    static int& Status;
    /// Global VA: 0x00A9FA98.
    DEFINE_REFERENCE(int, Duration, 0xA9FA98u)

    static void Start(int duration)
        { SET_REG32(ECX, duration);
            CALL(0x53B460); }

    static void Update()
        { CALL(0x53B560); }

    static bool Active()
        { return Status != 0; }
};

class NukeFlash
{
public:
    static NukeFlashStatus& Status;
    /// Global VA: 0x00827FC8.
    DEFINE_REFERENCE(int, StartTime, 0x827FC8u)
    /// Global VA: 0x00827FCC.
    DEFINE_REFERENCE(int, Duration, 0x827FCCu)

    static void FadeIn()
        { CALL(0x53AB70); }

    static void FadeOut()
        { CALL(0x53AC50); }

    static bool IsFadingIn()
        { return Status == NukeFlashStatus::FadeIn; }

    static bool IsFadingOut()
        { return Status == NukeFlashStatus::FadeOut; }
};
