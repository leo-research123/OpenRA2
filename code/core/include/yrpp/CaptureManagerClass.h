/*
    CaptureManager - used for mind control.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

class HouseClass;
class TechnoClass;

struct ControlNode
{
    explicit ControlNode() noexcept { }
    ~ControlNode() { LinkDrawTimer.~CDTimerClass(); }

    TechnoClass* Unit;
    HouseClass* OriginalOwner;
    DECLARE_PROPERTY(CDTimerClass, LinkDrawTimer);
};

class NOVTABLE CaptureManagerClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::CaptureManager;

    // Static
    /// Global VA: 0x0089E0F0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<CaptureManagerClass*>, Array, 0x89E0F0u)
#else
    static DynamicVectorClass<CaptureManagerClass*>& Array;
#endif

    // IPersist
    /// VA: 0x472960
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x4719A0
    ~CaptureManagerClass() override;

    // AbstractClass
    /// VA: 0x4729B0
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x4729A0
    int Size() const override { return sizeof(*this); }

    // non-virtual
    /// VA: 0x00471F90
#if defined(RA2_YRPP_GAME)
    bool UnlinkPointer(AbstractClass* object) { JMP_THIS(0x471F90); }
#else
    bool UnlinkPointer(AbstractClass* object);
#endif
    /// VA: 0x00471D40.
#if defined(RA2_YRPP_GAME)
    bool CaptureUnit(TechnoClass* pUnit)
        { JMP_THIS(0x471D40); }
#else
    bool CaptureUnit(TechnoClass* pUnit);
#endif
    /// VA: 0x00471FF0.
#if defined(RA2_YRPP_GAME)
    bool FreeUnit(TechnoClass* pUnit)
        { JMP_THIS(0x471FF0); }
#else
    bool FreeUnit(TechnoClass* pUnit);
#endif
    /// VA: 0x00472140.
#if defined(RA2_YRPP_GAME)
    void FreeAll()
        { JMP_THIS(0x472140); }
#else
    void FreeAll();
#endif

    int NumControlNodes() const
        { return ControlNodes.Count; }

    /// VA: 0x00471C90.
#if defined(RA2_YRPP_GAME)
    bool CanCapture(TechnoClass *Target) const
        { JMP_THIS(0x471C90); }
#else
    bool CanCapture(TechnoClass* target) const;
#endif
    /// VA: 0x004722A0.
#if defined(RA2_YRPP_GAME)
    bool CannotControlAnyMore() const
        { JMP_THIS(0x4722A0); }
#else
    bool CannotControlAnyMore() const;
#endif
    /// VA: 0x004722C0.
#if defined(RA2_YRPP_GAME)
    bool IsControllingSomething() const
        { JMP_THIS(0x4722C0); }
#else
    bool IsControllingSomething() const;
#endif
    /// VA: 0x004726C0.
#if defined(RA2_YRPP_GAME)
    bool IsOverloading(bool *wasDamageApplied) const
        { JMP_THIS(0x4726C0); }
#else
    bool IsOverloading(bool* wasDamageApplied) const;
#endif
    /// VA: 0x00471A50.
#if defined(RA2_YRPP_GAME)
    void HandleOverload()
        { JMP_THIS(0x471A50); }
#else
    void HandleOverload();
#endif
    /// VA: 0x00472640.
#if defined(RA2_YRPP_GAME)
    bool NeedsToDrawLinks() const
        { JMP_THIS(0x472640); }
#else
    bool NeedsToDrawLinks() const;
#endif
    /// VA: 0x00472160.
#if defined(RA2_YRPP_GAME)
    void DrawLinks() { JMP_THIS(0x472160); }
#else
    void DrawLinks();
#endif
    /// VA: 0x004723B0.
#if defined(RA2_YRPP_GAME)
    void DecideUnitFate(TechnoClass *Unit)
        { JMP_THIS(0x4723B0); }
#else
    void DecideUnitFate(TechnoClass* unit);
#endif
    /// VA: 0x004722D0.
#if defined(RA2_YRPP_GAME)
    int GetControlledCount()
        { JMP_THIS(0x4722D0); }
#else
    int GetControlledCount();
#endif
    /// VA: 0x004722F0.
#if defined(RA2_YRPP_GAME)
    HouseClass* GetOriginalOwner(TechnoClass *Unit) const
        { JMP_THIS(0x4722F0); }
#else
    HouseClass* GetOriginalOwner(TechnoClass* unit) const;
#endif

    // Constructor
    /// VA: 0x004717D0.
#if defined(RA2_YRPP_GAME)
    CaptureManagerClass(TechnoClass* pOwner, int nMaxControlNodes, bool bInfiniteControl) noexcept
        : CaptureManagerClass(noinit_t())
    { JMP_THIS(0x4717D0); }
#else
    CaptureManagerClass(TechnoClass* pOwner, int nMaxControlNodes, bool bInfiniteControl) noexcept;
#endif

protected:
    explicit __forceinline CaptureManagerClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    DynamicVectorClass<ControlNode*> ControlNodes;
    int MaxControlNodes;
    bool InfiniteMindControl;
    bool OverloadDeathSoundPlayed; // Has the mind control death sound played already?
    int OverloadPipState; // Used to create the red overloading pip by returning true in IsOverloading's wasDamageApplied for 10 frames.
    TechnoClass* Owner;
    int OverloadDamageDelay; // Decremented every frame. If it reaches zero, OverloadDamage is applied.
};
