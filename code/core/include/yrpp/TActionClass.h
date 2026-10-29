#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"
class ObjectClass;
class TeamTypeClass;

// forward declarations
class SuperClass;
class TechnoClass;
class TagTypeClass;
class TriggerTypeClass;
class TriggerClass;

class TActionClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Action;

    // Static
    /// Global VA: 0x00B0E658.
    static DynamicVectorClass<TActionClass*>& Array;

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x006E3DB0; original retained.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x006E3DB0); }
    /// VA: 0x006E3E30; original retained.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x006E3E30); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TActionClass();

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual void PointerExpired(AbstractClass* object, bool removed) override;
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const override;
    /// VA: unknown (legacy placeholder).
    virtual int Size() const override;
    /// VA: 0x006E3E50; original retained.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x006E3E50); }
    /// VA: unknown (legacy placeholder).
    virtual int GetArrayIndex() const override;

    // you are responsible for doing INI::ReadString and strtok'ing it before calling
    // this func only calls strtok again, doesn't know anything about buffers
    /// VA: 0x006DD5B0.
#if defined(RA2_YRPP_GAME)
    void LoadFromINI() { JMP_THIS(0x6DD5B0); }
#else
    void LoadFromINI();
    // Checked native record: explicit cursor, false on malformed input or failure.
    // The caller discards failed records. No exceptions escape either entry.
    bool LoadFromINI(const char*& cursor) noexcept;
#endif

    // you allocate the buffer for this, and save it to ini yourself after this returns
    // this func only sprintf's the stuff it needs into buffer
    /// VA: 0x006DD300.
    void PrepareSaveToINI(char* buffer) const { JMP_THIS(0x6DD300); }

    // fuck if I know what's the purpose of this, returns a bitfield of flags for trigger logic
    /// VA: 0x006E3EE0.
    static int GetFlags(int actionKind) { JMP_STD(0x6E3EE0); }

    // transforms actionKind to a number saying what to parse arguments as (team/tag/trigger id, waypoint, integer, etc)
    /// VA: 0x006E3B60.
    static int GetMode(int actionKind) { JMP_STD(0x6E3B60); }

    // main brain, returns whether succeeded (mostly, no consistency in results what so ever)
    // trigger fires all actions regardless of result of this
    /// VA: 0x006DD8B0.
    bool Execute(HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6DD8B0);
    }
#else
        ;
#endif

    // BIG LIST OF EXECUTE'S SLAVE FUNCTIONS - feel free to use

    // NOTE: most of these are defined as separate functions AS WELL AS inlined in Execute() above.
    // Ergo, hooking into them by their address will not always override builtin handling.
    // If you need to know which are inlined, poke me.
#pragma push_macro("ACTION_FUNC")

#define ACTION_FUNC(name, addr)                                                                                        \
    bool name(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,                            \
              CellStruct const& location) {                                                                            \
        JMP_THIS(addr);                                                                                                \
    }

#if defined(RA2_YRPP_GAME)
#define PORTED_ACTION_FUNC(name, addr) ACTION_FUNC(name, addr)
#else
#define PORTED_ACTION_FUNC(name, addr)                                                                                 \
    bool name(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,                            \
              CellStruct const& location) noexcept;
#endif

    /// VA: 0x006E0EF0
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    bool SetAutoBaseBuilding(HouseClass* house, ObjectClass* object, TriggerClass* trigger, const CellStruct& location) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x6E0EF0
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    ACTION_FUNC(SetAutoBaseBuilding, 0x6E0EF0);
#else
    bool SetAutoBaseBuilding(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept;
#endif

    /// VA: 0x006E0050
    PORTED_ACTION_FUNC(LightningStrikeAt, 0x6E0050);
    /// VA: 0x006E0080
    bool RemoveParticleSystemsAt(HouseClass* house, ObjectClass* object, TriggerClass* trigger,
                                 CellStruct const& location)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x006E0080);
    }
#else
        ;
#endif
    /// VA: 0x006E0110
    bool CreateParticleSystemAt(HouseClass* house, ObjectClass* object, TriggerClass* trigger,
                                CellStruct const& location)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x006E0110);
    }
#else
        ;
#endif
    /// VA: 0x006E01C0
    PORTED_ACTION_FUNC(WakeupAttachedObjects, 0x6E01C0);

    ACTION_FUNC(SetVeinGrowth, 0x6E0250);
    ACTION_FUNC(SetTiberiumGrowth, 0x6E0270);
    ACTION_FUNC(SetIceGrowth, 0x6E0290);

    ACTION_FUNC(WakeupAllIdle, 0x6E02B0);
    ACTION_FUNC(WakeupAllHarmless, 0x6E0330);
    ACTION_FUNC(WakeupGroup, 0x6E03B0);

    /// VA: 0x006E0440
    PORTED_ACTION_FUNC(PlayerWin, 0x6E0440);
    /// VA: 0x006E0460
    PORTED_ACTION_FUNC(PlayerLose, 0x6E0460);
    ACTION_FUNC(EndScenario, 0x6E0480);

    /// VA: 0x006E0490
    PORTED_ACTION_FUNC(Apply100DamageAt, 0x6E0490);
    ACTION_FUNC(SmallLightFlashAt, 0x6E0790);
    ACTION_FUNC(MediumLightFlashAt, 0x6E07F0);
    ACTION_FUNC(LargeLightFlashAt, 0x6E0850);

    PORTED_ACTION_FUNC(SellAttachedBuildings, 0x6E08B0);
    ACTION_FUNC(BerserkAttachedInfantry, 0x6E0930);
    /// VA: 0x006E09A0
    PORTED_ACTION_FUNC(SendAttachedBuildingsOffline, 0x6E09A0);
    ACTION_FUNC(SendAttachedBuildingsOnline, 0x6E0A20);

    /// VA: 0x006E0AA0.
    bool SwitchAttachedObjectsToHouse(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,
                                      CellStruct const& location)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E0AA0);
    }
#else
        ;
#endif
    /// VA: 0x006E0B60
    PORTED_ACTION_FUNC(SwitchAllObjectsToHouse, 0x6E0B60);
    ACTION_FUNC(MindControlBuildingsOfHouse, 0x6E0CA0);
    ACTION_FUNC(RelinquishMindControlOfBuildingsOfHouse, 0x6E0D00);

    /// VA: 0x006E0D60
#if defined(RA2_YRPP_GAME)
    bool PrintMessage(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,
                      CellStruct const& location) {
        JMP_THIS(0x6E0D60);
    }
#else
    // Requires the active scenario and, for a nonempty key, its rules. No
    // exceptions escape; allocation failure retains the fatal message contract.
    bool PrintMessage(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,
                      CellStruct const& location) noexcept;
#endif
    /// VA: 0x006E0DF0
    PORTED_ACTION_FUNC(MakeTwoHousesAlly, 0x6E0DF0);
    /// VA: 0x006E0E60
    PORTED_ACTION_FUNC(MakeTwoHousesEnemy, 0x6E0E60);

    ACTION_FUNC(SetTargetCell, 0x6E0ED0);

    ACTION_FUNC(GrowShroud, 0x6E0F90);

    ACTION_FUNC(GlobalSet, 0x6E0FA0);
    ACTION_FUNC(GlobalClear, 0x6E0FC0);

    // Requires the active scenario, map, rules and current player. No
    // exceptions may escape these visibility actions.
    /// VA: 0x006E0FE0
    PORTED_ACTION_FUNC(RevealAroundWaypoint, 0x6E0FE0);
    ACTION_FUNC(ReduceTiberiumAt, 0x6E1180);
    /// VA: 0x006E11C0
    PORTED_ACTION_FUNC(RevealZoneOfWaypoint, 0x6E11C0);

    PORTED_ACTION_FUNC(MissionTimerStart, 0x6E13A0);
    /// VA: 0x006E13E0
    PORTED_ACTION_FUNC(MissionTimerStop, 0x6E13E0);
    /// VA: 0x006E1440
    PORTED_ACTION_FUNC(MissionTimerExtend, 0x6E1440);
    ACTION_FUNC(MissionTimerShorten, 0x6E14A0);
    PORTED_ACTION_FUNC(MissionTimerStartWithText, 0x6E1530);
    PORTED_ACTION_FUNC(MissionTimerSetText, 0x6E15F0);

    /// VA: 0x006E16D0
    PORTED_ACTION_FUNC(PlayMovie, 0x6E16D0);
    ACTION_FUNC(PlayMovieInSidebar, 0x6E1720);
    ACTION_FUNC(PlayMovieInSidebarPauseGame, 0x6E1740);

    ACTION_FUNC(PlayAudio, 0x6E1760);
    ACTION_FUNC(PlayAudioAtRandomWP, 0x6E1780);
    /// VA: 0x006E18B0
    PORTED_ACTION_FUNC(PlayAudioAtSpecificWP, 0x6E18B0);
    /// VA: 0x006E1980
    PORTED_ACTION_FUNC(StopAudioAtSpecificWP, 0x6E1980);

    /// VA: 0x006E1A40
    PORTED_ACTION_FUNC(TeleportAllHouseObjectTo, 0x6E1A40);

    /// VA: 0x006E1A70
    PORTED_ACTION_FUNC(ReshroudMapAroundWaypoint, 0x6E1A70);

    /// VA: 0x006E1B90
    PORTED_ACTION_FUNC(PlayTheme, 0x6E1B90);
    /// VA: 0x6E1BB0
#if defined(RA2_YRPP_GAME)
    bool PlayEVA(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,
                 CellStruct const& location) {
        JMP_THIS(0x6E1BB0);
    }
#else
    // Submits an interrupt using the voice's priority, then returns true even
    // when playback is unavailable. No exceptions escape this boundary.
    bool PlayEVA(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger,
                 CellStruct const& location) noexcept;
#endif

    ACTION_FUNC(Grant1ShotSW, 0x6E1BD0);
    ACTION_FUNC(GrantPermanentSW, 0x6E1C40);

    ACTION_FUNC(DiplayDropFlare, 0x6E1CC0);

    ACTION_FUNC(WinnerIs, 0x6E1DC0);
    /// VA: 0x006E1E00
    PORTED_ACTION_FUNC(LoserIs, 0x6E1E00);

    /// VA: 0x006E1E40
    PORTED_ACTION_FUNC(EnableProductionForHouse, 0x6E1E40);
    /// VA: 0x006E1EA0
    PORTED_ACTION_FUNC(FireSale, 0x6E1EA0);
    ACTION_FUNC(EnableAutocreateForHouse, 0x6E1F00);

    /// VA: 0x006E1F60
#if defined(RA2_YRPP_GAME)
    ACTION_FUNC(CreateTeam, 0x6E1F60);
#else
    bool CreateTeam(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept;
#endif
    /// VA: 0x006E1F90
    PORTED_ACTION_FUNC(DestroyAllInstancesOfTeam, 0x6E1F90);

    /// VA: 0x6E1FB0
#if defined(RA2_YRPP_GAME)
    ACTION_FUNC(SpawnTeam, 0x6E1FB0);
#else
    bool SpawnTeam(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&);
#endif
    /// VA: 0x6E1FD0
#if defined(RA2_YRPP_GAME)
    ACTION_FUNC(SpawnTeamAtWP, 0x6E1FD0);
#else
    bool SpawnTeamAtWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&);
#endif

    /// VA: 0x006E2050
    PORTED_ACTION_FUNC(DestroyAttached, 0x6E2050);

    /// VA: 0x006E21E0
    PORTED_ACTION_FUNC(ResizePlayerView, 0x6E21E0);

    /// VA: 0x006E2290
    PORTED_ACTION_FUNC(PlayAnimAtWP, 0x6E2290);
    /// VA: 0x006E2390
    PORTED_ACTION_FUNC(FireWeaponAtWP, 0x6E2390);
    ACTION_FUNC(PlayVoxelAnimAtWP, 0x6E2520);

    /// VA: 0x006E2600
    PORTED_ACTION_FUNC(LightningStormStart, 0x6E2600);
    ACTION_FUNC(LightningStormStop, 0x6E2640);

    ACTION_FUNC(JumpCameraHome, 0x6E2850);

    ACTION_FUNC(ZoomIn, 0x6E2860);
    ACTION_FUNC(ZoomOut, 0x6E28DC);

    ACTION_FUNC(SetAttachedSpotlightBehaviour, 0x6E2970);

    PORTED_ACTION_FUNC(DeleteTrigger, 0x6E2A10);
    /// VA: 0x006E2A50
    PORTED_ACTION_FUNC(DeleteTag, 0x6E2A50);
    /// VA: 0x006E2AA0
    PORTED_ACTION_FUNC(ForceTrigger, 0x6E2AA0);
    ACTION_FUNC(EnableTrigger, 0x6E2AF0);
    /// VA: 0x006E2BB0
    PORTED_ACTION_FUNC(RadarEventAdd, 0x6E2BB0);

    ACTION_FUNC(LocalSet, 0x6E2BE0);
    ACTION_FUNC(LocalClear, 0x6E2C00);

    /// VA: 0x006E2C20
    PORTED_ACTION_FUNC(ClearSmudges, 0x6E2C20);

    ACTION_FUNC(StartMeteorShower, 0x6E2C40);

    ACTION_FUNC(SetAmbientChangeStep, 0x6E2E20);
    ACTION_FUNC(SetAmbientChangeRate, 0x6E2E40);

    ACTION_FUNC(SetTintRed, 0x6E2E60);
    ACTION_FUNC(SetTintGreen, 0x6E2EB0);
    ACTION_FUNC(SetTintBlue, 0x6E2F00);

    ACTION_FUNC(SetAmbientLevel, 0x6E2F4F);

    ACTION_FUNC(SetChronoScreenEffectState, 0x6E2F90);

    /// VA: 0x006E2FA0
    PORTED_ACTION_FUNC(EnableAITriggers, 0x6E2FA0);
    /// VA: 0x006E3000
    PORTED_ACTION_FUNC(DisableAITriggers, 0x6E3000);

    PORTED_ACTION_FUNC(Cheer, 0x6E3060);

    ACTION_FUNC(RestoreStartingUnitsOf, 0x6E30C0);
    /// VA: 0x006E3120
    PORTED_ACTION_FUNC(RestoreStartingBuildingsOf, 0x6E3120);

    /// VA: 0x006E3180
    PORTED_ACTION_FUNC(DestroyAllObjectsOf, 0x6E3180);
    /// VA: 0x006E31E0
    PORTED_ACTION_FUNC(DestroyAllBuildingsOf, 0x6E31E0);
    /// VA: 0x006E3240
    PORTED_ACTION_FUNC(DestroyAllNonNavalUnitsOf, 0x6E3240);
    /// VA: 0x006E32A0
    PORTED_ACTION_FUNC(DestroyAllNavalUnitsOf, 0x6E32A0);

    ACTION_FUNC(SetRatioAITeam, 0x6E3300);
    /// VA: 0x006E3320
    PORTED_ACTION_FUNC(SetRatioAITeamAircraft, 0x6E3320);
    /// VA: 0x006E3340
    PORTED_ACTION_FUNC(SetRatioAITeamInfantry, 0x6E3340);
    ACTION_FUNC(SetRatioAITeamUnits, 0x6E3360);

    ACTION_FUNC(IonBlastAtWP, 0x6E3380);
    ACTION_FUNC(NukeAtWP, 0x6E3410);
    /// VA: 0x006E35F0
    PORTED_ACTION_FUNC(LightningStormAtWP, 0x6E35F0);
    ACTION_FUNC(IronCurtainAtWP, 0x6E36E0);

    /// VA: 0x006E37F0
    PORTED_ACTION_FUNC(SetTechLevelOfTechnoType, 0x6E37F0);

    /// VA: 0x006E3890
    PORTED_ACTION_FUNC(ChronoInTeam, 0x6E3890);

    /// VA: 0x006E38B0
    PORTED_ACTION_FUNC(CrateAtWP, 0x6E38B0);

    ACTION_FUNC(ChemLauncherAtWP, 0x6E38F0);

    ACTION_FUNC(ToggleTrainCrate, 0x6E3B20);

    /// VA: 0x006E3B40
    PORTED_ACTION_FUNC(RadarBlackout, 0x6E3B40);

    /// VA: 0x006E4020
    PORTED_ACTION_FUNC(FlashTeamFor, 0x6E4020);
    ACTION_FUNC(TalkBubble, 0x6E4040);

    ACTION_FUNC(PauseScenarioFor, 0x6E4080);

    /// VA: 0x006E4090
    PORTED_ACTION_FUNC(EjectOccupantsOfFirstAttachedBuilding, 0x6E4090);

    /// VA: 0x006E4100
    PORTED_ACTION_FUNC(SwitchToTab, 0x6E4100);
    /// VA: 0x006E4150
    PORTED_ACTION_FUNC(FlashCameo, 0x6E4150);

    /// VA: 0x006E4200
    PORTED_ACTION_FUNC(SpawnBuilding, 0x6E4200);

    /// VA: 0x006E42D0
    PORTED_ACTION_FUNC(SetSWChargeLevel, 0x6E42D0);
    /// VA: 0x006E4320
    PORTED_ACTION_FUNC(SetSWRechargeTime, 0x6E4320);
    ACTION_FUNC(ResetSWRechargeTime, 0x6E4360);
    ACTION_FUNC(ResetSW, 0x6E43A0);

    /// VA: 0x006E43E0
    PORTED_ACTION_FUNC(SetPreferredTargetCell, 0x6E43E0);
    /// VA: 0x006E4440
    PORTED_ACTION_FUNC(ResetTargetCell, 0x6E4440);

    ACTION_FUNC(SetDefenseCell, 0x6E4460);
    ACTION_FUNC(ResetDefenseCell, 0x6E44C0);

    /// VA: 0x006E44E0
    PORTED_ACTION_FUNC(SetBaseCenter, 0x6E44E0);
    ACTION_FUNC(ResetBaseCenter, 0x6E4540);

    /// VA: 0x6E4560
#if defined(RA2_YRPP_GAME)
    ACTION_FUNC(FlashBuildingsOfType, 0x6E4560);
#else
    bool FlashBuildingsOfType(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&);
#endif

#undef ACTION_FUNC
#undef PORTED_ACTION_FUNC
#pragma pop_macro("ACTION_FUNC")
    // WHEEEEEW. End of slave functions.

    /// VA: 0x006E45E0.
    HouseClass* FindHouseByIndex(TriggerClass* pTrigger, int idxHouse) const
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E45E0);
    }
#else
        ;
#endif

    // Constructor
    /// VA: 0x006DD000.
    TActionClass();

protected:
    explicit __forceinline TActionClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    int ArrayIndex;
    TActionClass* NextAction;
    TriggerAction ActionKind;
    TeamTypeClass* TeamType;
    union {
        RectangleStruct Bounds;  // map bounds for use with action 40
        struct {
            int Param3;
            int Param4;
            int Param5;
            int Param6;
        };
    };  // It's enough for calling Bounds.X, just use a union here now. - secsome
    int Waypoint;
    int Value2;  // multipurpose
    TagTypeClass* TagType;
    TriggerTypeClass* TriggerType;
    char TechnoID[0x19];
    char Text[0x20];
    PROTECTED_PROPERTY(BYTE, align_8D[3]);
    int Value;  // multipurpose
};
