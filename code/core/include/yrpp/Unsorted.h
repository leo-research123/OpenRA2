#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/Helpers/CompileTime.h"
#include "yrpp/Interfaces.h"
#include "yrpp/Timer.h"
#include <ctime>
#include <cstring>
struct TacticalSelectableStruct;
class SideClass;
class AbstractClass;
class ObjectClass;
class TechnoClass;
class PlanningTokenClass;
class PlanningNodeClass;
class EventClass;
class TargetClass;

#pragma pack(push, 4)
class MouseThreadClass {
public:
    bool IsInactive;
    int CallCount;
    bool IsStopped;
    int field_C;
    unsigned long long Interval;
    bool IsThreadCalled;
    HANDLE ThreadHandle;
    DWORD ThreadID;
};
#pragma pack(pop)

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(MouseThreadClass) == 0x24);
#endif

// things that I can't put into nice meaningful classes
struct INoticeSink;
class Game {
public:
    // Original process-lifetime elapsed system timer. Its clock reads the same
    // authority as all other native SystemTimer consumers; map reset retains it.
    /// Global VA: 0x00887338.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(SysElapsedTimerClass, TickCount, 0x887338u)
#else
    static SysElapsedTimerClass& TickCount;
#endif
    /// VA: 0x00730A90
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ShowMessage(const wchar_t* text, int duration = -1) { JMP_STD(0x730A90); }
#else
    static void YRPP_FASTCALL ShowMessage(const wchar_t* text, int duration = -1) noexcept;
#endif
    /// Global VA: 0x00ABCD3C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(LONG, COMReferenceCount, 0xABCD3Cu)
#else
    static LONG& COMReferenceCount;
#endif

    /// Global VA: 0x00B04BEC.
    DEFINE_REFERENCE(bool, IsReadingAudioFile, 0xB04BECu)

    /// Global VA: 0x00B78138.
    DEFINE_REFERENCE(MouseThreadClass, MouseThread, 0xB78138u)
    // the magic checksum for version validation - linked in StaticInits
    /// Global VA: 0x0083D560.
    DEFINE_REFERENCE(DWORD, Savegame_Magic, 0x83D560u)

    /// Global VA: 0x00B0BC88.
    DEFINE_REFERENCE(DynamicVectorClass<ULONG>, COMClasses, 0xB0BC88u)

    /// Global VA: 0x00B73550.
    DEFINE_REFERENCE(HWND, hWnd, 0xB73550u)
    /// Global VA: 0x00B732F0.
    DEFINE_REFERENCE(HINSTANCE, hInstance, 0xB732F0u)
    /// Global VA: 0x00B7355C.
    DEFINE_REFERENCE(HIMC, hIMC, 0xB7355Cu)

    /// Global VA: 0x00887418.
    DEFINE_REFERENCE(bool, bVPLRead, 0x887418u)
    /// Global VA: 0x00840A6C.
    DEFINE_REFERENCE(bool, bVideoBackBuffer, 0x840A6Cu)
    /// Global VA: 0x00A8EB96.
    DEFINE_REFERENCE(bool, bAllowVRAMSidebar, 0xA8EB96u)

    /// Global VA: 0x00A8D5F8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(RecordFlag, RecordingFlag, 0xA8D5F8u)
#else
    static RecordFlag& RecordingFlag;
#endif
    /// Global VA: 0x00A8D58C.
    DEFINE_REFERENCE(CCFileClass, RecordFile, 0xA8D58Cu)

    /// Global VA: 0x00822CF1.
    DEFINE_REFERENCE(bool, bDrawShadow, 0x822CF1u)
#if defined(RA2_YRPP_GAME)
    /// Global VA: 0x00B0FE58.
    DEFINE_REFERENCE(bool, AttackMoveMode, 0xB0FE58u)
#else
    static bool& AttackMoveMode;
#endif
    /// VA: 0x00731BF0
    static bool IsAttackMoveMode() noexcept;
    /// Global VA: 0x00B0FE54.
    static int& SelectionCommandMode;
    /// Global VA: 0x00B0FE64.
    static bool& TypeSelectionIncludesMap;
    /// Global VA: 0x00B0FE65.
    static bool& TypeSelectionActive;
    // Nullable list used by the original selection-cycling commands.
    /// Global VA: 0x00B0FE6C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TechnoClass*>*, SelectionCandidates, 0xB0FE6Cu)
#else
    static DynamicVectorClass<TechnoClass*>*& SelectionCandidates;
#endif
    /// VA: 0x00733160
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PointerGotInvalid(AbstractClass* object) { JMP_STD(0x733160); }
#else
    static void YRPP_FASTCALL PointerGotInvalid(AbstractClass* object) noexcept;
#endif
    /// Global VA: 0x00B0FE68.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DWORD, TypeSelectionStarted, 0xB0FE68u)
#else
    static DWORD& TypeSelectionStarted;
#endif
    /// VA: 0x00732CA0
    static void BeginTypeSelection() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x732CA0);
    }
#else
        ;
#endif
    /// VA: 0x00732CC0
    static void EndTypeSelection() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x732CC0);
    }
#else
        ;
#endif
    /// VA: 0x00732950
    static void TypeSelect() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x732950);
    }
#else
        ;
#endif
    /// VA: 0x00731AF0
    static void BeginAttackMove() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x731AF0);
    }
#else
        ;
#endif
    /// VA: 0x00731A30
    static void ToggleBeacon() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x731A30);
    }
#else
        ;
#endif
    /// VA: 0x00730F30
    static void CheerSelectedHouse() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730F30);
    }
#else
        ;
#endif
    /// VA: 0x00730990
    static bool YRPP_FASTCALL ControlGroupIsSelected(int group) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730990);
    }
#else
        ;
#endif
    /// VA: 0x00730A10
    static int YRPP_FASTCALL ControlGroupCount(int group) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730A10);
    }
#else
        ;
#endif
    /// VA: 0x00731060
    static void YRPP_FASTCALL AssignControlGroup(int group) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x731060);
    }
#else
        ;
#endif
    /// VA: 0x007310D0
    static void YRPP_FASTCALL ClearControlGroup(int group) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x7310D0);
    }
#else
        ;
#endif
    /// VA: 0x007311C0
    static void YRPP_FASTCALL SelectControlGroup(int group) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x7311C0);
    }
#else
        ;
#endif
    /// VA: 0x007313A0
    static void YRPP_FASTCALL CenterControlGroup(int group) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x7313A0);
    }
#else
        ;
#endif
    /// Global VA: 0x00845550.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DWORD, ControlGroupSelectedAt, 0x845550u)
    DEFINE_REFERENCE(int, LastControlGroup, 0x845554u)
    DEFINE_REFERENCE(int, LastCheerFrame, 0x84555Cu)
#else
    static DWORD& ControlGroupSelectedAt;
    static int& LastControlGroup;
    static int& LastCheerFrame;
#endif
    /// VA: 0x00730AF0
    static void DeploySelected() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730AF0);
    }
#else
        ;
#endif
    /// VA: 0x00730EA0
    static void StopSelected() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730EA0);
    }
#else
        ;
#endif
    /// VA: 0x00730D60
    static void GuardSelected() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730D60);
    }
#else
        ;
#endif
    /// VA: 0x00730FE0
    static void ScatterSelected() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x730FE0);
    }
#else
        ;
#endif
    /// VA: 0x00647040
    static bool QueueOptions() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x647040);
    }
#else
        ;
#endif
    /// VA: 0x0063A0D0
    static void ReportNoGuardArea() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x63A0D0);
    }
#else
        ;
#endif
    /// VA: 0x00731D00
    static void YRPP_FASTCALL SetSelectionCommandMode(int mode) noexcept;
    /// Global VA: 0x00B0FE5C.
    static TechnoClass* (&SidebarTabObjects)[2];
    /// VA: 0x00734250
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL SetSidebarTabObject(TechnoClass* object) { JMP_STD(0x734250); }
#else
    static void YRPP_FASTCALL SetSidebarTabObject(TechnoClass* object) noexcept;
#endif
    // Original permits a null ECX to clear both slots. A static entry avoids
    // calling a C++ member through null; a match clears only the first slot.
    /// VA: 0x00734270
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL ClearSidebarTabObject(const TechnoClass* object) { JMP_STD(0x734270); }
#else
    static int YRPP_FASTCALL ClearSidebarTabObject(const TechnoClass* object) noexcept;
#endif
    /// Global VA: 0x008A0DEF.
    DEFINE_REFERENCE(bool, bAllowDirect3D, 0x8A0DEFu)
    /// Global VA: 0x008A0DF0.
    DEFINE_REFERENCE(bool, bDirect3DIsUseable, 0x8A0DF0u)

    /// Global VA: 0x00A83D49.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, PlayerWins, 0xA83D49u)
    DEFINE_REFERENCE(bool, PlayerLoses, 0xA8ECD0u)
#else
    static bool& PlayerWins;
    static bool& PlayerLoses;
#endif
    // Single-player result request consumption from the outer loop at 0x0055CFD0.
    // Returns 0/1/2 for no result/victory/defeat; presentation belongs to the host.
    static int TakeScenarioResult();
    /// Global VA: 0x00A8E9A0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, IsActive, 0xA8E9A0u)
#else
    static bool& IsActive;
#endif
    /// Global VA: 0x00884B8F.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, SidebarChanged, 0x884B8Fu)
#else
    static bool& SidebarChanged;
#endif
    /// Global VA: 0x00A8ED80.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, IsFocused, 0xA8ED80u)
#else
    static bool& IsFocused;
#endif
    /// Global VA: 0x00A8EDA0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, SpecialDialog, 0xA8EDA0u)
#else
    static int& SpecialDialog;
#endif
    /// Global VA: 0x00AC48D4.
    DEFINE_REFERENCE(bool, PCXInitialized, 0xAC48D4)

    /// Global VA: 0x00A8ED94.
    DEFINE_REFERENCE(int, Seed, 0xA8ED94u)
    /// Global VA: 0x00822CF4.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, TechLevel, 0x822CF4u)
#else
    static int& TechLevel;
#endif
    /// Global VA: 0x00A8B54C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, PlayerCount, 0xA8B54Cu)
#else
    static int& PlayerCount;
#endif
    /// Global VA: 0x00A8B394.
    DEFINE_REFERENCE(int, PlayerColor, 0xA8B394u)
    /// Global VA: 0x00AC10C8.
    DEFINE_REFERENCE(bool, ObserverMode, 0xAC10C8u)
    DEFINE_POINTER(char, ScenarioName, 0xA8B8E0u)
    /// Global VA: 0x00A8F7AC.
    DEFINE_REFERENCE(bool, DontSetExceptionHandler, 0xA8F7ACu)
    /// Global VA: 0x00A8B8B4.
    DEFINE_REFERENCE(bool, EnableMPDebug, 0xA8B8B4u)
    /// Global VA: 0x00A8B8B5.
    DEFINE_REFERENCE(bool, DrawMPDebugStats, 0xA8B8B5u)
    /// Global VA: 0x00B04880.
    DEFINE_REFERENCE(bool, EnableMPSyncDebug, 0xB04880u)
    /// Global VA: 0x00B0FBB8.
    DEFINE_REFERENCE(bool, ScoreStuffLoad, 0xB0FBB8u)

    /// Global VA: 0x00887470.
    DEFINE_REFERENCE(Vector3D<float>, VoxelLightSource, 0x887470)
    /// Global VA: 0x00887420.
    DEFINE_REFERENCE(Vector3D<float>, VoxelShadowLightSource, 0x887420)

    /// Global VA: 0x00A8D108.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(byte, [8], ChatMask, 0xA8D108u);
#else
    static byte (&ChatMask)[8];
#endif

    static struct Network {
    public:
        /// Global VA: 0x00B779C4.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(int, Tournament, 0xB779C4u)
#else
        static int& Tournament;
#endif
        /// Global VA: 0x00B779D4.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(DWORD, WOLGameID, 0xB779D4u)
#else
        static DWORD& WOLGameID;
#endif
        /// Global VA: 0x00B77788.
        DEFINE_REFERENCE(time_t, PlanetWestwoodStartTime, 0xB77788u)
        /// Global VA: 0x00B73814.
        DEFINE_REFERENCE(int, GameStockKeepingUnit, 0xB73814u)
        /// Global VA: 0x00A8B24C.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(int, ProtocolVersion, 0xA8B24Cu)
#else
        static int& ProtocolVersion;
#endif
        /// Global VA: 0x00A8B554.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(int, FrameSendRate, 0xA8B554u)
#else
        static int& FrameSendRate;
#endif
        /// Global VA: 0x00A8B570.
        DEFINE_REFERENCE(int, PreCalcFrameRate, 0xA8B570u)
        /// Global VA: 0x0083737C.
        DEFINE_REFERENCE(int, ReconnectTimeout, 0x83737Cu)
        /// Global VA: 0x00A8B550.
        DEFINE_REFERENCE(int, MaxAhead, 0xA8B550u)
        /// Global VA: 0x00A8B568.
        DEFINE_REFERENCE(int, MaxMaxAhead, 0xA8B568u)
        /// Global VA: 0x00A8B56C.
        DEFINE_REFERENCE(int, PreCalcMaxAhead, 0xA8B56Cu)
        /// Global VA: 0x00A8DB9C.
        DEFINE_REFERENCE(int, LatencyFudge, 0xA8DB9Cu)
        /// Global VA: 0x00A8B558.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(int, RequestedFPS, 0xA8B558u)
#else
        static int& RequestedFPS;
#endif
        /// Global VA: 0x00A8B8C2.
        DEFINE_REFERENCE(bool, OutOfSync, 0xA8B8C2)

        /// VA: 0x005DA6C0.
        static bool Init() { JMP_STD(0x5DA6C0); }
    } Network;

    // the game's own rounding function
    // infamous for true'ing (F2I(-5.00) == -4.00)
    /// VA: 0x007C5F00; original conversion called by this wrapper.
    // clang-format off
    static long long F2I64(double val)
#ifdef _MSVC
    {
        double something = val;
        __asm { fld something };
        CALL(0x7C5F00);
    }
#else
    ; // Original x87 operation; no portable implementation is supplied here.
#endif
    // clang-format on

    // the game's own rounding function
    // infamous for true'ing (F2I(-5.00) == -4.00)
    static int F2I(double val) { return static_cast<int>(F2I64(val)); }

    /// VA: 0x007DC720.
    [[noreturn]] static void RaiseError(HRESULT err) { JMP_STD(0x7DC720); }

    // actually is SessionClass::Callback
    /// VA: 0x0069AE90.
    static void SetProgress(int progress) {
        SET_REG32(ECX, 0xA8B238);
        JMP_STD(0x69AE90);
    }

    // Per-iteration service pump of the main loop. Standalone builds run its audio
    // update (0x00406F70); the network and online-service parts do not exist there.
    /// VA: 0x0048D080.
#if defined(RA2_YRPP_GAME)
    static void CallBack() { JMP_STD(0x48D080); }
#else
    static void CallBack() noexcept;
#endif

    /// VA: 0x004A3B40.
    static int YRPP_FASTCALL GetResource(int ID, int Type) { JMP_STD(0x4A3B40); }

    /// VA: 0x00777080.
    static void YRPP_FASTCALL CenterWindowIn(HWND Child, HWND Parent) { JMP_STD(0x777080); }

    /// VA: 0x0053E420.
    static void YRPP_FASTCALL sub_53E420(HWND hWindow) { JMP_STD(0x53E420); }

    /// VA: 0x0053E3C0.
    static void YRPP_FASTCALL sub_53E3C0(HWND hWindow) { JMP_STD(0x53E3C0); }

    /// VA: 0x00776D80.
    static void YRPP_STDCALL OnWindowMoving(tagRECT* Rect) { JMP_STD(0x776D80); }

    // Existing PlanMgr module entries. They operate on the original selection,
    // tokens and members, and introduce no replacement planner object.
    /// VA: 0x637D00
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_SubmitDelete(PlanningNodeClass* node) {
        reinterpret_cast<void(YRPP_THISCALL*)(PlanningNodeClass*)>(0x637D00)(node);
    }
#else
    static void YRPP_FASTCALL PlanningManager_SubmitDelete(PlanningNodeClass* node) noexcept;
#endif
    /// VA: 0x637060
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_SetNoticeSink(INoticeSink* sink) {
        reinterpret_cast<void(YRPP_THISCALL*)(INoticeSink*)>(0x637060)(sink);
    }
#else
    static void YRPP_FASTCALL PlanningManager_SetNoticeSink(INoticeSink* sink) noexcept;
#endif
    /// VA: 0x637E00
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_Execute(const EventClass* event) {
        return reinterpret_cast<void(YRPP_THISCALL*)(const EventClass*)>(0x637E00)(event);
    }
#else
    static void YRPP_FASTCALL PlanningManager_Execute(const EventClass* event) noexcept;
#endif
    /// VA: 0x638120
#if defined(RA2_YRPP_GAME)
    static bool YRPP_FASTCALL PlanningManager_AddCommand(TechnoClass* unit, const EventClass* event) {
        return reinterpret_cast<bool(YRPP_FASTCALL*)(TechnoClass*, const EventClass*)>(0x638120)(unit, event);
    }
#else
    static bool YRPP_FASTCALL PlanningManager_AddCommand(TechnoClass* unit, const EventClass* event) noexcept;
#endif
    /// VA: 0x638C70
#if defined(RA2_YRPP_GAME)
    static bool YRPP_FASTCALL PlanningManager_IsReady(TechnoClass* unit) {
        return reinterpret_cast<bool(YRPP_THISCALL*)(TechnoClass*)>(0x638C70)(unit);
    }
#else
    static bool YRPP_FASTCALL PlanningManager_IsReady(TechnoClass* unit) noexcept;
#endif
    /// VA: 0x637550
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_FlushEvents() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x637550)(); }
#else
    static void YRPP_CDECL PlanningManager_FlushEvents() noexcept;
#endif
    /// VA: 0x637270
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_Update() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x637270)(); }
#else
    static void YRPP_CDECL PlanningManager_Update() noexcept;
#endif
    /// VA: 0x639740
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_MakeLoop(TechnoClass* unit, PlanningNodeClass* node) {
        return reinterpret_cast<void(YRPP_FASTCALL*)(TechnoClass*, PlanningNodeClass*)>(0x639740)(unit, node);
    }
#else
    static void YRPP_FASTCALL PlanningManager_MakeLoop(TechnoClass* unit, PlanningNodeClass* node) noexcept;
#endif
    /// VA: 0x639800
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_Join(TechnoClass* unit, PlanningNodeClass* node) {
        return reinterpret_cast<void(YRPP_FASTCALL*)(TechnoClass*, PlanningNodeClass*)>(0x639800)(unit, node);
    }
#else
    static void YRPP_FASTCALL PlanningManager_Join(TechnoClass* unit, PlanningNodeClass* node) noexcept;
#endif
    /// VA: 0x639BC0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_DeleteNode(PlanningNodeClass* node) {
        return reinterpret_cast<void(YRPP_THISCALL*)(PlanningNodeClass*)>(0x639BC0)(node);
    }
#else
    static void YRPP_FASTCALL PlanningManager_DeleteNode(PlanningNodeClass* node) noexcept;
#endif
    /// VA: 0x639CB0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_AdjustLoop(TechnoClass* unit) {
        return reinterpret_cast<void(YRPP_THISCALL*)(TechnoClass*)>(0x639CB0)(unit);
    }
#else
    static void YRPP_FASTCALL PlanningManager_AdjustLoop(TechnoClass* unit) noexcept;
#endif
    /// VA: 0x6379C0
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_Begin() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x6379C0)(); }
#else
    static void YRPP_CDECL PlanningManager_Begin() noexcept;
#endif
    /// VA: 0x637A10
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_End() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x637A10)(); }
#else
    static void YRPP_CDECL PlanningManager_End() noexcept;
#endif
    /// VA: 0x731A50
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL BeginPlanning(bool fromButton) {
        return reinterpret_cast<void(YRPP_THISCALL*)(bool)>(0x731A50)(fromButton);
    }
#else
    static void YRPP_FASTCALL BeginPlanning(bool fromButton) noexcept;
#endif
    /// VA: 0x731A70
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL EndPlanning(bool fromButton) {
        return reinterpret_cast<void(YRPP_THISCALL*)(bool)>(0x731A70)(fromButton);
    }
#else
    static void YRPP_FASTCALL EndPlanning(bool fromButton) noexcept;
#endif
    /// VA: 0x63A120
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_Introduction(bool fromButton, bool enabled) {
        return reinterpret_cast<void(YRPP_FASTCALL*)(bool, bool)>(0x63A120)(fromButton, enabled);
    }
#else
    static void YRPP_FASTCALL PlanningManager_Introduction(bool fromButton, bool enabled) noexcept;
#endif
    /// VA: 0x63A1B0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_Help(int index) {
        return reinterpret_cast<void(YRPP_THISCALL*)(int)>(0x63A1B0)(index);
    }
#else
    static void YRPP_FASTCALL PlanningManager_Help(int index) noexcept;
#endif
    /// VA: 0x63A230
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_Report(const char* label) {
        return reinterpret_cast<void(YRPP_THISCALL*)(const char*)>(0x63A230)(label);
    }
#else
    static void YRPP_FASTCALL PlanningManager_Report(const char* label) noexcept;
#endif
    /// VA: 0x638E60
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_UpdateHover() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x638E60)(); }
#else
    static void YRPP_CDECL PlanningManager_UpdateHover() noexcept;
#endif
    /// VA: 0x63AC70
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL PlanningManager_CheckNode(PlanningNodeClass* node) {
        return reinterpret_cast<int(YRPP_THISCALL*)(PlanningNodeClass*)>(0x63AC70)(node);
    }
#else
    static int YRPP_FASTCALL PlanningManager_CheckNode(PlanningNodeClass* node) noexcept;
#endif
    /// VA: 0x63AD50
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_ClickNode() { reinterpret_cast<void(YRPP_CDECL*)()>(0x63AD50)(); }
#else
    static void YRPP_CDECL PlanningManager_ClickNode() noexcept;
#endif
    /// VA: 0x63A5A0
#if defined(RA2_YRPP_GAME)
    static bool YRPP_STDCALL PlanningManager_LeftDown(Point2D point) {
        return reinterpret_cast<bool(YRPP_STDCALL*)(Point2D)>(0x63A5A0)(point);
    }
#else
    static bool YRPP_STDCALL PlanningManager_LeftDown(Point2D point) noexcept;
#endif
    /// VA: 0x63A8E0
#if defined(RA2_YRPP_GAME)
    static bool YRPP_STDCALL PlanningManager_LeftUp(Point2D point) {
        return reinterpret_cast<bool(YRPP_STDCALL*)(Point2D)>(0x63A8E0)(point);
    }
#else
    static bool YRPP_STDCALL PlanningManager_LeftUp(Point2D point) noexcept;
#endif
    /// VA: 0x63AAC0
#if defined(RA2_YRPP_GAME)
    static void YRPP_STDCALL PlanningManager_RightDown(Point2D point) {
        return reinterpret_cast<void(YRPP_STDCALL*)(Point2D)>(0x63AAC0)(point);
    }
#else
    static void YRPP_STDCALL PlanningManager_RightDown(Point2D point) noexcept;
#endif
    /// VA: 0x63B0A0
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_DrawUnderlay() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x63B0A0)(); }
#else
    static void YRPP_CDECL PlanningManager_DrawUnderlay() noexcept;
#endif
    /// VA: 0x63B150
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_DrawOverlay() { return reinterpret_cast<void(YRPP_CDECL*)()>(0x63B150)(); }
#else
    static void YRPP_CDECL PlanningManager_DrawOverlay() noexcept;
#endif
    /// VA: 0x0063B2F0
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL PlanningManager_DrawFeedback() { reinterpret_cast<void(YRPP_CDECL*)()>(0x63B2F0)(); }
#else
    static void YRPP_CDECL PlanningManager_DrawFeedback() noexcept;
#endif
    // Original PlanMgr feedback state; no exceptions cross these entries.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, PlanningPendingError, 0xAC4C90)
    DEFINE_REFERENCE(bool, PlanningLoopFeedback, 0xAC4CC8)
    DEFINE_REFERENCE(bool, PlanningJoinFeedback, 0xAC4CD0)
    DEFINE_ARRAY_REFERENCE(int, [5], PlanningHelpCounts, 0xAC4BF4)
    DEFINE_REFERENCE(PlanningNodeClass*, PlanningBlockedNode, 0xAC4C3C)
#else
    static int& PlanningPendingError;
    static bool& PlanningLoopFeedback;
    static bool& PlanningJoinFeedback;
    static int (&PlanningHelpCounts)[5];
    static PlanningNodeClass*& PlanningBlockedNode;
#endif
    /// VA: 0x006375B0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_QueueEvent(const EventClass* event) {
        reinterpret_cast<void(YRPP_THISCALL*)(const EventClass*)>(0x6375B0)(event);
    }
#else
    static void YRPP_FASTCALL PlanningManager_QueueEvent(const EventClass* event) noexcept;
#endif
    /// Global VA: 0x00AC4B48
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<EventClass*>, PlanningEvents, 0xAC4B48)
#else
    static DynamicVectorClass<EventClass*>& PlanningEvents;
#endif
    /// VA: 0x006339B0
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL PlanningManager_OwnerIndex(const TechnoClass* unit) {
        return reinterpret_cast<int(YRPP_THISCALL*)(const TechnoClass*)>(0x6339B0)(unit);
    }
#else
    static int YRPP_FASTCALL PlanningManager_OwnerIndex(const TechnoClass* unit) noexcept;
#endif
    /// VA: 0x00639F80
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_RemoveHouseMember(unsigned house) {
        reinterpret_cast<void(YRPP_FASTCALL*)(unsigned)>(0x639F80)(house);
    }
#else
    static void YRPP_FASTCALL PlanningManager_RemoveHouseMember(unsigned house) noexcept;
#endif
    // This original entry also accepts null ECX; the member convenience
    // wrapper cannot represent a null C++ object invocation.
    /// VA: 0x006386E0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_ClearToken(TechnoClass* unit, const EventClass* event) {
        reinterpret_cast<void(YRPP_FASTCALL*)(TechnoClass*, const EventClass*)>(0x6386E0)(unit, event);
    }
#else
    static void YRPP_FASTCALL PlanningManager_ClearToken(TechnoClass* unit, const EventClass* event) noexcept;
#endif
    /// Global VA: 0x00AC4C40.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TechnoClass*>, PlanningUnits, 0xAC4C40)
    /// Global VA: 0x00AC4CF5.
    DEFINE_REFERENCE(bool, PlanningDeletingAll, 0xAC4CF5)
#else
    static DynamicVectorClass<TechnoClass*>& PlanningUnits;
    /// Global VA: 0x00AC4CF5.
    static bool& PlanningDeletingAll;
#endif
    /// VA: 0x00637640
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL PlanningManager_UnregisterNode(PlanningNodeClass* node) {
        JMP_STD(0x637640);
    }
#else
    static void YRPP_FASTCALL PlanningManager_UnregisterNode(PlanningNodeClass* node) noexcept;
#endif
    /// VA: 0x006378B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL PlanningManager_FlagNode(PlanningNodeClass* node) {
        JMP_STD(0x6378B0);
    }
#else
    static void YRPP_FASTCALL PlanningManager_FlagNode(PlanningNodeClass* node) noexcept;
#endif
    /// Global VA: 0x00AC4CCC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(PlanningNodeClass*, PlanningNodeAtAC4CCC, 0xAC4CCC)
    /// Global VA: 0x00AC4C38.
    DEFINE_REFERENCE(PlanningNodeClass*, PlanningNodeAtAC4C38, 0xAC4C38)
    /// Global VA: 0x00AC4BF0.
    DEFINE_REFERENCE(PlanningNodeClass*, PlanningNodeAtAC4BF0, 0xAC4BF0)
#else
    static PlanningNodeClass*& PlanningNodeAtAC4CCC;
    /// Global VA: 0x00AC4C38.
    static PlanningNodeClass*& PlanningNodeAtAC4C38;
    /// Global VA: 0x00AC4BF0.
    static PlanningNodeClass*& PlanningNodeAtAC4BF0;
#endif
    /// Global VA: 0x00AC4CB0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, PlanningInputBlocked, 0xAC4CB0)
#else
    static bool& PlanningInputBlocked;
#endif
    /// VA: 0x0063AB60
#if defined(RA2_YRPP_GAME)
    static bool YRPP_STDCALL PlanningManager_UpdateCursor(int x, int y) { JMP_STD(0x63AB60); }
#else
    static bool YRPP_STDCALL PlanningManager_UpdateCursor(int x, int y);
#endif
    /// VA: 0x00633BC0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static CoordStruct* YRPP_FASTCALL PlanningManager_EventCoords(CoordStruct* output,
                                                                                       const EventClass* event) {
        JMP_STD(0x633BC0);
    }
#else
    static CoordStruct* YRPP_FASTCALL PlanningManager_EventCoords(CoordStruct* output,
                                                                  const EventClass* event) noexcept;
#endif
    /// VA: 0x00638B70
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static bool YRPP_FASTCALL PlanningManager_IsLocalGuard(TechnoClass* unit, Mission mission,
                                                                                TargetClass target);
#else
    static bool YRPP_FASTCALL PlanningManager_IsLocalGuard(TechnoClass* unit, Mission mission,
                                                           TargetClass target) noexcept;
#endif
    // -1 allowed, 0 unsupported, 1 follows a terminating command, 2 follows a continual command.
    /// VA: 0x00638CE0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static int YRPP_FASTCALL PlanningManager_CheckCommand(TechnoClass* unit,
                                                                               const EventClass* event) {
        JMP_STD(0x638CE0);
    }
#else
    static int YRPP_FASTCALL PlanningManager_CheckCommand(TechnoClass* unit, const EventClass* event) noexcept;
#endif
    // Returns the original low byte: non-MegaMission types are returned intact.
    /// VA: 0x00637DD0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static BYTE YRPP_STDCALL PlanningManager_Submit(EventClass event);
#else
    static BYTE YRPP_STDCALL PlanningManager_Submit(EventClass event) noexcept;
#endif
    // The caller ignores the original incidental final sound return value.
    /// VA: 0x00639FD0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL PlanningManager_RejectEvent(const EventClass* event) {
        JMP_STD(0x639FD0);
    }
#else
    static void YRPP_FASTCALL PlanningManager_RejectEvent(const EventClass* event) noexcept;
#endif
    /// VA: 0x00637840
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlanningManager_Select(TechnoClass* unit) noexcept { JMP_STD(0x637840); }
    /// Global VA: 0x00AC4CF6.
    DEFINE_REFERENCE(bool, PlanningSelectionActive, 0xAC4CF6u)
    /// VA: 0x006370B0
    static void YRPP_FASTCALL PlanningManager_DeleteAll() noexcept { JMP_STD(0x6370B0); }
#else
    static void YRPP_FASTCALL PlanningManager_Select(TechnoClass* unit) noexcept;
    /// VA: 0x006370B0
    static void YRPP_FASTCALL PlanningManager_DeleteAll() noexcept;
    /// Global VA: 0x00AC4CF6.
    static bool& PlanningSelectionActive;
#endif
    /// Global VA: 0x00AC4C08.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, PlanningErrorReported, 0xAC4C08)
    /// Global VA: 0x00AC4B84.
    DEFINE_ARRAY_REFERENCE(int, [24], PlanningMemberCounts, 0xAC4B84)
#else
    static bool& PlanningErrorReported;
    /// Global VA: 0x00AC4B84.
    static int (&PlanningMemberCounts)[24];
#endif
    /// VA: 0x00639040
    static bool YRPP_CDECL PlanningManager_CheckSelection() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x639040);
    }
#else
        ;
#endif
    /// VA: 0x00639130
    static bool YRPP_CDECL PlanningManager_CheckCapacity() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x639130);
    }
#else
        ;
#endif
    /// VA: 0x00639E30
    static bool YRPP_FASTCALL PlanningManager_CompatibleTokens(const PlanningTokenClass* first,
                                                               const PlanningTokenClass* second) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x639E30);
    }
#else
        ;
#endif
    // -1 means every selected object supports waypoints; otherwise the first
    // unsupported object is aircraft=0, building=1, or other/null=2.
    /// VA: 0x00639DA0
    static int YRPP_CDECL PlanningManager_UnsupportedType() noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x639DA0);
    }
#else
        ;
#endif
    /// VA: 0x0063AB00.
    static void YRPP_STDCALL PlanningManager_WM_RBUTTONUP_63AB00(Point2D XY)
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x63AB00);
    }
#else
        ;
#endif

    /// VA: 0x006805F0.
    static HRESULT YRPP_FASTCALL Save_Sides(IStream* pStm, DynamicVectorClass<SideClass*>* pVector) {
        JMP_STD(0x6805F0);
    }

    /// VA: 0x0053E6B0.
    static void StreamerThreadFlush() { JMP_STD(0x53E6B0); }

    /// VA: 0x007327D0.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL UICommands_TypeSelect_7327D0(const char* iniName) { JMP_STD(0x7327D0); }
#else
    static void YRPP_FASTCALL UICommands_TypeSelect_7327D0(const char* iniName) noexcept;
#endif
    /// VA: 0x00732600
    static void YRPP_FASTCALL UICommands_TypeDeselect(const char* iniName) noexcept;

    /// VA: 0x00732D00.
#if defined(RA2_YRPP_GAME)
    static bool IsTypeSelecting() { JMP_STD(0x732D00); }
#else
    static bool IsTypeSelecting() noexcept;
#endif

    /// VA: 0x0048ACF0.
    static double GetFloaterGravity() { JMP_STD(0x48ACF0); }

    // One game iteration. True if the game is inactive, or a win, loss, restart
    // or abort request ended this iteration. The outer loop tests AL at 0x0048CE8F
    // before invoking its end-game handler.
    /// VA: 0x0055D360
#if defined(RA2_YRPP_GAME)
    static bool YRPP_FASTCALL MainLoop() { JMP_STD(0x55D360); }
#else
    static bool YRPP_FASTCALL MainLoop();
#endif

    // Turns one queued key into a command, an Options event or a held scroll arrow.
    // Zero means no key. Bit 0x800 marks a release, 0x100/0x200/0x400 the modifiers.
    /// VA: 0x0055DEE0.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL KeyboardProcess(DWORD& input) { JMP_STD(0x55DEE0); }
#else
    static void YRPP_FASTCALL KeyboardProcess(DWORD& input) noexcept;
#endif

    // First step of KeyboardProcess: chat and message-line editing (0x55E420).
    /// VA: 0x0055E420.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ProcessMessageInput(DWORD& input) { JMP_STD(0x55E420); }
#else
    static void YRPP_FASTCALL ProcessMessageInput(DWORD& input) noexcept;
#endif

    // Scroll arrows currently held by KeyboardProcess: 0x100 left, 0x1000 right,
    // 0x1 up, 0x10 down. MainLoop scrolls the map by them every iteration.
    /// Global VA: 0x00ABCE14.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DWORD, KeyboardScrollFlags, 0xABCE14u)
#else
    static DWORD& KeyboardScrollFlags;
#endif

    /// VA: 0x004093B0.
    static LARGE_INTEGER YRPP_FASTCALL AudioGetTime() { JMP_STD(0x4093B0); }

    /// VA: 0x0052FC20.
    static void InitRandom() { JMP_STD(0x52FC20); }

    /// VA: 0x0048C8B0.
    static void ShowSpecialDialog() { JMP_STD(0x48C8B0); }

    /// VA: 0x00755C50.
    static void YRPP_FASTCALL DestroyVoxelCaches() { JMP_STD(0x755C50); }

    static void InitUIStuff() {
        CALL(0x600560);  // InitCommonDialogStuff()

        if (!PCXInitialized) {
            PCXInitialized = true;
            CALL(0x61F190);  // InitUIColorShifts()
            CALL(0x61F210);  // LoadPCXFiles()
        }
    }

    /// VA: 0x00456980
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL DrawRadialIndicator(bool drawLine, bool adjustColor, const CoordStruct pCoord,
                                                  ColorStruct color, float lineMultiplier, bool unknown1,
                                                  bool unknown2) {
        JMP_STD(0x456980);
    }
#else
    static void YRPP_FASTCALL DrawRadialIndicator(bool drawLine, bool adjustColor, CoordStruct coords,
                                                  ColorStruct color, float radius, bool pixels, bool truncate);
#endif

    /// VA: 0x5BF260
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlayMovie(int index, int reserved = -1, char clearAfter = 1, char stretch = 1,
                                        char clearBefore = 1) {
        JMP_STD(0x5BF260);
    }
#else
    static void YRPP_FASTCALL PlayMovie(int index, int reserved = -1, char clearAfter = 1, char stretch = 1,
                                        char clearBefore = 1);
#endif

    /// VA: 0x005BED40
    static void YRPP_FASTCALL PlayMovie(const char* movieName, int queue_theme = -1,
                                        char clear_after_playback = -1,  // == 1: clear/present black after playback.
                                        char stretch_movie = -1,
                                        char clear_before_playback = -1,  // != 0: clear/present black before playback.
                                        char set_state_1 = -1);

    /// VA: 0x0064DAB0.
    static void YRPP_FASTCALL ComputeFrameCRC() { JMP_STD(0x64DAB0); }

    /// VA: 0x00650A90.
    static void YRPP_FASTCALL LogFrameCRC(int frameIndex) { JMP_STD(0x650A90); }

    /// VA: 0x006C87F0.
    static void RegisterGameStartTime() { JMP_STD(0x6C87F0); }

    /// VA: 0x006C8820.
    static void RegisterGameEndTime() { JMP_STD(0x6C8820); }

    /// VA: 0x006C6F50.
    static void SendStatisticsPacket() { JMP_STD(0x6C6F50); }

    /// VA: 0x005C0980.
    static bool YRPP_FASTCALL IsMoviePlaying() { JMP_STD(0x5C0980); }

    /// VA: 0x005C09A0.
    static void YRPP_FASTCALL BlitMovie() { JMP_STD(0x5C09A0); }
};

// this fake class contains the IIDs used by the game
// no comments because the IIDs suck
class IIDs {
public:
    /// Global VA: 0x007F7C90.
    DEFINE_REFERENCE(IID const, IUnknown, 0x7F7C90u)
    /// Global VA: 0x007F7C80.
    DEFINE_REFERENCE(IID const, IPersistStream, 0x7F7C80u)
    /// Global VA: 0x007F7C70.
    DEFINE_REFERENCE(IID const, IPersist, 0x7F7C70u)
    /// Global VA: 0x007E9AE0.
    DEFINE_REFERENCE(IID const, IRTTITypeInfo, 0x7E9AE0u)
    /// Global VA: 0x007EA768.
    DEFINE_REFERENCE(IID const, IHouse, 0x7EA768u)
    /// Global VA: 0x007E9B00.
    DEFINE_REFERENCE(IID const, IPublicHouse, 0x7E9B00u)
    /// Global VA: 0x007F7CB0.
    DEFINE_REFERENCE(IID const, IEnumConnections, 0x7F7CB0u)
    /// Global VA: 0x007F7CC0.
    DEFINE_REFERENCE(IID const, IConnectionPoint, 0x7F7CC0u)
    /// Global VA: 0x007F7CD0.
    DEFINE_REFERENCE(IID const, IConnectionPointContainer, 0x7F7CD0u)
    /// Global VA: 0x007F7CE0.
    DEFINE_REFERENCE(IID const, IEnumConnectionPoints, 0x7F7CE0u)
    /// Global VA: 0x007E36C0.
    DEFINE_REFERENCE(IID const, IApplication, 0x7E36C0u)
    /// Global VA: 0x007EA6E8.
    DEFINE_REFERENCE(IID const, IGameMap, 0x7EA6E8u)
    /// Global VA: 0x007ED358.
    DEFINE_REFERENCE(IID const, ILocomotion, 0x7ED358u)
    /// Global VA: 0x007E9B10.
    DEFINE_REFERENCE(IID const, IPiggyback, 0x7E9B10u)
    /// Global VA: 0x007E9B40.
    DEFINE_REFERENCE(IID const, IFlyControl, 0x7E9B40u)
    /// Global VA: 0x007E9B20.
    DEFINE_REFERENCE(IID const, ISwizzle, 0x7E9B20u)
};

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)  // suppress deprecated warning for reference

// this class links to functions gamemd imports
// to avoid having to link to their DLLs ourselves
class Imports {
#define ALIAS(Obj, Addr) DEFINE_REFERENCE(FP_##Obj const, Obj, Addr)

public:
    // OleLoadFromStream
    typedef HRESULT(YRPP_STDCALL* FP_OleSaveToStream)(LPPERSISTSTREAM pPStm, LPSTREAM pStm);
    ALIAS(OleSaveToStream, 0x7E15F4);

    typedef HRESULT(YRPP_STDCALL* FP_OleLoadFromStream)(LPSTREAM pStm, const IID* const iidInterface, LPVOID* ppvObj);
    ALIAS(OleLoadFromStream, 0x7E15F8);

    typedef HRESULT(YRPP_STDCALL* FP_CoRegisterClassObject)(const IID& rclsid, LPUNKNOWN pUnk, DWORD dwClsContext,
                                                            DWORD flags, LPDWORD lpdwRegister);
    ALIAS(CoRegisterClassObject, 0x7E15D8);

    typedef HRESULT(YRPP_STDCALL* FP_CoRevokeClassObject)(DWORD dwRegister);
    ALIAS(CoRevokeClassObject, 0x7E15CC);

    typedef DWORD(YRPP_STDCALL* FP_TimeGetTime)();
    ALIAS(TimeGetTime, 0x7E1530);

    /* user32.dll */
    typedef LRESULT(YRPP_STDCALL* FP_DefWindowProcA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    ALIAS(DefWindowProcA, 0x7E1394);

    typedef BOOL(YRPP_STDCALL* FP_MoveWindow)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
    ALIAS(MoveWindow, 0x7E1398);

    typedef BOOL(YRPP_STDCALL* FP_GetUpdateRect)(HWND hWnd, LPRECT lpRect, BOOL bErase);
    ALIAS(GetUpdateRect, 0x7E139C);

    typedef HWND (*FP_GetFocus)(void);
    ALIAS(GetFocus, 0x7E13A0);

    typedef HDC(YRPP_STDCALL* FP_GetDC)(HWND hWnd);
    ALIAS(GetDC, 0x7E13A4);

    typedef SHORT(YRPP_STDCALL* FP_GetKeyState)(int nVirtKey);
    ALIAS(GetKeyState, 0x7E13A8);

    typedef HWND (*FP_GetActiveWindow)(void);
    ALIAS(GetActiveWindow, 0x7E13AC);

    typedef HWND (*FP_GetCapture)(void);
    ALIAS(GetCapture, 0x7E13B0);

    typedef int(YRPP_STDCALL* FP_GetDlgCtrlID)(HWND hWnd);
    ALIAS(GetDlgCtrlID, 0x7E13B4);

    typedef HWND(YRPP_STDCALL* FP_ChildWindowFromPointEx)(HWND, POINT, UINT);
    ALIAS(ChildWindowFromPointEx, 0x7E13B8);

    typedef BOOL(YRPP_STDCALL* FP_GetWindowRect)(HWND hWnd, LPRECT lpRect);
    ALIAS(GetWindowRect, 0x7E13BC);

    typedef BOOL(YRPP_STDCALL* FP_GetCursorPos)(LPPOINT lpPoint);
    ALIAS(GetCursorPos, 0x7E13C0);

    typedef BOOL(YRPP_STDCALL* FP_CloseWindow)(HWND hWnd);
    ALIAS(CloseWindow, 0x7E13C4);

    typedef BOOL(YRPP_STDCALL* FP_EndDialog)(HWND hDlg, int nResult);
    ALIAS(EndDialog, 0x7E13C8);

    typedef HWND(YRPP_STDCALL* FP_SetFocus)(HWND hWnd);
    ALIAS(SetFocus, 0x7E13CC);

    typedef BOOL(YRPP_STDCALL* FP_SetDlgItemTextA)(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
    ALIAS(SetDlgItemTextA, 0x7E13D0);

    typedef int(YRPP_STDCALL* FP_DialogBoxParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent,
                                                  DLGPROC lpDialogFunc, LPARAM dwInitParam);
    ALIAS(DialogBoxParamA, 0x7E13D4);

#ifdef _MSVC
    typedef int(YRPP_STDCALL* FP_DialogBoxIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate,
                                                          HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    ALIAS(DialogBoxIndirectParamA, 0x7E13D8);
#endif

    typedef int(YRPP_STDCALL* FP_ShowCursor)(BOOL bShow);
    ALIAS(ShowCursor, 0x7E13DC);

    typedef SHORT(YRPP_STDCALL* FP_GetAsyncKeyState)(int vKey);
    ALIAS(GetAsyncKeyState, 0x7E13E0);

    typedef int(YRPP_STDCALL* FP_ToAscii)(UINT uVirtKey, UINT uScanCode, PBYTE lpKeyState, LPWORD lpChar, UINT uFlags);
    ALIAS(ToAscii, 0x7E13E4);

    typedef UINT(YRPP_STDCALL* FP_MapVirtualKeyA)(UINT uCode, UINT uMapType);
    ALIAS(MapVirtualKeyA, 0x7E13E8);

    typedef int(YRPP_STDCALL* FP_GetSystemMetrics)(int nIndex);
    ALIAS(GetSystemMetrics, 0x7E13EC);

    typedef BOOL(YRPP_STDCALL* FP_SetWindowPos)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy,
                                                UINT uFlags);
    ALIAS(SetWindowPos, 0x7E13F0);

    typedef BOOL(YRPP_STDCALL* FP_DestroyWindow)(HWND hWnd);
    ALIAS(DestroyWindow, 0x7E13F4);

    typedef BOOL (*FP_ReleaseCapture)(void);
    ALIAS(ReleaseCapture, 0x7E13F8);

    typedef HWND(YRPP_STDCALL* FP_SetCapture)(HWND hWnd);
    ALIAS(SetCapture, 0x7E13FC);

    typedef BOOL(YRPP_STDCALL* FP_AdjustWindowRectEx)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
    ALIAS(AdjustWindowRectEx, 0x7E1400);

    typedef HMENU(YRPP_STDCALL* FP_GetMenu)(HWND hWnd);
    ALIAS(GetMenu, 0x7E1404);

    typedef BOOL(YRPP_STDCALL* FP_AdjustWindowRect)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
    ALIAS(AdjustWindowRect, 0x7E1408);

    typedef DWORD(YRPP_STDCALL* FP_GetSysColor)(int nIndex);
    ALIAS(GetSysColor, 0x7E140C);

    typedef UINT(YRPP_STDCALL* FP_IsDlgButtonChecked)(HWND hDlg, int nIDButton);
    ALIAS(IsDlgButtonChecked, 0x7E1410);

    typedef BOOL(YRPP_STDCALL* FP_CheckDlgButton)(HWND hDlg, int nIDButton, UINT uCheck);
    ALIAS(CheckDlgButton, 0x7E1414);

    typedef DWORD(YRPP_STDCALL* FP_WaitForInputIdle)(HANDLE hProcess, DWORD dwMilliseconds);
    ALIAS(WaitForInputIdle, 0x7E1418);

    typedef HWND(YRPP_STDCALL* FP_GetTopWindow)(HWND hWnd);
    ALIAS(GetTopWindow, 0x7E141C);

    typedef HWND (*FP_GetForegroundWindow)(void);
    ALIAS(GetForegroundWindow, 0x7E1420);

    typedef HICON(YRPP_STDCALL* FP_LoadIconA)(HINSTANCE hInstance, LPCSTR lpIconName);
    ALIAS(LoadIconA, 0x7E1424);

    typedef HWND(YRPP_STDCALL* FP_SetActiveWindow)(HWND hWnd);
    ALIAS(SetActiveWindow, 0x7E1428);

    typedef BOOL(YRPP_STDCALL* FP_RedrawWindow)(HWND hWnd, const RECT* lprcUpdate, HRGN hrgnUpdate, UINT flags);
    ALIAS(RedrawWindow, 0x7E142C);

    typedef DWORD(YRPP_STDCALL* FP_GetWindowContextHelpId)(HWND);
    ALIAS(GetWindowContextHelpId, 0x7E1430);

    typedef BOOL(YRPP_STDCALL* FP_WinHelpA)(HWND hWndMain, LPCSTR lpszHelp, UINT uCommand, DWORD dwData);
    ALIAS(WinHelpA, 0x7E1434);

    typedef HWND(YRPP_STDCALL* FP_ChildWindowFromPoint)(HWND hWndParent, POINT Point);
    ALIAS(ChildWindowFromPoint, 0x7E1438);

    typedef HCURSOR(YRPP_STDCALL* FP_LoadCursorA)(HINSTANCE hInstance, LPCSTR lpCursorName);
    ALIAS(LoadCursorA, 0x7E143C);

    typedef HCURSOR(YRPP_STDCALL* FP_SetCursor)(HCURSOR hCursor);
    ALIAS(SetCursor, 0x7E1440);

    typedef void(YRPP_STDCALL* FP_PostQuitMessage)(int nExitCode);
    ALIAS(PostQuitMessage, 0x7E1444);

    typedef HWND(YRPP_STDCALL* FP_FindWindowA)(LPCSTR lpClassName, LPCSTR lpWindowName);
    ALIAS(FindWindowA, 0x7E1448);

    typedef BOOL(YRPP_STDCALL* FP_SetCursorPos)(int X, int Y);
    ALIAS(SetCursorPos, 0x7E144C);

#ifdef _MSVC
    typedef HWND(YRPP_STDCALL* FP_CreateDialogIndirectParamA)(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate,
                                                              HWND hWndParent, DLGPROC lpDialogFunc,
                                                              LPARAM dwInitParam);
    ALIAS(CreateDialogIndirectParamA, 0x7E1450);
#endif

    typedef int(YRPP_STDCALL* FP_GetKeyNameTextA)(LONG lParam, LPSTR lpString, int nSize);
    ALIAS(GetKeyNameTextA, 0x7E1454);

    typedef BOOL(YRPP_STDCALL* FP_ScreenToClient)(HWND hWnd, LPPOINT lpPoint);
    ALIAS(ScreenToClient, 0x7E1458);

    typedef BOOL(YRPP_STDCALL* FP_LockWindowUpdate)(HWND hWndLock);
    ALIAS(LockWindowUpdate, 0x7E145C);

    typedef int(YRPP_STDCALL* FP_MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
    ALIAS(MessageBoxA, 0x7E1460);

    typedef int(YRPP_STDCALL* FP_ReleaseDC)(HWND hWnd, HDC hDC);
    ALIAS(ReleaseDC, 0x7E1464);

    typedef HWND(YRPP_STDCALL* FP_WindowFromPoint)(POINT Point);
    ALIAS(WindowFromPoint, 0x7E1468);

    typedef BOOL(YRPP_STDCALL* FP_UpdateWindow)(HWND hWnd);
    ALIAS(UpdateWindow, 0x7E146C);

    typedef LONG(YRPP_STDCALL* FP_SetWindowLongA)(HWND hWnd, int nIndex, LONG dwNewLong);
    ALIAS(SetWindowLongA, 0x7E1470);

    typedef LONG(YRPP_STDCALL* FP_GetWindowLongA)(HWND hWnd, int nIndex);
    ALIAS(GetWindowLongA, 0x7E1474);

    typedef BOOL(YRPP_STDCALL* FP_ValidateRect)(HWND hWnd, const RECT* lpRect);
    ALIAS(ValidateRect, 0x7E1478);

    typedef BOOL(YRPP_STDCALL* FP_IntersectRect)(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
    ALIAS(IntersectRect, 0x7E147C);

    typedef int(YRPP_STDCALL* FP_MessageBoxIndirectA)(LPMSGBOXPARAMSA);
    ALIAS(MessageBoxIndirectA, 0x7E1480);

    typedef BOOL(YRPP_STDCALL* FP_PeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
                                                UINT wRemoveMsg);
    ALIAS(PeekMessageA, 0x7E1484);

    typedef LRESULT(YRPP_STDCALL* FP_CallWindowProcA)(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam,
                                                      LPARAM lParam);
    ALIAS(CallWindowProcA, 0x7E1488);

    typedef BOOL(YRPP_STDCALL* FP_KillTimer)(HWND hWnd, UINT uIDEvent);
    ALIAS(KillTimer, 0x7E148C);

    typedef LONG(YRPP_STDCALL* FP_SendDlgItemMessageA)(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam,
                                                       LPARAM lParam);
    ALIAS(SendDlgItemMessageA, 0x7E1490);

    typedef UINT(YRPP_STDCALL* FP_SetTimer)(HWND hWnd, UINT nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
    ALIAS(SetTimer, 0x7E1494);

    typedef BOOL(YRPP_STDCALL* FP_ShowWindow)(HWND hWnd, int nCmdShow);
    ALIAS(ShowWindow, 0x7E1498);

    typedef BOOL(YRPP_STDCALL* FP_InvalidateRect)(HWND hWnd, const RECT* lpRect, BOOL bErase);
    ALIAS(InvalidateRect, 0x7E149C);

    typedef BOOL(YRPP_STDCALL* FP_EnableWindow)(HWND hWnd, BOOL bEnable);
    ALIAS(EnableWindow, 0x7E14A0);

    typedef LRESULT(YRPP_STDCALL* FP_SendMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    ALIAS(SendMessageA, 0x7E14A4);

    typedef HWND(YRPP_STDCALL* FP_GetDlgItem)(HWND hDlg, int nIDDlgItem);
    ALIAS(GetDlgItem, 0x7E14A8);

    typedef BOOL(YRPP_STDCALL* FP_PostMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    ALIAS(PostMessageA, 0x7E14AC);

    typedef int (*FP_wsprintfA)(LPSTR, LPCSTR, ...);
    ALIAS(wsprintfA, 0x7E14B0);

    typedef BOOL(YRPP_STDCALL* FP_SetRect)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
    ALIAS(SetRect, 0x7E14B4);

    typedef BOOL(YRPP_STDCALL* FP_ClientToScreen)(HWND hWnd, LPPOINT lpPoint);
    ALIAS(ClientToScreen, 0x7E14B8);

    typedef BOOL(YRPP_STDCALL* FP_TranslateMessage)(const MSG* lpMsg);
    ALIAS(TranslateMessage, 0x7E14BC);

    typedef LONG(YRPP_STDCALL* FP_DispatchMessageA)(const MSG* lpMsg);
    ALIAS(DispatchMessageA, 0x7E14C0);

    typedef BOOL(YRPP_STDCALL* FP_GetClientRect)(HWND hWnd, LPRECT lpRect);
    ALIAS(GetClientRect, 0x7E14C4);

    typedef HWND(YRPP_STDCALL* FP_GetWindow)(HWND hWnd, UINT uCmd);
    ALIAS(GetWindow, 0x7E14C8);

    typedef BOOL(YRPP_STDCALL* FP_BringWindowToTop)(HWND hWnd);
    ALIAS(BringWindowToTop, 0x7E14CC);

    typedef BOOL(YRPP_STDCALL* FP_SetForegroundWindow)(HWND hWnd);
    ALIAS(SetForegroundWindow, 0x7E14D0);

    typedef HWND(YRPP_STDCALL* FP_CreateWindowExA)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                                                   DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                                   HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    ALIAS(CreateWindowExA, 0x7E14D4);

    typedef ATOM(YRPP_STDCALL* FP_RegisterClassA)(const WNDCLASSA* lpWndClass);
    ALIAS(RegisterClassA, 0x7E14D8);

    typedef int(YRPP_STDCALL* FP_GetClassNameA)(HWND hWnd, LPSTR lpClassName, int nMaxCount);
    ALIAS(GetClassNameA, 0x7E14DC);

    typedef BOOL(YRPP_STDCALL* FP_IsWindowVisible)(HWND hWnd);
    ALIAS(IsWindowVisible, 0x7E14E0);

    typedef BOOL(YRPP_STDCALL* FP_EnumChildWindows)(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam);
    ALIAS(EnumChildWindows, 0x7E14E4);

    typedef BOOL(YRPP_STDCALL* FP_IsWindowEnabled)(HWND hWnd);
    ALIAS(IsWindowEnabled, 0x7E14E8);

    typedef HWND(YRPP_STDCALL* FP_GetParent)(HWND hWnd);
    ALIAS(GetParent, 0x7E14EC);

    typedef HWND(YRPP_STDCALL* FP_GetNextDlgTabItem)(HWND hDlg, HWND hCtl, BOOL bPrevious);
    ALIAS(GetNextDlgTabItem, 0x7E14F0);

    typedef BOOL(YRPP_STDCALL* FP_IsDialogMessageA)(HWND hDlg, LPMSG lpMsg);
    ALIAS(IsDialogMessageA, 0x7E14F4);

    typedef int(YRPP_STDCALL* FP_TranslateAcceleratorA)(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);
    ALIAS(TranslateAcceleratorA, 0x7E14F8);

    typedef BOOL(YRPP_STDCALL* FP_CharToOemBuffA)(LPCSTR lpszSrc, LPSTR lpszDst, DWORD cchDstLength);
    ALIAS(CharToOemBuffA, 0x7E14FC);

    typedef HDC(YRPP_STDCALL* FP_BeginPaint)(HWND hWnd, LPPAINTSTRUCT lpPaint);
    ALIAS(BeginPaint, 0x7E1500);

    typedef BOOL(YRPP_STDCALL* FP_EndPaint)(HWND hWnd, const PAINTSTRUCT* lpPaint);
    ALIAS(EndPaint, 0x7E1504);

    typedef HWND(YRPP_STDCALL* FP_CreateDialogParamA)(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent,
                                                      DLGPROC lpDialogFunc, LPARAM dwInitParam);
    ALIAS(CreateDialogParamA, 0x7E1508);

    typedef int(YRPP_STDCALL* FP_GetWindowTextA)(HWND hWnd, LPSTR lpString, int nMaxCount);
    ALIAS(GetWindowTextA, 0x7E150C);

    typedef BOOL(YRPP_STDCALL* FP_RegisterHotKey)(HWND hWnd, int id, UINT fsModifiers, UINT vk);
    ALIAS(RegisterHotKey, 0x7E1510);

    typedef LONG(YRPP_STDCALL* FP_InterlockedIncrement)(void* lpAddend);
    ALIAS(InterlockedIncrement, 0x7E11C8);

    typedef LONG(YRPP_STDCALL* FP_InterlockedDecrement)(void* lpAddend);
    ALIAS(InterlockedDecrement, 0x7E11CC);
#undef ALIAS
};

#pragma warning(pop)
#endif  // _WIN32: these imports describe the original Windows DLL entry table.

class MovieInfo {
public:
    // Original registry is DVC<const char*>: resizing transfers pointer values,
    // not owning MovieInfo objects. Names remain owned until ClearArray().
    static DynamicVectorClass<const char*>& Array;
    static int YRPP_FASTCALL FindIndex(const char* name);
    static void ClearArray();

    bool operator==(MovieInfo const& rhs) const { return !_strcmpi(this->Name, rhs.Name); }

    explicit MovieInfo(const char* fname)
#ifdef _WIN32
        : Name(fname ? _strdup(fname) : nullptr)
#else
        : Name(fname ? strdup(fname) : nullptr)
#endif
    {
    }

    MovieInfo() : Name(nullptr) {}

    ~MovieInfo() {
        if (this->Name) {
            free(const_cast<char*>(this->Name));
        }
    }

    const char* Name;  // yes, only that
};

struct MovieUnlockableInfo {
    /// Global VA: 0x00832C20.
    DEFINE_ARRAY_REFERENCE(MovieUnlockableInfo, [1u], Common, 0x832C20u)
    /// Global VA: 0x00832C30.
    DEFINE_ARRAY_REFERENCE(MovieUnlockableInfo, [8u], Allied, 0x832C30u)
    /// Global VA: 0x00832CA0.
    DEFINE_ARRAY_REFERENCE(MovieUnlockableInfo, [8u], Soviet, 0x832CA0u)

    MovieUnlockableInfo() = default;

    explicit MovieUnlockableInfo(const char* pFilename, const char* pDescription = nullptr, int disk = 2)
        : Filename(pFilename), Description(pDescription), DiskRequired(disk) {}

    const char* Filename{nullptr};
    const char* Description{nullptr};
    int DiskRequired{2};
};

namespace Unsorted {
// Borrowed selection representative. Cell takes precedence when both target
// arguments are supplied. Returns null for an empty selection or failure;
// exceptions must not propagate across this original entry boundary.
/// VA: 0x005353D0
ObjectClass* YRPP_FASTCALL BestSelectedObject(const CellStruct* cell, ObjectClass* target) noexcept;

// if != 0, EVA_SWxxxActivated is skipped
/// Global VA: 0x00A8B538.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(int, MuteSWLaunches, 0xA8B538)
#else
extern int& MuteSWLaunches;
#endif

// Original drag_select_aborted: suppress a second release dispatch in the
// same input pass after completing/aborting tactical rubber-band selection.
/// Global VA: 0x00A8ED9D.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(bool, DragSelectAborted, 0xA8ED9D)
#else
extern bool& DragSelectAborted;
#endif

// skip unit selection and move command voices?
/// Global VA: 0x00822CF2.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(bool, MoveFeedback, 0x822CF2)
#else
extern bool& MoveFeedback;
#endif

/// Global VA: 0x00A8ED6B.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(byte, ArmageddonMode, 0xA8ED6B)
#else
extern byte& ArmageddonMode;
#endif
/// Global VA: 0x008A0360.
DEFINE_REFERENCE(DynamicVectorClass<ObjectClass*>, ObjectsInLayers, 0x8A0360)

// checkbox states, afaik
/// Global VA: 0x00A8B258.
DEFINE_REFERENCE(byte, Bases, 0xA8B258)
/// Global VA: 0x00A8B260.
DEFINE_REFERENCE(byte, BridgeDestruction, 0xA8B260)
/// Global VA: 0x00A8B261.
DEFINE_REFERENCE(byte, Crates, 0xA8B261)
/// Global VA: 0x00A8B262.
DEFINE_REFERENCE(byte, ShortGame, 0xA8B262)
/// Global VA: 0x00A8B263.
DEFINE_REFERENCE(byte, SWAllowed, 0xA8B263)
/// Global VA: 0x00A8B26C.
DEFINE_REFERENCE(byte, MultiEngineer, 0xA8B26C)
/// Global VA: 0x00A8B31C.
DEFINE_REFERENCE(byte, AlliesAllowed, 0xA8B31C)
/// Global VA: 0x00A8B31D.
DEFINE_REFERENCE(byte, HarvesterTruce, 0xA8B31D)
/// Global VA: 0x00A8B31E.
DEFINE_REFERENCE(byte, CTF, 0xA8B31E)
/// Global VA: 0x00A8B31F.
DEFINE_REFERENCE(byte, FOW, 0xA8B31F)
/// Global VA: 0x00A8B320.
DEFINE_REFERENCE(byte, MCVRedeploy, 0xA8B320)

/// Global VA: 0x00B0CEC8.
#if defined(RA2_YRPP_GAME)
DEFINE_ARRAY_REFERENCE(TacticalSelectableStruct, [500], TacticalSelectables, 0xB0CEC8)
#else
extern TacticalSelectableStruct (&TacticalSelectables)[RA2_CONTROL_CAPACITY];
#endif
/// Global VA: 0x00B0FE65.
DEFINE_REFERENCE(bool, TypeSelecting, 0xB0FE65)

struct ColorPacker {
    int _R_SHL;
    int _R_SHR;
    int _B_SHL;
    int _B_SHR;
    int _G_SHL;
    int _G_SHR;
};

/// Global VA: 0x008A0DD0.
DEFINE_REFERENCE(ColorPacker, ColorPackData, 0x8A0DD0)

/// Global VA: 0x00ABD490.
DEFINE_REFERENCE(CellStruct, CellSpreadTable, 0xABD490)

/// Global VA: 0x008809A0.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(int, CurrentSWType, 0x8809A0)
#else
extern int& CurrentSWType;
#endif

static const int except_txt_length = 0xFFFF;
/// Global VA: 0x008A3A08.
DEFINE_ARRAY_REFERENCE(char, [65536], except_txt_content, 0x8A3A08)

/// Global VA: 0x0089F688.
#if defined(RA2_YRPP_GAME)
DEFINE_ARRAY_REFERENCE(CellStruct, [8], AdjacentCell, 0x89F688)
#else
extern CellStruct (&AdjacentCell)[8];
#endif
/// Global VA: 0x0089F6D8.
#if defined(RA2_YRPP_GAME)
DEFINE_ARRAY_REFERENCE(Point2D, [8], AdjacentCoord, 0x89F6D8)
#else
extern Point2D (&AdjacentCoord)[8];
#endif

/*
     * This thing is ridiculous
     * all xxTypeClass::Create functions use it:

        // doing this makes no sense - it's just a wrapper around CTOR, which doesn't call any Mutex'd functions... but who cares
        InfantryTypeClass *foo = something;
        ++SomeMutex;
        InfantryClass *obj = foo->CreateObject();
        --SomeMutex;

        // XXX do not do this if you aren't sure if the object can exist in this place
        // - this flag overrides any placement checks so you can put Terror Drones into trees and stuff
        ++SomeMutex;
        obj->Unlimbo(blah);
        --SomeMutex;

        AI base node generation uses it:
        int level = SomeMutex;
        SomeMutex = 0;
        House->GenerateAIBuildList();
        SomeMutex = level;

        Building destruction uses it:
        if(!SomeMutex) {
            Building->ShutdownSensorArray();
            Building->ShutdownDisguiseSensor();
        }

        Building placement uses it:
        if(!SomeMutex) {
            UnitTypeClass *freebie = Building->Type->FreeUnit;
            if(freebie) {
                freebie->CreateObject(blah);
            }
        }

        Building state animations use it:
        if(SomeMutex) {
            // foreach attached anim
            // update anim state (normal | damaged | garrisoned) if necessary, play anim
        }

        building selling uses it:
        if(blah) {
            ++SomeMutex;
            this->Type->UndeploysInto->CreateAtMapCoords(blah);
            --SomeMutex;
        }

        Robot Control Centers use it:
        if ( !SomeMutex ) {
            VoxClass::PlayFromName("EVA_RobotTanksOffline/BackOnline", -1, -1);
    }

    and so on...
    */
// Note: SomeMutex has been renamed to this because it reflects the usage better
/// Global VA: 0x00A8E7AC.
extern int& ScenarioInit;
/// Global VA: 0x00A8ED80.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(bool, GameInFocus, 0xA8ED80)
#else
extern bool& GameInFocus;
#endif

/// Global VA: 0x00A8EDA0.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(int, SpecialDialog, 0xA8EDA0)
#else
extern int& SpecialDialog;
#endif
/// Global VA: 0x00A8ED8C.
DEFINE_REFERENCE(int, WSDialogCount, 0xA8ED8C)

/// Global VA: 0x00A8ED5C.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(bool, ScenarioStarted, 0xA8ED5C)
#else
extern bool& ScenarioStarted;
#endif

// Whether the tactical map is being updated/drawn. Cleared to halt the
// battlefield behind a full-screen dialog and restored afterwards.
/// Global VA: 0x00A8E378.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(bool, TacticalActive, 0xA8E378)
#else
extern bool& TacticalActive;
#endif

// Locks the game's own input processing so a modal dialog owns input.
/// Global VA: 0x00A8ED9C.
#if defined(RA2_YRPP_GAME)
DEFINE_REFERENCE(bool, UserInputLocked, 0xA8ED9C)
#else
extern bool& UserInputLocked;
#endif

/// Global VA: 0x00B0BCE4.
DEFINE_REFERENCE(HANDLE, AppMutex, 0xB0BCE4)
};

struct CheatData {
    bool* Destination;
    const char* TriggerString;
    DWORD unknown1;
    DWORD unknown2;
};

// this holds four original cheats, keep that limit in mind
/// Global VA: 0x00825C28.
DEFINE_ARRAY_REFERENCE(CheatData, [4], OriginalCheats, 0x825C28)
