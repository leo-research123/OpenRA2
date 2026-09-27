#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

// forward declarations
class SuperClass;
class TechnoClass;

// The AirstrikeClass handles the airstrikes Boris calls in.
class NOVTABLE TemporalClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Temporal;

    // Static
    /// Global VA: 0x00B0EC60.
    DEFINE_REFERENCE(DynamicVectorClass<TemporalClass*>, Array, 0xB0EC60u)

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TemporalClass() RX;

    // AbstractClass
    /// VA: 0x0071A760
    void Update() override { JMP_THIS(0x71A760); }
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // non-virtual
    /// VA: 0x0071AB60
#if defined(RA2_YRPP_GAME)
    void UnlinkPointer(AbstractClass* object) { JMP_THIS(0x71AB60); }
#else
    void UnlinkPointer(AbstractClass* object);
#endif
    /// VA: 0x0071AF20.
    void Fire(TechnoClass* pTarget)
        { JMP_THIS(0x71AF20); }
    /// VA: 0x0071AE50.
    bool CanWarpTarget(TechnoClass* pTarget) const
        { JMP_THIS(0x71AE50); }

    // hardcoded to accumulate only up to 50 helpers
    /// VA: 0x0071AB10.
    int GetWarpPerStep( int nHelperCount = 0 ) const
        { JMP_THIS(0x71AB10); }

    /// VA: 0x0071ABC0.
    void LetGo()
        { JMP_THIS(0x71ABC0); }
    /// VA: 0x0071AD40.
    void JustLetGo()
        { JMP_THIS(0x71AD40); }
    /// VA: 0x0071ADE0.
    void Detach()
        { JMP_THIS(0x71ADE0); }

    // Constructor
    /// VA: 0x0071A4E0.
    TemporalClass(TechnoClass* pOwnerUnit) noexcept
        : TemporalClass(noinit_t())
    { JMP_THIS(0x71A4E0); }

protected:
    explicit __forceinline TemporalClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    TechnoClass*       Owner;
    TechnoClass*       Target;
    CDTimerClass       LifeTimer;
    void*              unknown_pointer_38;
    SuperClass*        SourceSW;

    TemporalClass*     NextTemporal;
    TemporalClass*     PrevTemporal;

    int                WarpRemaining;
    int                WarpPerStep;
};
