#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ArrayClasses.h"
#include "yrpp/GameModeOptionsClass.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/IPX.h"
#ifdef _WIN32
#include <winsock.h>
#endif
#include "yrpp/VersionClass.h"
#include "yrpp/Helpers/CompileTime.h"
#include "yrpp/CCFileClass.h"

class MPGameModeClass;
class HouseClass;

struct SessionOptionsClass
{
    int MPGameMode;
    int ScenIndex;
    int GameSpeed;
    int Credits;
    int UnitCount;
    bool ShortGame;
    bool SuperWeaponsAllowed;
    bool BuildOffAlly;
    bool MCVRepacks;
    bool CratesAppear;
    Vector3D<int> SlotData[8];
};

// The original packed records are only native objects under the x86 Microsoft
// ABI. Other hosts need native alignment for wchar_t and nested file objects.
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma pack(push, 1)
#endif
struct NodeNameType {
    /// Global VA: 0x00A8DA74.
    DEFINE_REFERENCE(DynamicVectorClass<NodeNameType*>, Array, 0xA8DA74)
    void SetCountry(int country); // 696F90; -2 retains the random-country marker.
    int GetStartPoint() const; // 696F50; distinguishes an unassigned random spot.

    wchar_t Name[20];
#ifdef _WIN32
    sockaddr_in Address;
#else
    // WinSock's 16-byte address representation, independent of the host's
    // sockaddr_in (BSD hosts place sin_len before sin_family).
    struct NetworkAddress {
        WORD sin_family;
        WORD sin_port;
        struct { DWORD S_addr; } sin_addr;
        char sin_zero[8];
    } Address;
#endif
    char Serial[19];
    int Country;
    int InitialCountry;
    int Color;
    int InitialColor;
    int StartPoint;
    int InitialStartPoint;
    int Team;
    int InitialTeam;
    DWORD SpectatorFlag; // 0xFFFFFFFF if Spectator
    int HouseIndex;
    int Time;
    DWORD unknown_int_77;
    int Clan;
    DWORD unknown_int_7F;
    BYTE unknown_byte_83;
    BYTE unknown_byte_84;
};
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma pack(pop)
static_assert(sizeof(NodeNameType) == 0x85);
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
#pragma pack(push, 4)
#endif
class SessionClass
{
public:
    /// Global VA: 0x00A8B238.
    DEFINE_REFERENCE(SessionClass, Instance, 0xA8B238u)

    // Original flags inside the opaque Session record, read by AddMessage.
    /// Global VA: 0x00A8D1F8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, AnimateLanMessages, 0xA8D1F8u)
    /// Global VA: 0x00A8D1F9.
    DEFINE_REFERENCE(bool, AnimateInternetMessages, 0xA8D1F9u)
#else
    static bool& AnimateLanMessages;
    /// Global VA: 0x00A8D1F9.
    static bool& AnimateInternetMessages;
#endif

    static bool IsCampaign()
    {
        return Instance.GameMode == GameMode::Campaign;
    }

    static bool IsSkirmish()
    {
        return Instance.GameMode == GameMode::Skirmish;
    }

    static bool IsSingleplayer()
    {
        return Instance.GameMode == GameMode::Campaign
            || Instance.GameMode == GameMode::Skirmish;
    }

    static bool IsMultiplayer()
    {
        return Instance.GameMode == GameMode::LAN
            || Instance.GameMode == GameMode::Internet;
    }

    // non-virtual
    /// VA: 0x00699980.
    void ReadScenarioDescriptions()
        { JMP_THIS(0x699980) }

    /// VA: 0x00697B70.
    bool CreateConnections()
        { JMP_THIS(0x697B70) }

    /// VA: 0x0069BAB0.
    void Resume()
        { JMP_THIS(0x69BAB0) }

    // Is the given house (or the local player, when null) the game's master/host?
    // Reads MasterPlayerID, falling back to MasterPlayerName, then to the first
    // non-defeated human house.
    /// VA: 0x00697E70.
    bool Am_I_Master(HouseClass* who = nullptr)
        { JMP_THIS(0x697E70) }

    // --- Fields the static layout above does not name, accessed by fixed offset.
    //     Offsets verified against gamemd.exe.

    // Non-zero while the session is suspended: the dialog message handler then
    // services Call_Back() instead of recursively running the main loop.
    int& Suspended()
        { return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + 0x287C); }

    // Non-zero while an in-game frame is running (set by the resume routine at
    // 0x69BAB0, cleared by the pause routine at 0x69BB40). The owner-draw painter
    // (OwnerDraw::Draw_Menu) checks this to choose the dialog backdrop: when set
    // it redraws the in-game sidebar behind a band-1 dialog, when clear it draws
    // the full multiplayer menu screen (mpyscrnl). Clear it to give an in-game
    // dialog the menu backdrop, and restore it afterwards.
    bool& InGameFrameActive()
        { return *reinterpret_cast<bool*>(reinterpret_cast<char*>(this) + 0x30D8); }

    // House index of the current game master/host, or -1 if none assigned.
    int& MasterPlayerID()
        { return *reinterpret_cast<int*>(reinterpret_cast<char*>(this) + 0x3074); }

    // UTF-16 name of the current game master/host (wchar_t[21]).
    wchar_t* MasterPlayerName()
        { return reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(this) + 0x3020); }

    // Drops a player's connection by house index: prints the "connection
    // lost" (error == 1) / "left game" (error == 0) message, deletes the IPX
    // connection, queues the remove-player event and reassigns the host.
    /// VA: 0x005DA750.
    static void YRPP_FASTCALL Destroy_Connection(int id, int error)
        { JMP_STD(0x5DA750) }

    // Incoming global-packet receive buffers, filled by
    // IPXManagerClass::Get_Global_Message. GlobalReceivePacket is the raw packet
    // (engine GlobalPacketType, 0x1C7 bytes; cast to the desired packet struct);
    // GlobalReceiveAddress is the sender's address.
    /// Global VA: 0x00A8D638.
    DEFINE_REFERENCE(char, GlobalReceivePacket, 0xA8D638)
    /// Global VA: 0x00A8D804.
    DEFINE_REFERENCE(IPXAddressClass, GlobalReceiveAddress, 0xA8D804)

    GameMode GameMode;
    MPGameModeClass* MPGameMode;
    DWORD unknown_08;
    DWORD unknown_0C;
    DWORD unknown_10;
    CommProtocolType CommProtocol;
    GameModeOptionsClass Config;
    DWORD UniqueID;
    char Handle[20];
    int PlayerColor;
    DWORD unknown_160;
    DWORD unknown_164;
    DWORD unknown_168;
    DWORD unknown_16C;
    DWORD unknown_170;
    int idxSide;
    int idxSide2;
    int Color;
    int Color2;
    int Side;
    int Side2;
    SessionOptionsClass Skirmish;
    SessionOptionsClass LAN;
    SessionOptionsClass WOL;
    BOOL MultiplayerObserver;
    DWORD Unknown_304;
    bool WOLLimitResolution;
    int LastNickSlot;
    int MPlayerMax;
    int MPlayerCount;
    int MaxAhead;
    int FrameSendRate;
    int DesiredFrameRate;
    int ProcessTimer;
    int ProcessTicks;
    int ProcessFrames;
    int MaxMaxAhead;
    int PrecalcMaxAhead;
    int PrecalcDesiredFrameRate;
    struct
    {
        char Name[64];
        int MaxRoundTrip;
        int Resends;
        int Lost;
        int PercentLost;
        int MaxAvgRoundTrip;
        int FrameSyncStalls;
        int CommandCoundStalls;
        IPXAddressClass Address;
    } MPStats[8];
    bool EnableMultiplayerDebug;
    bool DrawMPDebugStats;
    char field_67E;
    char field_67F;
    int LoadGame;
    int SaveGame;
    char field_688;
    bool SawCompletion;
    bool OutOfSync;
    char field_68B;
    int GameVersion;
    DynamicVectorClass<class MultiMission*> MultiMission;
    char ScenarioFilename[0x202]; // 0x6A8
    PROTECTED_PROPERTY(char, unknown_8AA[0x1AAA]);
    CCFileClass RecordFile;
    unsigned int Record  : 1;
    unsigned int Play    : 1;
    unsigned int Attract : 1;
    PROTECTED_PROPERTY(char, unknown_23C4[0x448]);
    DynamicVectorClass<NodeNameType*> unknown_vector_280C;
    DynamicVectorClass<NodeNameType*> unknown_vector_2824;
    DynamicVectorClass<NodeNameType*> StartSpots;
    PROTECTED_PROPERTY(DWORD, unknown_2854[0x221]);
    bool CurrentlyInGame; // at least used for deciding dialog backgrounds
};
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma pack(pop)
static_assert(sizeof(SessionClass) == 0x30DC);
#endif
