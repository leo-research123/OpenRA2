/*
    Actual AI Team Scripts
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

// forward declarations
#include "yrpp/ScriptTypeClass.h"

class NOVTABLE ScriptClass : public AbstractClass
{
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
    /// VA: unknown (legacy placeholder).
    ~ScriptClass() override;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    AbstractType WhatAmI() const override {return AbsID;}
    /// VA: unknown (legacy placeholder).
    int Size() const override {return sizeof(*this);}

    /// VA: 0x00691500.
#if defined(RA2_YRPP_GAME)
    ScriptActionNode* GetCurrentAction(ScriptActionNode *buffer) const
        { JMP_THIS(0x691500); }
#else
    ScriptActionNode* GetCurrentAction(ScriptActionNode* buffer) const {
        *buffer = CurrentMission == -1 ? ScriptActionNode{-1,0} : Type->ScriptActions[CurrentMission];
        return buffer;
    }
#endif
    /// VA: 0x006915D0
    bool HasCurrentMission() const { return CurrentMission < Type->ActionsCount; }

    /// VA: 0x00691540.
    ScriptActionNode* GetNextAction(ScriptActionNode *buffer) const
        { JMP_THIS(0x691540); }

    /// VA: 0x00691590.
    bool ClearMission()
        { JMP_THIS(0x691590); }

    /// VA: 0x006915A0.
    bool SetMission(int nLine)
        { JMP_THIS(0x6915A0); }

    bool NextMission()
        { ++this->CurrentMission; return this->HasNextMission(); }

    /// VA: 0x006915B0.
    bool HasNextMission() const
        { JMP_THIS(0x6915B0); }

    // Constructor
    /// VA: 0x006913C0.
#if defined(RA2_YRPP_GAME)
    ScriptClass(ScriptTypeClass* pType) noexcept
        : AbstractClass(noinit_t())
    { JMP_THIS(0x6913C0); }
#else
    ScriptClass(ScriptTypeClass* pType) noexcept;
#endif

protected:
    explicit __forceinline ScriptClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    ScriptTypeClass * Type;
    int field_28;
    int CurrentMission;
};
