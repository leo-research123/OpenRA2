#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/IndexClass.h"

#include "yrpp/Helpers/CompileTime.h"

class NOVTABLE CommandClass
{
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
    virtual bool PreventCombinationOverride(WWKey eInput) const // Do we need to check extra value like SHIFT?
        { return false; }										// If this value is true, the game won't process
                                                                // Combination keys written here
                                                                // e.g. To ignore SHIFT + this key
                                                                // return eInput & WWKey::Shift;

    /// VA: 0x535BE0
    virtual bool ExtraTriggerCondition(WWKey eInput) const // Only with this key set to true will the game call the Execute
        { return !(eInput & WWKey::Release); }

    /// VA: 0x535BF0
    // Requests draining up to ten duplicate buffered input events after Execute.
    virtual bool CheckLoop55E020(WWKey eInput) const
        { return false; }

    /// VA: implementation-defined (pure virtual).
    virtual void Execute(WWKey eInput) const = 0;
};
