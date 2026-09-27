#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

// forward declarations
class AircraftTypeClass;
class ObjectClass;
class TechnoClass;
class FootClass;

// The AirstrikeClass handles the airstrikes Boris calls in.
class NOVTABLE AirstrikeClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Airstrike;

    // Static
    /// Global VA: 0x00889FB8.
    DEFINE_REFERENCE(DynamicVectorClass<AirstrikeClass*>, Array, 0x889FB8u)

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
    virtual ~AirstrikeClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int	Size() const R0;

    // non-virtual
    /// VA: 0x0041D7E0
    bool CanTarget(ObjectClass* target) const { JMP_THIS(0x41D7E0); }
    /// VA: 0x0041D830.
    void StartMission(ObjectClass* pTarget)
        { JMP_THIS(0x41D830); }

    /// VA: 0x0041DA20.
    void ResetTarget(ObjectClass* pTarget)
    { JMP_THIS(0x41DA20); }

    /// VA: 0x0041DB40.
    void ClearTarget()
    { JMP_THIS(0x41DB40); }

    /// VA: 0x0041D540.
    void InvalidatePointer(void* ptr)
    { JMP_THIS(0x41D540); }

    // Constructor
    /// VA: 0x0041D380.
    AirstrikeClass(TechnoClass* pOwner) noexcept
        : AirstrikeClass(noinit_t())
    { JMP_THIS(0x41D380); }

protected:
    explicit __forceinline AirstrikeClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    int AirstrikeTeam;			//As in the INI files.
    int EliteAirstrikeTeam;	//As in the INI files.
    int AirstrikeTeamTypeIndex;	//As in the INI files.
    int EliteAirstrikeTeamTypeIndex;	//As in the INI files.
    DWORD unknown_34;
    DWORD unknown_38;	//unused?
    bool IsOnMission;	//Is the Aircraft on its way?
    bool unknown_bool_3D;
    DWORD TeamDissolveFrame;	//when was the last time this team was invoked and subsequently dissolved
    int AirstrikeRechargeTime;	//As in the INI files.
    int EliteAirstrikeRechargeTime;	//As in the INI files.
    TechnoClass* Owner;		//The unit that called the Airstrike (usually Boris).
    ObjectClass* Target;	//The Airstrike's target.
    AircraftTypeClass* AirstrikeTeamType;	//As in the INI files.
    AircraftTypeClass* EliteAirstrikeTeamType;	//As in the INI files.
    FootClass* FirstObject;
};
