#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Randomizer.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/BasicStructures.h"
#include "yrpp/Timer.h"
#include "yrpp/Helpers/CompileTime.h"

class INIClass;
class CCINIClass;
class TechnoTypeClass;
struct IStream;

struct Variable
{
    char Name[40];
    char Value;
};

struct LightingStruct
{
    TintStruct Tint;
    int Ground; // Ground and Level use ini value * 1000 + 0.01.
    int Level; // Tint channels use ini value * 100 + 0.01.
};

struct ScenarioFlags
{
    bool bit00 : 1;
    bool bit01 : 1;
    bool bit02 : 1;
    bool bit03 : 1;
    bool CTFMode : 1; // the base for it does exist...
    bool Inert : 1;
    bool TiberiumGrows : 1;
    bool TiberiumSpreads : 1;

    bool MCVDeploy : 1;
    bool InitialVeteran : 1;
    bool FixedAlliance : 1;
    bool HarvesterImmune : 1;
    bool FogOfWar : 1;
    bool bit13 : 1;
    bool TiberiumExplosive : 1;
    bool DestroyableBridges : 1;

    bool Meteorites : 1;
    bool IonStorms : 1;
    bool Visceroids : 1;
    bool bit19 : 1;
    bool bit20 : 1;
    bool bit21 : 1;
    bool bit22 : 1;
    bool bit23 : 1;

    bool bit24 : 1;
    bool bit25 : 1;
    bool bit26 : 1;
    bool bit27 : 1;
    bool bit28 : 1;
    bool bit29 : 1;
    bool bit30 : 1;
    bool bit31 : 1;
};

class ScenarioClass
{
public:
    // Static
    static ScenarioClass*& Instance;
    static int& NewINIFormat;
    static TheaterType& LastTheater; // same storage as Theater::LastTheater
    static bool& WasGameSaved; // Original 0xABCE08; historical name.
    static int& PausedAudioVolume; // 83D834, default 0x4000; shared by Pause/Resume.

    /// VA: 0x004AE4C0.
    static void YRPP_FASTCALL UpdateCellLighting();

    /// VA: 0x0053C280.
    static void YRPP_FASTCALL UpdateLighting();

    // this function is only being inlined in RecalcLighting, but we can call it for just updating the hashpals
    /// VA: 0x0053AC80.
    static void YRPP_FASTCALL UpdateHashPalLighting(int R, int G, int B, bool tint);

    /// VA: 0x00555AC0.
    static void YRPP_FASTCALL ScenarioLighting(int* r, int* g, int* b);

    // this calls UpdateCellLighting() from above and does other good stuff
    // initializers call it with -1, -1, -1, 0 , map retint actions use current tint * 10, 0
    /// VA: 0x0053AD00.
    static void YRPP_FASTCALL RecalcLighting(int R, int G, int B, bool tint);

    /// VA: 0x0067CEF0.
    static bool YRPP_FASTCALL SaveGame(const char* FileName, const wchar_t* Description, bool BarGraph = false)
        { JMP_STD(0x67CEF0); }

    /// VA: 0x0067E440.
    static bool YRPP_FASTCALL LoadGame(const char* FileName)
        { JMP_STD(0x67E440); }

    // 683AB0 controller; the world loader is still an explicit runtime dependency.
    /// VA: 0x00683AB0.
    static bool YRPP_FASTCALL StartScenario(const char* FileName, bool Briefing, int CampaignIndex);
    /// VA: 0x00684620.
    static bool YRPP_FASTCALL ReadScenario(const char* filename); // 684620.
    /// VA: 0x00686730.
    static bool YRPP_FASTCALL ReadScenarioFile(const char* filename); // 686730.
    /// VA: 0x00684370.
    static bool YRPP_FASTCALL WaitForPlayers(); // 684370.
    /// VA: 0x00686B20.
    static bool YRPP_FASTCALL InitializeWorldINI(CCINIClass* ini, bool skip_units); // 686B20.
    /// VA: 0x006851F0.
    static int YRPP_FASTCALL ClearWorld(); // 6851F0; returns the map reset result.
    /// VA: 0x00534450.
    static int YRPP_FASTCALL DestroyWorldObjects(); // 534450; returns the final initialization depth.

    /// VA: 0x00683EB0.
    static void YRPP_FASTCALL PauseGame();

    /// VA: 0x00683FB0.
    static void YRPP_FASTCALL ResumeGame();

    /// VA: 0x00687F10.
    static void YRPP_FASTCALL AssignHouses();

    void ReadStartPoints(INIClass& ini);
    // 689E90: fields, special flags, lists, ranking, lighting and player setup.
    // World/INI registries must already be loaded by the outer startup sequence.
    /// VA: 0x00689E90.
    bool ReadINI(INIClass& ini);
    // Existing ReadINI lighting subsection, without object/UI initialization.
    // Missing keys retain the Scenario's current/default original values.
    bool ReadLightingINI(INIClass& ini) noexcept;
    // 68AD70: export current scene metadata. Multiplayer export omits the
    // player, movie and carryover fields that the original conditional skips.
    /// VA: 0x0068AD70.
    bool WriteINI(INIClass& ini, bool multiplayer);
    /// VA: 0x00683610.
    void Reset(); // 683610: reset per-scenario defaults and the three lists.
    void ClearWaypoints(); // 68BD60.
    /// VA: 0x0068BCB0.
    int CreateUniqueID(); // 68BCB0; increments with 32-bit wrap.
    /// VA: 0x00689670.
    char SetGlobal(int index, char value); // 689670, returns previous value.
    /// VA: 0x00689910.
    char SetLocal(int index, char value); // 689910, returns previous value.
    bool GetGlobal(int index, char* value); // 689760; failure preserves output.
    bool GetLocal(int index, char* value); // 689A00; failure preserves output.
    int FindGlobal(const char* name); // 689820; case-sensitive, -1 on miss.
    int FindLocal(const char* name); // 689AC0.
    /// VA: 0x00689670.
    char SetGlobal(const char* name, char value); // 6896C0; miss returns 1.
    /// VA: 0x00689910.
    char SetLocal(const char* name, char value); // 689960; miss returns 1.
    bool GetGlobal(const char* name, char* value); // 689790.
    bool GetLocal(const char* name, char* value); // 689A30.
    bool ReadGlobalVariables(INIClass& ini); // 689880: names only.
    /// VA: 0x00689B20
    bool ReadLocalVariables(INIClass& ini); // name, initial bool value.
    bool WriteLocalVariables(INIClass& ini); // 689C60.
    /// VA: 0x0068BDC0
    bool ReadWaypoints(INIClass& ini); // also marks the corresponding map cells.
    bool WriteWaypoints(INIClass& ini); // 68BE90.
    // 689310/689470: the Scenario record within a world stream. The record uses
    // the fixed x86 layout on every host. False reports incomplete transport,
    // invalid list counts, allocation or pointer registration failure.
    /// VA: 0x00689310.
    bool Save(IStream* stream);
    /// VA: 0x00689470.
    bool Load(IStream* stream);

    // valid range [0..701]
    /// VA: 0x0068BD80.
    bool IsDefinedWaypoint(int idx);

    CellStruct * GetWaypointCoords(CellStruct *dest, int idx);

    CellStruct GetWaypointCoords(int idx);

    // CTOR / DTOR
public:
    /// VA: 0x006832C0.
    ScenarioClass();
    ~ScenarioClass(); // Original normal exit inlines the three member destructors.
    ScenarioClass(const ScenarioClass&) = delete;
    ScenarioClass& operator=(const ScenarioClass&) = delete;

public:
    // Properties
    ScenarioFlags SpecialFlags;
    char NextScenario [0x104];
    char AltNextScenario [0x104];
    int HomeCell; //CellStruct?
    int AltHomeCell; //CellStruct?
    int UniqueID; //defaults to 1,000,000 - random salt for this game's communications
    Randomizer Random; //218
    DWORD Difficulty1;
    DWORD Difficulty2; // 2 - Difficulty1
    SysElapsedTimerClass ElapsedTimer;
    SysTimerClass PauseTimer;
    DWORD unknown_62C; // Nested logic-pause count, tested by 0x0055D826.
    bool IsGamePaused; // Timed release pending; 0x0055D7D0 tests PauseTimer.
    CellStruct Waypoints [702];

    // Map Header
    int StartX;
    int StartY;
    int Width;
    int Height;
    int NumberStartingPoints;
    Point2D StartingPoints [0x8];
    int HouseIndices [0x10]; // starting position => HouseClass::Array.GetItem(#)
    CellStruct HouseHomeCells [0x8];
    bool TeamsPresent;
    int NumCoopHumanStartSpots;
    CDTimerClass MissionTimer;
    wchar_t * MissionTimerTextCSF;
    char MissionTimerText [32];
    CDTimerClass ShroudRegrowTimer;
    CDTimerClass FogTimer;
    CDTimerClass IceTimer;
    CDTimerClass unknown_timer_123c;
    CDTimerClass AmbientTimer;
    int TechLevel;
    TheaterType Theater;
    char FileName [0x104];
    wchar_t Name [0x2D];
    char UIName [0x20];
    wchar_t UINameLoaded [0x2D];

    // Movies
    int Intro; // Movie registry indices; -1 means none (not filename pointers).
    int Brief;
    int Win;
    int Lose;
    int Action;
    int PostScore;
    int PreMapSelect;

    wchar_t Briefing [0x400];
    char BriefingCSF [0x20];
    int ThemeIndex;
    int HumanPlayerHouseTypeIndex;
    double CarryOverMoney;
    int CarryOverCap;
    int Percent;

    Variable GlobalVariables [50];
    Variable LocalVariables [100];

    CellStruct View1;
    CellStruct View2;
    CellStruct View3;
    CellStruct View4;
    DWORD unknown_34A0;
    bool FreeRadar; //34A4
    bool TrainCrate;
    bool TiberiumGrowthEnabled;
    bool VeinGrowthEnabled;
    bool IceGrowthEnabled; //34A8
    bool BridgeDestroyed; // RA1 leftover, no logic attached
    bool VariablesChanged; // global or local has been updated 34AA
    bool AmbientChanged; // ambient has been changed 34AB
    bool EndOfGame; //34AC
    bool TimerInherit;
    bool SkipScore;
    bool OneTimeOnly;
    bool SkipMapSelect;  //34B0
    bool TruckCrate;
    bool FillSilos;
    bool TiberiumDeathToVisceroid;
    bool IgnoreGlobalAITriggers; //34B4
    bool unknown_bool_34B5;
    bool unknown_bool_34B6;
    bool unknown_bool_34B7;
    int PlayerSideIndex; //34B8
    bool MultiplayerOnly; //34BC
    bool IsRandom;
    bool PickedUpAnyCrate;
    CDTimerClass unknown_timer_34C0;
    int CampaignIndex;
    int StartingDropships;
    TypeList<TechnoTypeClass*> AllowableUnits;
    TypeList<int> AllowableUnitMaximums;
    TypeList<int> DropshipUnitCounts;

    // General Lighting
    int AmbientOriginal; // set at map creation
    int AmbientCurrent; // current ambient
    int AmbientTarget; // target ambient (while changing)
    LightingStruct NormalLighting;

    // Ion lighting
    int IonAmbient;
    LightingStruct IonLighting;

    // Nuke flash lighting
    int NukeAmbient;
    LightingStruct NukeLighting;
    int NukeAmbientChangeRate;

    // Dominator lighting
    int DominatorAmbient;
    LightingStruct DominatorLighting;
    int DominatorAmbientChangeRate;

    DWORD unknown_3598;
    int InitTime;
    short Stage;
    bool UserInputLocked;
    bool unknown_35A3;
    int ParTimeEasy;
    int ParTimeMedium;
    int ParTimeDifficult;
    char UnderParTitle [0x1F]; //35B0
    char UnderParMessage [0x1F]; //35CF
    char OverParTitle [0x1F]; //35EE
    char OverParMessage [0x1F]; //360D
    char LSLoadMessage [0x1F]; //362C
    char LSBrief [0x1F]; //364B
    int LS640BriefLocX;
    int LS640BriefLocY;
    int LS800BriefLocX;
    int LS800BriefLocY;
    char LS640BkgdName [0x40];
    char LS800BkgdName [0x40];
    char LS800BkgdPal [0x40];
};

class MapSelectClass
{
public:
    /// VA: 0x005AE100.
    bool SetNextScenario(ScenarioClass* pItem)
        { JMP_THIS(0x5AE100); }
};
