#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/IndexClass.h"

#include "yrpp/Helpers/CompileTime.h"

class NOVTABLE CommandClass {
public:
    // static
    /// Global VA: 0x0087F658.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<CommandClass*>, Array, 0x87F658u)
#else
    static DynamicVectorClass<CommandClass*>& Array;
#endif
    /// Global VA: 0x0087F680.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE((IndexClass<int, CommandClass*>), Hotkeys, 0x87F680u)
#else
    static IndexClass<int, CommandClass*>& Hotkeys;
#endif

    // Command lookup/dispatch phases of Game::KeyboardProcess (0x55DEE0).
    // Non-virtual helpers preserve the original object layout and vtable.
    // False on a miss or failure, with a null output. No exceptions escape.
    static bool FindHotkey(WWKey input, CommandClass*& command) noexcept;
    // False on failure. On success, drain requests duplicate device-key removal.
    // No exceptions escape; drain is false when processing fails.
    bool ProcessInput(WWKey input, bool& drain) const noexcept;

    // CommandClass
    virtual ~CommandClass() = default;
    /// VA: implementation-defined (pure virtual).
    virtual const char* GetName() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual const wchar_t* GetUIName() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual const wchar_t* GetUICategory() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual const wchar_t* GetUIDescription() const = 0;

    /// VA: 0x535BD0
    virtual bool PreventCombinationOverride(WWKey eInput) const  // Do we need to check extra value like SHIFT?
    {
        return false;
    }  // If this value is true, the game won't process
    // Combination keys written here
    // e.g. To ignore SHIFT + this key
    // return eInput & WWKey::Shift;

    /// VA: 0x535BE0
    virtual bool ExtraTriggerCondition(
        WWKey eInput) const  // Only with this key set to true will the game call the Execute
    {
        return !(eInput & WWKey::Release);
    }

    /// VA: 0x535BF0
    // Requests draining up to ten duplicate buffered input events after Execute.
    virtual bool CheckLoop55E020(WWKey eInput) const { return false; }

    /// VA: implementation-defined (pure virtual).
    virtual void Execute(WWKey eInput) const = 0;
};

// Original PlanningMode command, vtable 0x7EB9B4.
class PlanningModeCommandClass final : public CommandClass {
public:
    /// VA: 0x5366A0
    const char* GetName() const override;
    /// VA: 0x5366B0
    const wchar_t* GetUIName() const override;
    /// VA: 0x5366D0
    const wchar_t* GetUICategory() const override;
    /// VA: 0x5366F0
    const wchar_t* GetUIDescription() const override;
    /// VA: 0x536710
    bool PreventCombinationOverride(WWKey input) const override;
    /// VA: 0x536730
    bool ExtraTriggerCondition(WWKey input) const override;
    /// VA: 0x536740
    bool CheckLoop55E020(WWKey input) const override;
    /// VA: 0x536750
    void Execute(WWKey input) const override;
};

// Original Delete command. Planner deletion is supported by the native host.
class DeleteCommandClass final : public CommandClass {
public:
    /// VA: 0x537F20
    const char* GetName() const override;
    /// VA: 0x537F30
    const wchar_t* GetUIName() const override;
    /// VA: 0x537F50
    const wchar_t* GetUICategory() const override;
    /// VA: 0x537F70
    const wchar_t* GetUIDescription() const override;
    /// VA: 0x537F90
    void Execute(WWKey input) const override;
};
