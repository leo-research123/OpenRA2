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

class NOVTABLE SuperClass : public AbstractClass {
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
    /// VA: 0x006CE200
    virtual AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x006CE210
    virtual int Size() const override { return sizeof(*this); }
    /// VA: 0x006CDFF0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x6CDFF0); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif

    // Destructor
    /// VA: 0x006CB120
#if defined(RA2_YRPP_GAME)
    virtual ~SuperClass() { JMP_THIS(0x6CB120); }
#else
    virtual ~SuperClass();
#endif

    // non virtual
    /// VA: 0x006CB3A0
#if defined(RA2_YRPP_GAME)
    void CreateChronoAnim(CoordStruct coords) { JMP_THIS(0x6CB3A0); }
#else
    void CreateChronoAnim(CoordStruct coords);
#endif

    /// VA: 0x006CE0B0.
#if defined(RA2_YRPP_GAME)
    void Reset() { JMP_THIS(0x6CE0B0); }
#else
    void Reset();
#endif

    // was the setting successful? (did we have to change the state (true) or was it already in the same hold state(false))
    /// VA: 0x006CB4D0.
#if defined(RA2_YRPP_GAME)
    bool SetOnHold(bool onHold) { JMP_THIS(0x6CB4D0); }
#else
    bool SetOnHold(bool onHold);
#endif

    /// VA: 0x006CB560.
#if defined(RA2_YRPP_GAME)
    bool Grant(bool oneTime, bool announce, bool onHold) { JMP_THIS(0x6CB560); }
#else
    bool Grant(bool oneTime, bool announce, bool onHold);
#endif

    // true if this was ->Granted
    /// VA: 0x006CB7B0.
#if defined(RA2_YRPP_GAME)
    bool Lose() { JMP_THIS(0x6CB7B0); }
#else
    bool Lose();
#endif

    /// VA: 0x006CC2A0
    bool IsPowered() const { return this->Type->IsPowered; }

    /// VA: 0x6CC390
#if defined(RA2_YRPP_GAME)
    void Launch(const CellStruct& cell, bool isPlayer) { JMP_THIS(0x6CC390); }
#else
    void Launch(const CellStruct& cell, bool isPlayer);
#endif

    /// VA: 0x006CC360.
#if defined(RA2_YRPP_GAME)
    char CanFire() const { JMP_THIS(0x6CC360); }
#else
    char CanFire() const;
#endif

    /// VA: 0x006CB820.
#if defined(RA2_YRPP_GAME)
    void SetReadiness(bool ready)  // this->IsCharged = Ready
    {
        JMP_THIS(0x6CB820);
    }
#else
    void SetReadiness(bool ready);
#endif

    /// VA: 0x006CB830.
#if defined(RA2_YRPP_GAME)
    char StopPreclickAnim(bool isPlayer)  // if this is a PostClick SW, firing it does this
    {
        JMP_THIS(0x6CB830);
    }
#else
    char StopPreclickAnim(bool isPlayer);
#endif

    /// VA: 0x006CB920.
#if defined(RA2_YRPP_GAME)
    char ClickFire(bool isPlayer,
                   const CellStruct& cell)  // calls Launch after printing Lightning Storm warning and other fluff
    {
        JMP_THIS(0x6CB920);
    }
#else
    char ClickFire(bool isPlayer, const CellStruct& cell);
#endif

    /// VA: 0x006CBCA0.
#if defined(RA2_YRPP_GAME)
    bool HasChargeProgressed(bool isPlayer) { JMP_THIS(0x6CBCA0); }
#else
    bool HasChargeProgressed(bool isPlayer);
#endif

    /// VA: 0x006CBEE0.
#if defined(RA2_YRPP_GAME)
    signed int AnimStage() const { JMP_THIS(0x6CBEE0); }
#else
    signed int AnimStage() const;
#endif

    /// VA: 0x006CC1E0.
#if defined(RA2_YRPP_GAME)
    void SetCharge(int percentage) { JMP_THIS(0x6CC1E0); }
#else
    void SetCharge(int percentage);
#endif

    /// VA: 0x006CC260.
#if defined(RA2_YRPP_GAME)
    int GetRechargeTime() const { JMP_THIS(0x6CC260); }
#else
    int GetRechargeTime() const;
#endif

    /// VA: 0x006CC280.
#if defined(RA2_YRPP_GAME)
    void SetRechargeTime(int time)  // makes this SW rechange in this many frames, as opposed to [Type]RechargeTime
    {
        JMP_THIS(0x6CC280);
    }
#else
    void SetRechargeTime(int time);
#endif

    /// VA: 0x006CC290.
    void ResetRechargeTime()  // nullifies the previous call
    {
        JMP_THIS(0x6CC290);
    }

    /// VA: 0x006CC2B0.
#if defined(RA2_YRPP_GAME)
    const wchar_t* NameReadiness()
        const  // the string to be displayed over the SW in the sidebar - "Ready" or ChargeDrain state
    {
        JMP_THIS(0x6CC2B0);
    }
#else
    const wchar_t* NameReadiness() const;
#endif

    /// VA: 0x006CDE90.
#if defined(RA2_YRPP_GAME)
    bool ShouldDrawProgress() const  // sidebar
    {
        JMP_THIS(0x6CDE90);
    }
#else
    bool ShouldDrawProgress() const;
#endif

    /// VA: 0x006CE1A0.
#if defined(RA2_YRPP_GAME)
    bool ShouldFlashTab() const  // sidebar
    {
        JMP_THIS(0x6CE1A0);
    }
#else
    bool ShouldFlashTab() const;
#endif

    // Constructor
    /// VA: 0x006CAF90.
#if defined(RA2_YRPP_GAME)
    SuperClass(SuperWeaponTypeClass* pSWType, HouseClass* pOwner) noexcept : SuperClass(noinit_t()) {
        JMP_THIS(0x6CAF90);
    }
#else
    SuperClass(SuperWeaponTypeClass* pSWType, HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline SuperClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

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
    int SpecialSoundDuration;  // see 0x6CD14F
    CoordStruct SpecialSoundLocation;
    bool CanHold;  // 0x60
    PROTECTED_PROPERTY(BYTE, unused_61);
    CellStruct ChronoMapCoords;  // 0x62
    PROTECTED_PROPERTY(BYTE, unused_66[2]);
    AnimClass* Animation;  // 0x68
    bool AnimationGotInvalid;
    bool IsPresent;
    bool IsOneTime;  // remove this SW when it has been fired once
    bool IsReady;
    bool IsSuspended;
    PROTECTED_PROPERTY(BYTE, unused_71[3]);
    int ReadyFrame;  // when did it become ready?
    int CameoChargeState;
    ChargeDrainState ChargeDrainState;
};

class LightningStorm {
public:
    /// Global VA: 0x00B0CDD8.
    DEFINE_REFERENCE(double, CloudHeightFactor, 0xB0CDD8u)
    /// Global VA: 0x00A9F9CC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(CellStruct, Coords, 0xA9F9CCu)
#else
    static CellStruct& Coords;
#endif
    /// Global VA: 0x00A9FACC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(HouseClass*, Owner, 0xA9FACCu)
#else
    static HouseClass*& Owner;
#endif
    /// Global VA: 0x00A9FAB8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, Deferment, 0xA9FAB8u)
#else
    static int& Deferment;
#endif
    /// Global VA: 0x00827FC4.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, Duration, 0x827FC4u)
#else
    static int& Duration;
#endif
    /// Global VA: 0x00827FC0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, StartTime, 0x827FC0u)
#else
    static int& StartTime;
#endif
    static bool& Active;  // Global VA: 0x00A9FAB4, original or standalone storage.
    /// Global VA: 0x00A9FAD0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, TimeToEnd, 0xA9FAD0u)
#else
    static bool& TimeToEnd;
#endif

    /// Global VA: 0x00A9F9D0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, CloudsPresent, 0xA9F9D0u)
#else
    static DynamicVectorClass<AnimClass*>& CloudsPresent;
#endif
    /// Global VA: 0x00A9FA60.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, CloudsManifesting, 0xA9FA60u)
#else
    static DynamicVectorClass<AnimClass*>& CloudsManifesting;
#endif
    /// Global VA: 0x00A9FA18.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AnimClass*>, BoltsPresent, 0xA9FA18u)
#else
    static DynamicVectorClass<AnimClass*>& BoltsPresent;
#endif

    /// VA: 0x539EB0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL Start(int duration, int deferment, CellStruct cell, HouseClass* owner) {
        JMP_STD(0x539EB0);
    }
#else
    static void YRPP_FASTCALL Start(int duration, int deferment, CellStruct cell, HouseClass* owner);
#endif
    /// VA: 0x53A090
#if defined(RA2_YRPP_GAME)
    static void RequestStop() { JMP_STD(0x53A090); }
#else
    static void RequestStop();
#endif
    /// VA: 0x53A100
#if defined(RA2_YRPP_GAME)
    static bool IsActive() { JMP_STD(0x53A100); }
#else
    static bool IsActive() { return Active; }
#endif
    /// VA: 0x53A0E0
#if defined(RA2_YRPP_GAME)
    static bool HasDeferment() { JMP_STD(0x53A0E0); }
#else
    static bool HasDeferment() { return Active || Deferment > 0; }
#endif

    /// VA: 0x0053A140
#if defined(RA2_YRPP_GAME)
    static void Strike(CellStruct cell) {
        PUSH_VAR32(cell);
        CALL(0x53A140);
    }
#else
    static void Strike(CellStruct cell);
#endif

    /// VA: 0x0053A300.
#if defined(RA2_YRPP_GAME)
    static void Strike2(CoordStruct coords) { JMP_STD(0x53A300); }
#else
    static void Strike2(CoordStruct coords);
#endif

    /// VA: 0x53AE00
#if defined(RA2_YRPP_GAME)
    static void PrintMessage() { CALL(0x53AE00); }
#else
    static void PrintMessage();
#endif

    /// VA: 0x0053A6C0.
#if defined(RA2_YRPP_GAME)
    static void Update() { JMP_STD(0x53A6C0); }
#else
    static void Update();
#endif

    /// VA: 0x0053AB50.
#if defined(RA2_YRPP_GAME)
    static void Init() { JMP_STD(0x53AB50); }
#else
    static void Init();
#endif
};

class PsyDom {
public:
    static PsychicDominatorStatus& Status;
    /// Global VA: 0x00A9FA48.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(CellStruct, Coords, 0xA9FA48u)
    DEFINE_REFERENCE(HouseClass*, Owner, 0xA9FAC8u)
    DEFINE_REFERENCE(AnimClass*, Anim, 0xA9FAC4u)
    /// VA: 0x0053AE50
    static void Start(HouseClass* owner, CellStruct coords) {
        PUSH_VAR32(coords);
        SET_REG32(ECX, owner);
        CALL(0x53AE50);
    }
    /// VA: 0x0053AF40
    static void Update() { CALL(0x53AF40); }
    /// VA: 0x0053B080
    static void Fire() { CALL(0x53B080); }
#else
    static CellStruct& Coords;
    static HouseClass*& Owner;
    static AnimClass*& Anim;
    /// VA: 0x0053AE50
    static void Start(HouseClass* owner, CellStruct coords);
    /// VA: 0x0053AF40
    static void Update();
    /// VA: 0x0053B080
    static void Fire();
#endif

    /// VA: 0x53B410
#if defined(RA2_YRPP_GAME)
    static void PrintMessage() { CALL(0x53B410); }
#else
    static void PrintMessage();
#endif

    static bool Active() { return Status != PsychicDominatorStatus::Inactive; }
};

class ChronoScreenEffect {
public:
    static int& Status;
    /// Global VA: 0x00A9FA98.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, Duration, 0xA9FA98u)
    /// VA: 0x53B460
    static void YRPP_FASTCALL Start(int duration) { JMP_STD(0x53B460); }
    /// VA: 0x53B560
    static void Update() { CALL(0x53B560); }
#else
    static int& Duration;
    /// VA: 0x53B460
    static void YRPP_FASTCALL Start(int duration) noexcept;
    /// VA: 0x53B560
    static void Update() noexcept;
#endif

    static bool Active() { return Status != 0; }
};

class NukeFlash {
public:
    static NukeFlashStatus& Status;
    /// Global VA: 0x00827FC8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, StartTime, 0x827FC8u)
    DEFINE_REFERENCE(int, Duration, 0x827FCCu)
    /// VA: 0x0053AB70
    static void FadeIn() { CALL(0x53AB70); }
    /// VA: 0x0053AC50
    static void FadeOut() { CALL(0x53AC50); }
#else
    static int& StartTime;
    static int& Duration;
    /// VA: 0x0053AB70
    static void FadeIn();
    /// VA: 0x0053AC50
    static void FadeOut();
#endif

    static bool IsFadingIn() { return Status == NukeFlashStatus::FadeIn; }

    static bool IsFadingOut() { return Status == NukeFlashStatus::FadeOut; }
};
