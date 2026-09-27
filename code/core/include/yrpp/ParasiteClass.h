#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

class AnimClass;
class FootClass;

class NOVTABLE ParasiteClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Parasite;

    /// Global VA: 0x00AC4910.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<ParasiteClass*>, Array, 0xAC4910u)
#else
    static DynamicVectorClass<ParasiteClass*>& Array;
#endif
    /// Global VA: 0x00B0F5B8
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<ParasiteClass*>, AnimExpirationListeners, 0xB0F5B8u)
#else
    static DynamicVectorClass<ParasiteClass*>& AnimExpirationListeners;
#endif

    // IPersist
    /// VA: 0x006296D0
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x006295B0
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x006293E0
    ~ParasiteClass() override;

    // AbstractClass
    /// VA: 0x00629FD0
    void Update() override;
    // Victim expiration restores the limbo-launched owner to the map.
    /// VA: 0x0062A260
    void PointerExpired(AbstractClass* object, bool removed) override;
    /// VA: 0x0062AF60
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x0062AF50
    int Size() const override { return sizeof(*this); }

    // non-virtual
    /// VA: 0x006297F0.
    void UpdateSquid()
        { JMP_THIS(0x6297F0); }

    /// VA: 0x00629720.
    bool UpdateGrapple()
        { JMP_THIS(0x629720); }

    /// VA: 0x0062A4A0.
    void ExitUnit();

    /// VA: 0x0062A8E0.
    bool CanInfect(FootClass *pTarget) const;

    /// VA: 0x0062A980.
    void TryInfect(FootClass *pTarget);

    /// VA: 0x0062AB40.
    bool CanExistOnVictimCell() const;
    /// VA: 0x0062AC30
    CoordStruct* GetExitCoords(CoordStruct* output);

    // Constructor
    /// VA: 0x006292B0.
    ParasiteClass(FootClass* pOwner = nullptr) noexcept;

protected:
    explicit __forceinline ParasiteClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    FootClass*      Owner;
    FootClass*      Victim;
    CDTimerClass    SuppressionTimer;
    CDTimerClass    DamageDeliveryTimer;
    AnimClass*      GrappleAnim;
    ParasiteState   GrappleState;
    int             GrappleAnimFrame;
    int             GrappleAnimDelay;
    bool            GrappleAnimGotInvalid;
};
