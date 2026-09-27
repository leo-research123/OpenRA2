#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ScenarioClass.h"
#include "yrpp/QueueClass.h"
#include "yrpp/TargetClass.h"
#include "yrpp/Unsorted.h"

#pragma pack(push, 1)
class EventClass
{
public:
    struct EMPTY { };
    struct POWERON {
        TargetClass Target;
    };
    struct POWEROFF {
        TargetClass Target;
    };
    struct ALLY {
        int HouseID;
    };
    struct MEGAMISSION {
        TargetClass Whom;
        unsigned char Mission;
        char _gap_;
        TargetClass Target;
        TargetClass Destination;
        TargetClass Follow;
        // 0: ordinary order; 1: append to plan; 2: execute a planned node.
        BYTE IsPlanningEvent;
    };
    struct MEGAMISSION_F {
        TargetClass Whom;
        unsigned char Mission;
        TargetClass Target;
        TargetClass Destination;
        int Speed;
        int MaxSpeed;
    };
    struct IDLE {
        TargetClass Whom;
    };
    struct SCATTER {
        TargetClass Whom;
    };
    struct DESTRUCT { };
    struct DEPLOY {
        TargetClass Whom;
    };
    struct DETONATE {
        TargetClass Whom;
    };
    struct PLACE {
        AbstractType RTTIType;
        int HeapID;
        int IsNaval;
        CellStruct Location;
    };
    struct OPTIONS { };
    struct GAMESPEED {
        int GameSpeed;
    };
    struct PRODUCE {
        AbstractType RTTIType;
        int HeapID;
        int IsNaval;
    };
    struct SUSPEND {
        AbstractType RTTIType;
        int HeapID;
        int IsNaval;
    };
    struct ABANDON {
        AbstractType RTTIType;
        int HeapID;
        int IsNaval;
    };
    struct PRIMARY {
        TargetClass Whom;

    };
    struct SPECIAL_PLACE {
        int ID;
        CellStruct Location;
    };
    struct EXIT { };
    struct ANIMATION {
        int AnimID;
        int HouseID;
        Point2D Location;
    };
    struct REPAIR {
        TargetClass Whom;
    };
    struct SELL {
        TargetClass Whom;
    };
    struct SELLCELL {
        CellStruct Location;
    };
    struct SPECIAL {
        ScenarioFlags SpecialFlags;
    };
    struct FRAMESYNC { };
    struct MESSAGE { };
    struct RESPONSE_TIME {
        char unknown;
    };
    struct FRAMEINFO {
        unsigned int CRC;
        unsigned short CommandCount;
        unsigned char Delay;
    };
    struct SAVEGAME { };
    struct ARCHIVE {
        TargetClass Whom1;
        TargetClass Whom2;
    };
    struct ADDPLAYER {
        void* unknownPointer;
    };
    struct TIMING {
        unsigned short RequestedFPS;
        unsigned short MaxAhead;
        unsigned char FrameSendRate;
    };
    struct PROCESS_TIME {
        unsigned short Time;
    };
    struct PAGEUSER { };
    struct REMOVEPLAYER {
        int HouseID;
    };
    struct LATENCYFUDGE {
        int LatencyFudge;
    };
    struct MEGAFRAMEINFO {
        char Unknown[104];
    };
    struct PACKETTIMING {
        char Unknown[64];
    };
    struct ABOUTTOEXIT { };
    struct FALLBACKHOST {
        int FallbackHost;
    };
    struct ADDRESSCHANGE {
        char PlayerID;
        DWORD Address;
    };
    struct PLANCONNECT {
        TargetClass Target1;
        TargetClass Target2;
    };
    struct PLANCOMMIT { };
    struct PLANNODEDELETE {
        TargetClass Target;
    };
    struct ALLCHEER {
        char Unknown[4];
    };
    struct ABANDON_ALL {
        char Unknown[12];
    };

    /// Global VA: 0x0082091C.
    DEFINE_ARRAY_REFERENCE(const char*, [47], EventNames, 0x0082091C)

    enum { MAX_EVENTS = 128 };

    /// Global VA: 0x00A802C8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE((QueueClass<EventClass, MAX_EVENTS>), OutList, 0x00A802C8)
    /// Global VA: 0x008B41F8.
    DEFINE_REFERENCE((QueueClass<EventClass, MAX_EVENTS * 128>), DoList, 0x008B41F8)
#else
    static QueueClass<EventClass, MAX_EVENTS>& OutList;
    static QueueClass<EventClass, MAX_EVENTS * 128>& DoList;
    // Native storage initializes unused wire bytes; original constructors do not.
    EventClass() noexcept : Type(EventType::Empty),IsExecuted(false),HouseIndex(-1),Frame(0),DataBuffer{} {}
#endif
    // Embedded original objects construct only their defined event fields.
    explicit EventClass(noinit_t) noexcept {}

    /// VA: 0x004C6CB0
#if defined(RA2_YRPP_GAME)
    void Execute() { JMP_THIS(0x4C6CB0); }
#else
    void Execute();
#endif

    /// Global VA: 0x00A83ED0.
    DEFINE_REFERENCE((QueueClass<EventClass, MAX_EVENTS * 2>), MegaMissionList, 0x00A83ED0)
    // 8 houses, 8-time cache targets
    /// Global VA: 0x00AC50FC.
    DEFINE_ARRAY_REFERENCE(DWORD, [8 * 8], MegaMissionTargetNum, 0x00AC50FC)
    /// Global VA: 0x00AFA468.
    DEFINE_ARRAY_REFERENCE(TargetClass, [8 * 8][MAX_EVENTS], MegaMissionTargets, 0x00AFA468)

    // this points to CRCs from 0x100 last frames
    /// Global VA: 0x00B04474.
    DEFINE_ARRAY_REFERENCE(DWORD, [256], LatestFramesCRC, 0x00B04474)
    /// Global VA: 0x00AC51FC.
    DEFINE_REFERENCE(DWORD, CurrentFrameCRC, 0x00AC51FC)

    // The engine's out-of-sync sync dump, called from Execute_DoList (0x64CC68)
    // when a FRAMEINFO CRC mismatch is found: writes SYNC*.TXT for the offending
    // event so a desync can be diagnosed. Print_CRCs_Current_Player writes the
    // local player's SYNC<player>.TXT; Print_CRCs_All_Players writes
    // SYNC<player>_<slot>.TXT for each of the 256 frame slots, and the engine
    // only uses it when Unsorted::EnableMPSyncDebug is set.
    /// VA: 0x0064DEA0.
    static void YRPP_FASTCALL Print_CRCs_Current_Player(EventClass* ev)
        { JMP_STD(0x64DEA0); }

    /// VA: 0x006516F0.
    static void YRPP_FASTCALL Print_CRCs_All_Players(int frame_slot, EventClass* ev)
        { JMP_STD(0x6516F0); }

    // Original by-value submission updates only the copied event's Frame.
    /// VA: 0x006521C0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static bool YRPP_STDCALL AddEvent(EventClass event) { JMP_STD(0x6521C0); }
#else
    static bool YRPP_STDCALL AddEvent(EventClass event) noexcept;
#endif

    /// VA: 0x004C66C0.
    explicit EventClass(int houseIndex, EventType eventType)
    {
        JMP_THIS(0x4C66C0);
    }

    // Special
    /// VA: 0x004C65A0.
    explicit EventClass(int houseIndex, int id)
    {
        JMP_THIS(0x4C65A0);
    }

    // Target
    /// VA: 0x004C65E0.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, EventType eventType, int id, int rtti)
    {
        JMP_THIS(0x4C65E0);
    }
#else
    explicit EventClass(int houseIndex, EventType eventType, int id, int rtti) noexcept;
#endif

    // Sellcell
    /// VA: 0x004C6650.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, EventType eventType, const CellStruct& cell)
    {
        JMP_THIS(0x4C6650);
    }
#else
    explicit EventClass(int houseIndex, EventType eventType, const CellStruct& cell) noexcept;
#endif

    // Archive & Planning_Connect
    /// VA: 0x004C6780.
    explicit EventClass(int houseIndex, EventType eventType, TargetClass src, TargetClass dest)
    {
        JMP_THIS(0x4C6780);
    }

    // Anim
    /// VA: 0x004C6800.
    explicit EventClass(int houseIndex, int anim_id, HouseClass* pHouse, const CellStruct& cell)
    {
        JMP_THIS(0x4C6800);
    }

    // MegaMission
    /// VA: 0x004C6860.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, TargetClass src, Mission mission, TargetClass target, TargetClass dest, TargetClass follow)
    {
        JMP_THIS(0x4C6860);
    }
#else
    explicit EventClass(int houseIndex, TargetClass src, Mission mission, TargetClass target, TargetClass dest, TargetClass follow) noexcept;
#endif

    // MegaMission_F
    /// VA: 0x004C68E0.
    explicit EventClass(int houseIndex, TargetClass src, Mission mission, TargetClass target, TargetClass dest, SpeedType speed, int/*MPHType*/ maxSpeed)
    {
        JMP_THIS(0x4C68E0);
    }

    // Production
    /// VA: 0x004C6970.
    explicit EventClass(int houseIndex, EventType eventType, int rtti_id, int heap_id, BOOL is_naval)
    {
        JMP_THIS(0x4C6970);
    }

    // Place with default heap ID -1 and naval flag 0. The last argument is
    // a cell pointer, not an int reference (which made Target ambiguous).
    /// VA: 0x004C69E0.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rtti, const CellStruct& cell)
    {
        JMP_THIS(0x4C69E0);
    }
#else
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rtti, const CellStruct& cell) noexcept;
#endif

    // Place with an explicit heap ID and naval flag 0.
    /// VA: 0x004C6A60.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rtti, int heapid, const CellStruct& cell)
    {
        JMP_THIS(0x4C6A60);
    }
#else
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rtti, int heapid, const CellStruct& cell) noexcept;
#endif

    // Place
    /// VA: 0x004C6AE0.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rttitype, int heapid, int is_naval, const CellStruct& cell)
    {
        JMP_THIS(0x4C6AE0);
    }
#else
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rtti, int heapid, int naval, const CellStruct& cell) noexcept;
#endif

    // SpecialPlace
    /// VA: 0x004C6B60.
#if defined(RA2_YRPP_GAME)
    explicit EventClass(int houseIndex, EventType eventType, int id, const CellStruct& cell)
    {
        JMP_THIS(0x4C6B60);
    }
#else
    explicit EventClass(int houseIndex, EventType eventType, int id, const CellStruct& cell) noexcept;
#endif

    // Specific?, maybe int[2] otherwise
    /// VA: 0x004C6BE0.
    explicit EventClass(int houseIndex, EventType eventType, AbstractType rttitype, int id)
    {
        JMP_THIS(0x4C6BE0);
    }

    // Address Change
    /// VA: 0x004C6C50.
    explicit EventClass(int houseIndex, void* /*IPAddressClass*/ ip, char unknown_0)
    {
        JMP_THIS(0x4C6C50);
    }

    explicit EventClass(const EventClass& another)
    {
        memcpy(this, &another, sizeof(*this));
    }

    EventClass& operator=(const EventClass& another)
    {
        if (this != &another)
            memcpy(this, &another, sizeof(*this));

        return *this;
    }

    bool operator==(const EventClass& q) const
    {
        return memcmp(this, &q, sizeof(q)) == 0;
    };

    // ========================
    EventType Type;
    bool IsExecuted;
    char HouseIndex; // '-1' stands for not a valid house
    unsigned int Frame; // 'Frame' is the frame that the command should execute on.

    union
    {
        char DataBuffer[104];

        EMPTY Empty;

        POWERON Poweron;

        POWEROFF Poweroff;

        ALLY Ally;

        MEGAMISSION MegaMission;

        MEGAMISSION_F MegaMissionF;

        IDLE Idle;

        SCATTER Scatter;

        DESTRUCT Destruct;

        DEPLOY Deploy;

        DETONATE Detonate;

        PLACE Place;

        OPTIONS Options;

        GAMESPEED GameSpeed;

        // This event starts production of the specified object type. The house can
        // determine from the type and ID value, what object to begin production on and
        // what factory to use.
        PRODUCE Produce;

        SUSPEND Suspend;

        // This event is generated when the player cancels production of the specified
        // object type. From the object type, the exact factory can be inferred.
        ABANDON Abandon;

        // Toggles the primary factory state of the specified building.
        PRIMARY Primary;

        SPECIAL_PLACE SpecialPlace;

        EXIT Exit;

        ANIMATION Animation;

        // Starts or stops repair on the specified object. This event is triggered by the
        // player clicking the repair wrench on a building.
        REPAIR Repair;

        // Tells a building/unit to sell. This event is triggered by the player clicking the
        // sell animating cursor over the building or unit.
        SELL Sell;

        // Used to sell walls
        SELLCELL SellCell;

        // Update the special control flags. This is necessary so that in a multiplayer
        // game, all machines will agree on the rules. If these options change during
        // game play, then all players are informed that options have changed.
        SPECIAL Special;

        FRAMESYNC FrameSync;

        MESSAGE Message;

        RESPONSE_TIME ResponseTime;

        FRAMEINFO FrameInfo;

        SAVEGAME SaveGame;

        // Update the archive target for this building.
        ARCHIVE Archive;

        ADDPLAYER AddPlayer;

        TIMING Timing;

        PROCESS_TIME ProcessTime;

        PAGEUSER PageUser;

        REMOVEPLAYER RemovePlayer;

        LATENCYFUDGE LatencyFudge;

        MEGAFRAMEINFO MegafFameInfo;

        PACKETTIMING PacketTiming;

        ABOUTTOEXIT AboutToExit;

        FALLBACKHOST FallbackHost;

        ADDRESSCHANGE AddressChange;

        PLANCONNECT PlanConnect;

        PLANCOMMIT PlanCommit;

        PLANNODEDELETE PlanNodeDelete;

        ALLCHEER AllCheer;

        ABANDON_ALL AbandonAll;
    };
};
#pragma pack(pop)

static_assert(sizeof(EventClass) == 111);
static_assert(offsetof(EventClass, DataBuffer) == 7);
static_assert(offsetof(EventClass, MegaMission) + offsetof(EventClass::MEGAMISSION, Mission) == 0xC);
static_assert(offsetof(EventClass, MegaMission) + offsetof(EventClass::MEGAMISSION, Target) == 0xE);
static_assert(offsetof(EventClass, MegaMission) + offsetof(EventClass::MEGAMISSION, Destination) == 0x13);
static_assert(offsetof(EventClass, MegaMission) + offsetof(EventClass::MEGAMISSION, Follow) == 0x18);
static_assert(offsetof(EventClass, MegaMission) + offsetof(EventClass::MEGAMISSION, IsPlanningEvent) == 0x1D);
