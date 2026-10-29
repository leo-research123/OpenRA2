#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

// forward declarations
class SuperClass;
class TechnoClass;

// The original temporal weapon controller and cooperating attacker chain.
class NOVTABLE TemporalClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Temporal;

    // Static
    /// Global VA: 0x00B0EC60.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TemporalClass*>, Array, 0xB0EC60u)
#else
    static DynamicVectorClass<TemporalClass*>& Array;
#endif

    // IPersist
    /// VA: 0x71A720
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) {
        return reinterpret_cast<HRESULT(YRPP_STDCALL*)(TemporalClass*, CLSID*)>(0x71A720)(this, pClassID);
    }
#else
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
#else
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { return static_cast<HRESULT>(0x80004001u); }
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;
#else
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { return static_cast<HRESULT>(0x80004001u); }
#endif

    // Destructor
    /// VA: 0x71B1B0
#if defined(RA2_YRPP_GAME)
    virtual ~TemporalClass() RX;
#else
    virtual ~TemporalClass();
#endif

    // AbstractClass
    /// VA: 0x0071A760
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x71A760); }
#else
    void Update() override;
#endif
    /// VA: 0x71B1A0
    virtual AbstractType WhatAmI() const { return AbsID; }
    /// VA: 0x71B190
    virtual int Size() const { return sizeof(*this); }

    // non-virtual
    /// VA: 0x0071AB60
#if defined(RA2_YRPP_GAME)
    void UnlinkPointer(AbstractClass* object) { JMP_THIS(0x71AB60); }
#else
    void UnlinkPointer(AbstractClass* object);
#endif
    /// VA: 0x0071AF20.
#if defined(RA2_YRPP_GAME)
    void Fire(TechnoClass* pTarget) { JMP_THIS(0x71AF20); }
#else
    void Fire(TechnoClass* pTarget);
#endif
    /// VA: 0x0071AE50.
#if defined(RA2_YRPP_GAME)
    bool CanWarpTarget(TechnoClass* pTarget) const { JMP_THIS(0x71AE50); }
#else
    bool CanWarpTarget(TechnoClass* pTarget) const;
#endif

    // hardcoded to accumulate only up to 50 helpers
    /// VA: 0x0071AB10.
#if defined(RA2_YRPP_GAME)
    int GetWarpPerStep(int nHelperCount = 0) const { JMP_THIS(0x71AB10); }
#else
    int GetWarpPerStep(int nHelperCount = 0) const;
#endif

    /// VA: 0x0071ABC0.
#if defined(RA2_YRPP_GAME)
    void LetGo() { JMP_THIS(0x71ABC0); }
#else
    void LetGo();
#endif
    /// VA: 0x0071AD40.
#if defined(RA2_YRPP_GAME)
    void JustLetGo() { JMP_THIS(0x71AD40); }
#else
    void JustLetGo() noexcept;
#endif
    /// VA: 0x0071ADE0.
#if defined(RA2_YRPP_GAME)
    void Detach() { JMP_THIS(0x71ADE0); }
#else
    void Detach() noexcept;
#endif

    // Constructor
    /// VA: 0x0071A4E0.
#if defined(RA2_YRPP_GAME)
    TemporalClass(TechnoClass* pOwnerUnit) noexcept : TemporalClass(noinit_t()) { JMP_THIS(0x71A4E0); }
#else
    TemporalClass(TechnoClass* pOwnerUnit) noexcept;
#endif

protected:
    explicit __forceinline TemporalClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    TechnoClass* Owner;
    TechnoClass* Target;
    CDTimerClass LifeTimer;
    void* unknown_pointer_38;
    SuperClass* SourceSW;

    TemporalClass* NextTemporal;
    TemporalClass* PrevTemporal;

    int WarpRemaining;
    mutable int WarpPerStep;
};
