#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

// forward declarations
class AircraftTypeClass;
class ObjectClass;
class TechnoClass;
class FootClass;

// The AirstrikeClass handles the airstrikes Boris calls in.
class NOVTABLE AirstrikeClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Airstrike;

    // Static
    /// Global VA: 0x00889FB8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AirstrikeClass*>, Array, 0x889FB8u)
#else
    static DynamicVectorClass<AirstrikeClass*>& Array;
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
    /// VA: 0x41DD50
    ~AirstrikeClass() override;

    // AbstractClass
    /// VA: 0x41DD40
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x41DD30
    int Size() const override { return sizeof(*this); }

    // non-virtual
    /// VA: 0x41D7E0
#if defined(RA2_YRPP_GAME)
    bool CanTarget(ObjectClass* target) const { JMP_THIS(0x41D7E0); }
#else
    bool CanTarget(ObjectClass* target) const;
#endif
    /// VA: 0x41D830
#if defined(RA2_YRPP_GAME)
    void StartMission(ObjectClass* target) { JMP_THIS(0x41D830); }
#else
    void StartMission(ObjectClass* target);
#endif
    /// VA: 0x41D860
#if defined(RA2_YRPP_GAME)
    void CreateMission(ObjectClass* target) { JMP_THIS(0x41D860); }
#else
    void CreateMission(ObjectClass* target);
#endif
    /// VA: 0x41DA20
#if defined(RA2_YRPP_GAME)
    void ResetTarget(ObjectClass* target) { JMP_THIS(0x41DA20); }
#else
    void ResetTarget(ObjectClass* target);
#endif
    /// VA: 0x41DB40
#if defined(RA2_YRPP_GAME)
    void ClearTarget() { JMP_THIS(0x41DB40); }
#else
    void ClearTarget();
#endif
    /// VA: 0x41DC80
#if defined(RA2_YRPP_GAME)
    void RemoveAircraft(FootClass* aircraft) { JMP_THIS(0x41DC80); }
#else
    void RemoveAircraft(FootClass* aircraft);
#endif
    /// VA: 0x41D540
#if defined(RA2_YRPP_GAME)
    void InvalidatePointer(void* ptr) { JMP_THIS(0x41D540); }
#else
    void InvalidatePointer(void* ptr);
#endif
    /// VA: 0x41D380
#if defined(RA2_YRPP_GAME)
    AirstrikeClass(TechnoClass* owner) noexcept : AirstrikeClass(noinit_t()) { JMP_THIS(0x41D380); }
#else
    AirstrikeClass(TechnoClass* owner) noexcept;
#endif

protected:
    explicit __forceinline AirstrikeClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    int AirstrikeTeam;                //As in the INI files.
    int EliteAirstrikeTeam;           //As in the INI files.
    int AirstrikeTeamTypeIndex;       // Original number of aircraft created (0x2C).
    int EliteAirstrikeTeamTypeIndex;  // Aircraft remaining in the linked list (0x30).
    DWORD unknown_34;
    DWORD unknown_38;  //unused?
    bool IsOnMission;  //Is the Aircraft on its way?
    bool unknown_bool_3D;
    DWORD TeamDissolveFrame;               //when was the last time this team was invoked and subsequently dissolved
    int AirstrikeRechargeTime;             //As in the INI files.
    int EliteAirstrikeRechargeTime;        //As in the INI files.
    TechnoClass* Owner;                    //The unit that called the Airstrike (usually Boris).
    ObjectClass* Target;                   //The Airstrike's target.
    AircraftTypeClass* AirstrikeTeamType;  //As in the INI files.
    AircraftTypeClass* EliteAirstrikeTeamType;  //As in the INI files.
    FootClass* FirstObject;
};
