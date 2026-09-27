/*
    Base class for all game objects with missions (yeah... not many).
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/Timer.h"

class CCINIClass;

class MissionControlClass
{
    public:
        static MissionControlClass (&Array)[0x20];
        static const char* const (&Names)[0x20]; // 816CAC, not the object array.

        static MissionControlClass* YRPP_FASTCALL Find(const char* pName);

        static Mission YRPP_FASTCALL FindIndex(const char* pName);

        static const char* YRPP_FASTCALL FindName(const Mission& index);

        MissionControlClass() noexcept;

        const char* GetName();

        bool LoadFromINI(CCINIClass* pINI);

        // Properties
        int ArrayIndex;
        bool NoThreat;
        bool Zombie;
        bool Recruitable;
        bool Paralyzed;
        bool Retaliate;
        bool Scatter;
        double Rate; //default 0.016
        double AARate; //default 0.016
};

class NOVTABLE MissionClass : public ObjectClass
{
public:
    // Destructor
    virtual ~MissionClass() { /* ~ObjectClass() */ }

    // MissionClass
    /// VA: 0x005B3060
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x5B3060); }
#else
    void Update() override;
#endif
    /// VA: 0x005B3040
#if defined(RA2_YRPP_GAME)
    Mission GetCurrentMission() const override JMP_THIS(0x005B3040);
#else
    Mission GetCurrentMission() const override;
#endif
    // QueueMission's return is the original AL contract, not an acceptance flag.
    // Inspect QueuedMission/CurrentMission to learn whether an order changed.
    /// VA: 0x005B35E0
#if defined(RA2_YRPP_GAME)
    virtual bool QueueMission(Mission mission, bool start_mission) JMP_THIS(0x5B35E0);
#else
    virtual bool QueueMission(Mission mission, bool start_mission);
#endif
    /// VA: 0x005B3570
#if defined(RA2_YRPP_GAME)
    virtual bool NextMission() JMP_THIS(0x5B3570);
#else
    virtual bool NextMission();
#endif
    /// VA: 0x005B2FD0
#if defined(RA2_YRPP_GAME)
    virtual void ForceMission(Mission mission) JMP_THIS(0x5B2FD0);
#else
    virtual void ForceMission(Mission mission);
#endif
    /// VA: 0x005B3650
#if defined(RA2_YRPP_GAME)
    virtual void Override_Mission(Mission mission, AbstractClass* target, AbstractClass* destination) JMP_THIS(0x5B3650);
#else
    virtual void Override_Mission(Mission mission, AbstractClass* target, AbstractClass* destination);
#endif
    /// VA: 0x005B36B0
#if defined(RA2_YRPP_GAME)
    virtual bool Mission_Revert() JMP_THIS(0x5B36B0);
#else
    virtual bool Mission_Revert();
#endif
    /// VA: 0x005B3A10
#if defined(RA2_YRPP_GAME)
    virtual bool MissionIsOverriden() const JMP_THIS(0x5B3A10);
#else
    virtual bool MissionIsOverriden() const;
#endif
    /// VA: 0x004E0140
    virtual bool ReadyToNextMission() const { return true; }

    /// VA: 0x005B2E10
    virtual int Mission_Sleep() { return 450; }
    /// VA: 0x005B2E20
    virtual int Mission_Harmless() { return 450; }
    /// VA: 0x005B2E30
    virtual int Mission_Ambush() { return 450; }
    /// VA: 0x005B2E40
    virtual int Mission_Attack() { return 450; }
    /// VA: 0x005B2E50
    virtual int Mission_Capture() { return 450; }
    /// VA: 0x005B2E60
    virtual int Mission_Eaten() { return 450; }
    /// VA: 0x005B2E70
    virtual int Mission_Guard() { return 450; }
    /// VA: 0x005B2E80
    virtual int Mission_AreaGuard() { return 450; }
    /// VA: 0x005B2E90
    virtual int Mission_Harvest() { return 450; }
    /// VA: 0x005B2EA0
    virtual int Mission_Hunt() { return 450; }
    /// VA: 0x005B2EB0
    virtual int Mission_Move() { return 450; }
    /// VA: 0x005B2EC0
    virtual int Mission_Retreat() { return 450; }
    /// VA: 0x005B2ED0
    virtual int Mission_Return() { return 450; }
    /// VA: 0x005B2EE0
    virtual int Mission_Stop() { return 450; }
    /// VA: 0x005B2EF0
    virtual int Mission_Unload() { return 450; }
    /// VA: 0x005B2F00
    virtual int Mission_Enter() { return 450; }
    /// VA: 0x005B2F10
    virtual int Mission_Construction() { return 450; }
    /// VA: 0x005B2F20
    virtual int Mission_Selling() { return 450; }
    /// VA: 0x005B2F30
    virtual int Mission_Repair() { return 450; }
    /// VA: 0x005B2F40
    virtual int Mission_Missile() { return 450; }
    /// VA: 0x005B2F50
    virtual int Mission_Open() { return 450; }
    /// VA: 0x005B2F60
    virtual int Mission_Rescue() { return 450; }
    /// VA: 0x005B2F70
    virtual int Mission_Patrol() { return 450; }
    /// VA: 0x005B2F80
    virtual int Mission_ParaDropApproach() { return 450; }
    /// VA: 0x005B2F90
    virtual int Mission_ParaDropOverfly() { return 450; }
    /// VA: 0x005B2FA0
    virtual int Mission_Wait() { return 450; }
    /// VA: 0x005B2FB0
    virtual int Mission_SpyPlaneApproach() { return 450; }
    /// VA: 0x005B2FC0
    virtual int Mission_SpyPlaneOverfly() { return 450; }
    
    /// VA: 0x005B36E0.
#if defined(RA2_YRPP_GAME)
    static bool YRPP_FASTCALL IsRecruitableMission(Mission mission)
        { JMP_STD(0x5B36E0); }
#else
    static bool YRPP_FASTCALL IsRecruitableMission(Mission mission);
#endif

    /// VA: 0x005B3A00
    MissionControlClass* CurrentMissionControl() const {
        return &MissionControlClass::Array[static_cast<int>(CurrentMission)];
    }

    // Constructor
#if defined(RA2_YRPP_GAME)
    MissionClass() noexcept
        : MissionClass(noinit_t())
    { THISCALL(0x5B2DA0); }
#else
    MissionClass() noexcept;
#endif

protected:
    explicit __forceinline MissionClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    Mission  CurrentMission;
    Mission  SuspendedMission;
    Mission  QueuedMission;
    bool     unknown_bool_B8;
    int      MissionStatus;
    int      CurrentMissionStartTime;	//in frames
    int      MissionAccumulateTime;
    DECLARE_PROPERTY(CDTimerClass, UpdateTimer);
};
