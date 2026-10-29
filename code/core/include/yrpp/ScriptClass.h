/*
    Actual AI Team Scripts
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include <bit>

// forward declarations
#include "yrpp/ScriptTypeClass.h"

class NOVTABLE ScriptClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Script;
    /// Global VA: 0x008872B0
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<ScriptClass*>, Array, 0x8872B0u)
#else
    static DynamicVectorClass<ScriptClass*>& Array;
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
    /// VA: 0x691EE0
    ~ScriptClass() override;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: unknown (legacy placeholder).
    int Size() const override { return sizeof(*this); }

    /// VA: 0x00691500.
#if defined(RA2_YRPP_GAME)
    ScriptActionNode* GetCurrentAction(ScriptActionNode* buffer) const { JMP_THIS(0x691500); }
#else
    ScriptActionNode* GetCurrentAction(ScriptActionNode* buffer) const {
        *buffer = CurrentMission == -1 ? ScriptActionNode{TMISSION_NONE, 0} : Type->ScriptActions[CurrentMission];
        return buffer;
    }
#endif
    /// VA: 0x006915D0
    bool HasCurrentMission() const { return unsigned(CurrentMission) < unsigned(Type->ActionsCount); }

    /// VA: 0x691540
#if defined(RA2_YRPP_GAME)
    ScriptActionNode* GetNextAction(ScriptActionNode* buffer) const { JMP_THIS(0x691540); }
#else
    ScriptActionNode* GetNextAction(ScriptActionNode* buffer) const;
#endif

    /// VA: 0x00691590.
#if defined(RA2_YRPP_GAME)
    bool ClearMission() { JMP_THIS(0x691590); }
#else
    bool ClearMission() {
        CurrentMission = -1;
        return true;
    }
#endif

    /// VA: 0x006915A0.
#if defined(RA2_YRPP_GAME)
    bool SetMission(int nLine) { JMP_THIS(0x6915A0); }
#else
    bool SetMission(int nLine) {
        CurrentMission = nLine;
        return true;
    }
#endif

    /// VA: 0x6915B0
#if defined(RA2_YRPP_GAME)
    bool NextMission() { JMP_THIS(0x6915B0); }
#else
    bool NextMission() {
        CurrentMission = std::bit_cast<int>(unsigned(CurrentMission) + 1u);
        return unsigned(CurrentMission) < unsigned(Type->ActionsCount);
    }
#endif
    // Read-only lookahead; NextMission is the original advancing operation.
    bool HasNextMission() const { return unsigned(CurrentMission) + 1u < unsigned(Type->ActionsCount); }

    // Constructor
    /// VA: 0x006913C0.
#if defined(RA2_YRPP_GAME)
    ScriptClass(ScriptTypeClass* pType) noexcept : AbstractClass(noinit_t()) { JMP_THIS(0x6913C0); }
#else
    ScriptClass(ScriptTypeClass* pType) noexcept;
#endif

protected:
    explicit __forceinline ScriptClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    ScriptTypeClass* Type;
    int field_28;
    int CurrentMission;
};
