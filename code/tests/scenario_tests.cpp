#include "support/test_support.hpp"
#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "api/clock.hpp"
#include "api/scenario_runtime.hpp"
#include "api/ini_runtime.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

static_assert(std::is_same_v<decltype(ScenarioClass::Intro), int>);
static_assert(std::is_same_v<decltype(ScenarioClass::ElapsedTimer), SysElapsedTimerClass>);
static_assert(std::is_same_v<decltype(ScenarioClass::PauseTimer), SysTimerClass>);
static_assert(offsetof(SessionClass, RecordFile) % alignof(CCFileClass) == 0);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ScenarioClass) == 0x3740);
static_assert(sizeof(Randomizer) == 0x3f4 && offsetof(Randomizer, Table) == 0xc);
static_assert(offsetof(ScenarioClass, Random) == 0x218);
static_assert(offsetof(ScenarioClass, ElapsedTimer) == 0x614);
static_assert(offsetof(ScenarioClass, PauseTimer) == 0x620);
static_assert(offsetof(ScenarioClass, MissionTimer) == 0x11e8);
static_assert(offsetof(ScenarioClass, Intro) == 0x1434 && offsetof(ScenarioClass, PreMapSelect) == 0x144c);
static_assert(offsetof(ScenarioClass, GlobalVariables) == 0x1c88);
static_assert(offsetof(ScenarioClass, LocalVariables) == 0x248a);
static_assert(offsetof(ScenarioClass, AllowableUnits) == 0x34d4);
static_assert(offsetof(ScenarioClass, AllowableUnitMaximums) == 0x34f0);
static_assert(offsetof(ScenarioClass, DropshipUnitCounts) == 0x350c);
#endif

namespace {

struct Clock {
    DWORD now = 16000;
    DWORD step = 0;
    int calls = 0;
    static std::uint32_t milliseconds(void* context) noexcept {
        auto& c = *static_cast<Clock*>(context);
        DWORD now = c.now; c.now += c.step; ++c.calls; return now;
    }
};
void clocks(void* context) {
    auto& clock = *static_cast<Clock*>(context);
    EXPECT_TRUE((SystemTimer::GetTime() == 1000)) << "system clock divides wrapping milliseconds by 16";
    SysElapsedTimerClass elapsed; SysTimerClass countdown;
    elapsed.Start(7); countdown.Start(20);
    Unsorted::CurrentFrame = 99;
    CDTimerClass frames(30);
    clock.now += 16 * 5;
    EXPECT_TRUE((elapsed.GetTimeElapsed() == 12 && countdown.GetTimeLeft() == 15)) << "elapsed and countdown use opposite time directions";
    EXPECT_TRUE((frames.GetTimeLeft() == 30)) << "system clock cannot advance frame timers";
    Unsorted::CurrentFrame += 3;
    EXPECT_TRUE((frames.GetTimeLeft() == 27)) << "frame timer follows original frame storage";
    elapsed.Pause(); countdown.Pause();
    clock.now += 16000;
    EXPECT_TRUE((elapsed.GetTimeElapsed() == 12 && countdown.GetTimeLeft() == 15)) << "paused timers hold state";
    elapsed.Pause(); countdown.Pause();
    elapsed.Resume(); countdown.Resume(); clock.now += 16 * 20;
    EXPECT_TRUE((elapsed.GetTimeElapsed() == 32 && countdown.GetTimeLeft() == 0)) << "resume preserves accumulated time and clamps countdown";
    elapsed.Stop(); countdown.Stop();
    EXPECT_TRUE((!elapsed.IsTicking() && elapsed.GetTimeElapsed() == 0 && countdown.Expired())) << "stop semantics";
    clock.now = 0xfffffff0u;
    EXPECT_TRUE((SystemTimer::GetTime() == 0x0fffffffu)) << "original millisecond counter width";
    clock.now = 0;
    EXPECT_TRUE((SystemTimer::GetTime() == 0)) << "original timeGetTime wrap is not extended to 64 bits";
    Clock nested{32000};
    game::ClockServices nested_services{&nested, Clock::milliseconds};
    try {
        game::with_clock(nested_services, [](void*) {
            EXPECT_TRUE((SystemTimer::GetTime() == 2000)) << "nested clock";
            throw std::runtime_error("fixture");
        }, nullptr);
    } catch (const std::runtime_error&) {}
    EXPECT_TRUE((SystemTimer::GetTime() == 0)) << "exception restores previous clock";
}
void random_vectors() {
    std::ifstream source(RA2_SCENARIO_RANDOM_FIXTURE);
    EXPECT_TRUE((bool(source))) << "open original RNG fixture";
    DWORD seed, result; int index, count = 0;
    Randomizer random(0);
    while (source >> seed >> index >> result) {
        if (index == 0) random = Randomizer(seed);
        EXPECT_TRUE((static_cast<DWORD>(random.Random()) == result)) << "original RNG sequence mismatch";
        ++count;
    }
    EXPECT_TRUE((source.eof() && count == 3000)) << "complete original RNG vectors";
    Randomizer first(42), second(42);
    EXPECT_TRUE((first.RandomRanged(9, 9) == 9 && first.Next1 == 0)) << "single-valued range consumes nothing";
    for (int i = 0; i < 2000; ++i) {
        const int a = first.RandomRanged(-37, 142), b = second.RandomRanged(142, -37);
        EXPECT_TRUE((a == b && a >= -37 && a <= 142)) << "swapped and signed range";
    }
    first.unknown_00 = true;
    const int next = first.Next1;
    EXPECT_TRUE((first.Random() == 0 && first.RandomRanged(5, 50) == 5 && first.Next1 == next)) << "disabled random has no state consumption";
}
struct Runtime {
    ScenarioClass* scenario = nullptr;
    int mode = -1, notifications = 0, index = -1;
    bool global = false;
    CellClass* cell = nullptr;
    MapClass* lookup_map = nullptr;
    game::ScenarioRenderServices* render = nullptr;
    int redraws = 0, palette_visits = 0;
    static int session(void* p) noexcept { return static_cast<Runtime*>(p)->mode; }
    static void changed(void* p, bool global, int index) {
        auto& r = *static_cast<Runtime*>(p);
        EXPECT_TRUE((r.scenario->VariablesChanged)) << "notification observes updated flag";
        r.global = global; r.index = index; ++r.notifications;
    }
    static bool text(void*, const char* label, int, const wchar_t*& output) {
        if (!std::strcmp(label, "TXT_COMPUTER")) { output = L"Computer fixture"; return true; }
        if (std::strcmp(label, "GUI:SkirmishGame")) return false;
        output = L"Skirmish fixture"; return true;
    }
    static bool cell_at(void* p, const CellStruct& coordinates, CellClass*& output) {
        auto& runtime = *static_cast<Runtime*>(p);
        if (runtime.lookup_map) {
            output = runtime.lookup_map->TryGetCellAt(coordinates);
            if (output) return true;
        }
        if (!runtime.cell) return false;
        runtime.cell->MapCoords = coordinates;
        output = runtime.cell;
        return true;
    }
    static bool palette(void* p, HashIterator*, DynamicVectorClass<ColorScheme*>*&) {
        ++static_cast<Runtime*>(p)->palette_visits; return false;
    }
    static int palette_count(void*) { return 0; }
    static void redraw(void* p, int mode) {
        EXPECT_TRUE((mode == 1)) << "lighting requests the original sidebar redraw mode";
        ++static_cast<Runtime*>(p)->redraws;
    }
};
struct MemoryStream {
    std::vector<unsigned char> bytes;
    std::size_t cursor = 0;
    bool reject_write = false;
    std::vector<unsigned> ids;
    std::vector<TechnoTypeClass**> slots;
    IStream* handle() { return reinterpret_cast<IStream*>(this); }
    static bool read(void*, IStream* handle, void* data, unsigned size) {
        auto& self = *reinterpret_cast<MemoryStream*>(handle);
        if (size > self.bytes.size() - self.cursor) return false;
        std::memcpy(data, self.bytes.data() + self.cursor, size); self.cursor += size;
        return true;
    }
    static bool write(void*, IStream* handle, const void* data, unsigned size) {
        auto& self = *reinterpret_cast<MemoryStream*>(handle);
        if (self.reject_write) return false;
        const auto* first = static_cast<const unsigned char*>(data);
        self.bytes.insert(self.bytes.end(), first, first + size); return true;
    }
    static bool pointer_id(void*, const TechnoTypeClass* type, unsigned& id) {
        const auto value = reinterpret_cast<std::uintptr_t>(type);
        if (value > 0xffffffffu) return false;
        id = static_cast<unsigned>(value); return true;
    }
    static bool swizzle(void* context, unsigned id, TechnoTypeClass** slot) {
        auto& self = *static_cast<MemoryStream*>(context);
        self.ids.push_back(id); self.slots.push_back(slot); *slot = nullptr;
        return true;
    }
};
void streams(void* context) {
    auto& memory = *static_cast<MemoryStream*>(context);
    ScenarioClass source;
    source.CampaignIndex = 17;
    source.Name[0] = 0x4e2d; source.Name[1] = 0;
    std::wcscpy(source.Briefing, L"Portable \U0001f680 briefing");
    source.DominatorLighting = {{17, 18, 19}, 153, 261};
    source.Waypoints[701] = {-7, 321}; source.LocalVariables[99].Value = -2;
    source.AllowableUnits.unknown_18 = 123;
    for (unsigned i = 0; i < 25; ++i) {
        EXPECT_TRUE((source.AllowableUnits.AddItem(reinterpret_cast<TechnoTypeClass*>(std::uintptr_t(0x1000 + i * 4))))) << "stream pointer list";
        EXPECT_TRUE((source.AllowableUnitMaximums.AddItem(i % 2 ? -1 : int(i)))) << "stream int list";
    }
    EXPECT_TRUE((source.DropshipUnitCounts.AddItem(7))) << "dropship stream list";
    source.Random.Random(); source.Random.Random();
    source.ElapsedTimer.Start(19);
    EXPECT_TRUE((source.Save(memory.handle()) && source.ElapsedTimer.IsTicking())) << "save restores running elapsed timer";
    EXPECT_TRUE((memory.bytes.size() == 0x3740 + 12 + 51 * 4)) << "fixed x86 record and list encoding";
    EXPECT_TRUE((memory.bytes[0x1360] == 0x2d && memory.bytes[0x1361] == 0x4e)) << "UTF-16 name offset independent of native wchar size";
    ScenarioClass loaded;
    EXPECT_TRUE((loaded.DropshipUnitCounts.AddItem(999))) << "load replaces an existing owned array";
    EXPECT_TRUE((loaded.Load(memory.handle()) && memory.cursor == memory.bytes.size())) << "read complete scenario record";
    EXPECT_TRUE((loaded.CampaignIndex == 17 && loaded.Waypoints[701] == CellStruct{-7, 321} &&
        loaded.LocalVariables[99].Value == -2 && loaded.DominatorLighting.Ground == 153 &&
        !std::wcscmp(loaded.Briefing, source.Briefing) && loaded.Name[0] == 0x4e2d)) << "cross-layout record restores scenario fields and Unicode";
    Randomizer initial(0);
    EXPECT_TRUE((loaded.Random.Next1 == 0 && loaded.Random.Random() == initial.Random() &&
        loaded.PauseTimer.IsTicking() && loaded.PauseTimer.TimeLeft == 0 &&
        loaded.ElapsedTimer.IsTicking() && loaded.ElapsedTimer.TimeElapsed == 19)) << "load reconstructs random and pause timer and resumes elapsed timer";
    EXPECT_TRUE((loaded.AllowableUnits.Count == 25 && loaded.AllowableUnits.unknown_18 == 123 &&
        loaded.AllowableUnitMaximums[1] == -1 && loaded.DropshipUnitCounts[0] == 7 && memory.ids.size() == 25)) << "load restores original list state and registers every reference";
    for (unsigned i = 0; i < 25; ++i) {
        EXPECT_TRUE((memory.ids[i] == 0x1000 + i * 4 && memory.slots[i] == &loaded.AllowableUnits[i])) << "swizzle slots remain stable after all vector growth";
        *memory.slots[i] = source.AllowableUnits[i];
    }
    memory.reject_write = true;
    EXPECT_TRUE((!source.Save(memory.handle()) && source.ElapsedTimer.IsTicking())) << "write failure restores running timer";
    memory.reject_write = false;
    source.ElapsedTimer.Pause(); memory.bytes.clear();
    EXPECT_TRUE((source.Save(memory.handle()) && !source.ElapsedTimer.IsTicking())) << "paused save remains paused";
    for (const auto size : {std::size_t(4), std::size_t(0x3740 + 3), memory.bytes.size() - 1}) {
        MemoryStream truncated; truncated.bytes.assign(memory.bytes.begin(), memory.bytes.begin() + size);
        ScenarioClass invalid;
        EXPECT_TRUE((!invalid.Load(truncated.handle()))) << "truncated stream reports failure";
    }
    MemoryStream malformed; malformed.bytes = memory.bytes;
    std::memset(malformed.bytes.data() + 0x3740, 0xff, 4);
    ScenarioClass invalid;
    EXPECT_TRUE((!invalid.Load(malformed.handle()))) << "negative list count reports failure";
}
struct PauseFixture {
    Runtime* runtime;
    Clock* clock;
    VolumeStruct volume{};
    VolumeStruct* volume_slot = &volume;
    ToolTipManager* tooltip = nullptr;
    bool enabled = true;
    std::vector<std::string> events;
    static void append(void* p, const char* text) { static_cast<PauseFixture*>(p)->events.emplace_back(text); }
    static void suspend(void* p) { append(p, "suspend audio"); }
    static void resume(void* p) { append(p, "resume audio"); }
    static void input(void* p, bool paused) { append(p, paused ? "pause input" : "resume input"); }
    static void tooltip_state(void* p, ToolTipManager* value, bool enabled) {
        EXPECT_TRUE((value == static_cast<PauseFixture*>(p)->tooltip)) << "live tooltip singleton";
        append(p, enabled ? "enable tooltip" : "disable tooltip");
    }
    static void capture(void* p) { append(p, "release capture"); }
    static void cursor(void* p, int index, bool mini) {
        EXPECT_TRUE((index == 0 && !mini)) << "pause cursor arguments"; append(p, "default cursor");
    }
    static void restore(void* p) { append(p, "restore cursor"); }
    static void hide(void* p) { append(p, "hide cursor"); }
    static void show(void* p) { append(p, "show cursor"); }
    static void render(void* p) { append(p, "render"); }
    static void redraw(void* p, int mode) { EXPECT_TRUE((mode == 2)) << "pause redraw mode"; append(p, "redraw"); }
};
void pauses(void* context) {
    auto& fixture = *static_cast<PauseFixture*>(context);
    fixture.runtime->mode = -1;
    ScenarioClass scenario;
    auto* previous = ScenarioClass::Instance;
    ScenarioClass::Instance = &scenario;
    struct Restore { ScenarioClass* p; ~Restore() { ScenarioClass::Instance = p; } } restore{previous};
    fixture.clock->step = 16;
    for (int mode : {0, 3, 5}) for (bool ticking : {false, true}) for (bool tips : {false, true}) {
        fixture.runtime->mode = mode;
        fixture.tooltip = tips ? reinterpret_cast<ToolTipManager*>(&fixture) : nullptr;
        fixture.events.clear();
        scenario.ElapsedTimer.StartTime = ticking ? 500 : -1;
        scenario.ElapsedTimer.TimeElapsed = 19;
        scenario.IsGamePaused = true;
        fixture.volume.unknown_int_8 = std::bit_cast<std::int32_t>(0x87651234u);
        fixture.volume.Volume = 0x42;
        const int calls = fixture.clock->calls;
        ScenarioClass::PauseGame();
        const bool pauses_timer = mode == 0 || mode == 5;
        EXPECT_TRUE((ScenarioClass::PausedAudioVolume == 0x8765 && fixture.volume.GetVolume() == 0x4000 &&
            fixture.volume.Volume == 0x43)) << "pause saves unsigned high-word volume and marks dirty";
        EXPECT_TRUE((scenario.ElapsedTimer.IsTicking() == (ticking && !pauses_timer) &&
            fixture.clock->calls - calls == (ticking && pauses_timer ? 2 : 0) && scenario.IsGamePaused)) << "pause mode and original logging clock reads without changing IsGamePaused";
        std::vector<std::string> expected{"suspend audio", "pause input"};
        if (tips) expected.emplace_back("disable tooltip");
        for (const auto* entry : {"release capture", "default cursor", "hide cursor", "redraw", "render", "show cursor"})
            expected.emplace_back(entry);
        EXPECT_TRUE((fixture.events == expected)) << "pause subsystem order";
        fixture.events.clear();
        ScenarioClass::ResumeGame();
        expected = {"restore cursor"};
        if (tips) expected.emplace_back("enable tooltip");
        expected.emplace_back("resume input"); expected.emplace_back("resume audio");
        EXPECT_TRUE((fixture.events == expected && scenario.ElapsedTimer.IsTicking() &&
            fixture.volume.GetVolume() == 0x8765 && ScenarioClass::PausedAudioVolume == 0x4000)) << "resume subsystem order, volume restoration and timer state";
    }
    fixture.clock->step = 0;
    fixture.runtime->mode = -1;
}
struct CountryFixture : HouseTypeClass {
    CountryFixture(const char* name, int index) : HouseTypeClass(noinit_t{}) {
        std::strncpy(ID, name, sizeof(ID)); std::strncpy(Name, name, sizeof(Name));
        ArrayIndex = ArrayIndex2 = index;
    }
};
struct HouseFixture : HouseClass {
    HouseFixture(HouseTypeClass* country, int index) : HouseClass(country) {
        Type = country; ArrayIndex = index; IsHumanPlayer = IsInPlayerControl = false;
        IQLevel2 = -1; TechLevel = -1; ColorSchemeIndex = -1;
    }
};
struct IniFixture {
    bool armageddon = false;
    bool switch_to_campaign = false;
    Runtime* runtime = nullptr;
    std::vector<int> events;
    static void progress(void* p, int value) { static_cast<IniFixture*>(p)->events.push_back(value); }
    static void pump(void* p) {
        auto& self = *static_cast<IniFixture*>(p);
        self.events.push_back(-1);
        if (self.switch_to_campaign) self.runtime->mode = 0;
    }
    static void lighting(void* p) { static_cast<IniFixture*>(p)->events.push_back(-2); }
    static bool name(void*, game::IniTypeKind, int, const char*&) { return false; }
    static bool text(void*, const char*, const wchar_t*&) { return false; }
    static bool find(void*, game::IniTypeKind kind, const char* name, bool allocate, game::IniTypeResult& output) {
        EXPECT_TRUE((!allocate)) << "Scenario INI does not allocate registry types";
        if (kind == game::IniTypeKind::techno && (!_strcmpi(name, "TANK") || !_strcmpi(name, "SOLDIER"))) {
            output.index = !_strcmpi(name, "TANK") ? 0 : 1;
            output.techno = reinterpret_cast<TechnoTypeClass*>(std::uintptr_t(0x1000 + output.index * 4));
            return true;
        }
        if ((kind == game::IniTypeKind::movie || kind == game::IniTypeKind::theme) && !std::strcmp(name, "INTRO")) {
            output.index = 7; return true;
        }
        return false;
    }
};
void read_ini(void* context) {
    auto services = *static_cast<game::ScenarioRuntimeServices*>(context);
    auto& runtime = *static_cast<Runtime*>(services.context);
    runtime.mode = 0;
    IniFixture fixture;
    fixture.runtime = &runtime;
    CountryFixture country("Americans", 12);
    country.ArrayIndex2 = 27;
    HouseFixture first(&country, 0), second(&country, 1);
    std::strcpy(first.PlainName, "Americans"); std::strcpy(second.PlainName, "Player Two");
    first.CurrentDropshipIndex = second.CurrentDropshipIndex = 987;
    DynamicVectorClass<HouseClass*> houses; houses.AddItem(&first); houses.AddItem(&second);
    HouseClass* current = nullptr;
    game::ScenarioHouseServices world{}; world.houses = &houses; world.current_player = &current;
    game::ScenarioIniServices loading{&fixture, &fixture.armageddon, IniFixture::progress, IniFixture::pump, IniFixture::lighting};
    services.houses = &world; services.ini = &loading;
    struct Test { IniFixture* fixture; HouseClass** current; HouseFixture* first; HouseFixture* second; } test{&fixture, &current, &first, &second};
    std::string error;
    game::IniRuntimeServices registry{nullptr, IniFixture::name, IniFixture::find, IniFixture::text};
    struct Outer { game::ScenarioRuntimeServices* services; Test* test; } outer{&services, &test};
    const bool success = game::with_ini_runtime(registry, [](void* p) {
        auto& outer = *static_cast<Outer*>(p);
        EXPECT_TRUE((game::with_scenario_runtime(*outer.services, [](void* p) {
            auto& t = *static_cast<Test*>(p);
            ScenarioClass scenario;
            scenario.StartX = 11; scenario.StartY = 12; scenario.Width = 13; scenario.Height = 14;
            scenario.NumberStartingPoints = 2;
            scenario.StartingPoints[0] = {15, 16}; scenario.StartingPoints[1] = {17, 18};
            scenario.Name[0] = L'Z'; scenario.Name[1] = 0;
            scenario.Intro = 3; scenario.ThemeIndex = 4;
            std::memset(&scenario.SpecialFlags, 0xff, sizeof(scenario.SpecialFlags));
            INIClass ini;
            ini.WriteString("Basic", "Player", "Player Two");
            ini.WriteString("Basic", "Name", "must not replace startup title");
            ini.WriteString("Header", "Waypoint1", "31,32");
            ini.WriteString("Basic", "NextScenario", "ALL02UMD.MAP");
            ini.WriteString("Basic", "Intro", "INTRO");
            ini.WriteString("Basic", "Brief", "unknown");
            ini.WriteString("Basic", "Theme", "unknown");
            ini.WriteString("Basic", "CarryOverMoney", "175%");
            ini.WriteInteger("Basic", "CarryOverCap", 1250);
            ini.WriteInteger("Basic", "Percent", 41);
            ini.WriteInteger("Basic", "StartingDropships", 3);
            ini.WriteInteger("Basic", "HomeCell", 4); ini.WriteInteger("Basic", "AltHomeCell", 5);
            ini.WriteString("Basic", "AllowableUnits", "TANK,missing,tank,SOLDIER,TANK,SOLDIER");
            ini.WriteString("Basic", "AllowableUnitMaximums", "3");
            ini.WriteString("VariableNames", "3", "Ready,1");
            ini.WriteString("Ranking", "ParTimeEasy", "01:02:03");
            ini.WriteString("Ranking", "UnderParTitle", "MISSION:Fast");
            ini.WriteString("Lighting", "Ambient", "0.8799");
            ini.WriteString("Lighting", "Red", "-1");
            ini.WriteString("Lighting", "Ground", "0.037");
            ini.WriteString("Lighting", "NukeAmbientChangeRate", "2.9");
            ini.WriteString("Lighting", "DominatorAmbientChangeRate", "0.025");
            ini.WriteBool("SpecialFlags", "TiberiumGrows", false);
            ini.WriteBool("SpecialFlags", "MCVDeploy", false);
            EXPECT_TRUE((scenario.ReadINI(ini))) << "read complete Scenario INI";
            EXPECT_TRUE((scenario.StartX == 11 && scenario.Width == 13 && scenario.StartingPoints[0] == Point2D{31,32} &&
                scenario.StartingPoints[1] == Point2D{17,18})) << "header fields retain existing defaults";
            EXPECT_TRUE((scenario.Name[0] == L'Z' && !std::strcmp(scenario.NextScenario,"ALL02UMD.MAP") &&
                scenario.Intro == 7 && scenario.Brief == -1 && scenario.ThemeIndex == -1)) << "movie fallback, unknown theme and startup-owned title";
            EXPECT_TRUE((scenario.CarryOverMoney == 1 && scenario.CarryOverCap == 1250 && scenario.Percent == 41 &&
                scenario.StartingDropships == 3 && scenario.HomeCell == 4 && scenario.AltHomeCell == 5)) << "carryover clamp does not scale the read cap";
            EXPECT_TRUE((scenario.AllowableUnits.Count == 5 && scenario.AllowableUnits[0] == scenario.AllowableUnits[1] &&
                scenario.AllowableUnitMaximums.Count == 3 && scenario.AllowableUnitMaximums[2] == -1 &&
                scenario.DropshipUnitCounts.Count == 3)) << "unknown types skipped, duplicates kept, shrinking deficit preserved";
            EXPECT_TRUE((scenario.LocalVariables[3].Value == 1 && scenario.ParTimeEasy == 223380 &&
                !std::strcmp(scenario.UnderParTitle,"MISSION:Fast") &&
                !std::strcmp(scenario.OverParTitle,"GUI:NoOverParTitle"))) << "local variables and ranking defaults";
            EXPECT_TRUE((scenario.AmbientOriginal == 87 && scenario.AmbientCurrent == 87 && scenario.AmbientTarget == 87 &&
                scenario.NormalLighting.Tint.Red == -99 && scenario.NormalLighting.Ground == 37 &&
                scenario.NukeAmbientChangeRate == 2 && scenario.DominatorAmbientChangeRate == 25)) << "lighting scales, bias and unscaled nuke rate";
            EXPECT_TRUE((!scenario.SpecialFlags.TiberiumGrows && !scenario.SpecialFlags.MCVDeploy && scenario.SpecialFlags.bit31)) << "special flag reads preserve unrelated bits";
            EXPECT_TRUE((*t.current == t.second && t.second->CurrentDropshipIndex == 0 && t.first->CurrentDropshipIndex == 987 &&
                scenario.HumanPlayerHouseTypeIndex == 27 && t.second->IsHumanPlayer && t.second->IsInPlayerControl)) << "campaign resolves exact house name and country ArrayIndex2";
            EXPECT_TRUE((t.fixture->events == std::vector<int>{-1,55,-1,58,-1,60,-1})) << "INI loading callback order";
            t.fixture->events.clear(); t.fixture->armageddon = true;
            ini.Clear(); ini.WriteString("Basic", "Player", "player two");
            scenario.CarryOverMoney = -0.25; ScenarioClass::NewINIFormat = 99;
            EXPECT_TRUE((scenario.ReadINI(ini))) << "repeated INI with absent keys";
            EXPECT_TRUE((scenario.AllowableUnits.Count == 5 && scenario.AllowableUnitMaximums.Count == 4 &&
                scenario.DropshipUnitCounts.Count == 7 && scenario.CarryOverMoney == -0.25 &&
                ScenarioClass::NewINIFormat == 0 && !scenario.LocalVariables[3].Name[0] && scenario.LocalVariables[3].Value == 1)) << "empty lists keep defaults, dropship counts append and unnamed local value survives";
            EXPECT_TRUE((*t.current == t.first && t.fixture->events == std::vector<int>{-1,55,-1,58,-1,-2,60,-1})) << "case-sensitive miss uses first house; armageddon resets lighting at original point";
            ini.WriteInteger("Header", "NumberStartingPoints", 999);
            scenario.HouseIndices[0] = 12345;
            EXPECT_TRUE((scenario.ReadINI(ini) && scenario.NumberStartingPoints == 999 && scenario.HouseIndices[0] == 12345)) << "malformed header count cannot overwrite adjacent Scenario data";
            t.fixture->runtime->mode = 3;
            t.fixture->armageddon = false; t.fixture->switch_to_campaign = true;
            scenario.SpecialFlags.TiberiumGrows = true;
            ini.Clear(); ini.WriteBool("SpecialFlags", "TiberiumGrows", false);
            EXPECT_TRUE((scenario.ReadINI(ini) && scenario.SpecialFlags.TiberiumGrows && *t.current == t.first)) << "player selection uses live session mode after event processing";
        }, outer.test))) << "INI Scenario services";
    }, &outer, error);
    if (!success) throw std::runtime_error(error);
    runtime.mode = -1;
}
void write_ini(void* context) {
    auto services = *static_cast<game::ScenarioRuntimeServices*>(context);
    auto& runtime = *static_cast<Runtime*>(services.context);
    runtime.mode = 0;
    struct DisplayFixture : DisplayClass {
        ~DisplayFixture() override { Cells.Clear(); } // Fixture Cells are stack-owned.
        bool SetCursor(MouseCursorType, bool) override { return false; }
        bool UpdateCursor(MouseCursorType, bool) override { return false; }
        bool RestoreCursor() override { return false; }
        void UpdateCursorMinimapState(bool) override {}
        MouseCursorType GetLastMouseCursor() override { return MouseCursorType{}; }
    } map;
    struct CellFixture : CellClass {
        mutable int calls = 0;
        CellFixture(short x, short y) : CellClass() { MapCoords = {x,y}; Level = 5; SlopeIndex = 0; }
        CoordStruct* GetCoords(CoordStruct* out) const override { ++calls; return CellClass::GetCoords(out); }
    } first(1,4), second(2,3), outside(-1,8), invalid(0,0);
    struct TypeFixture : TechnoTypeClass {
        TypeFixture(const char* name) : TechnoTypeClass(noinit_t{}) { std::strcpy(ID, name); }
        HRESULT YRPP_STDCALL GetClassID(CLSID*) override { return static_cast<HRESULT>(0x80004001u); }
        AbstractType WhatAmI() const override { return AbstractType::UnitType; }
        int Size() const override { return sizeof(*this); }
        bool SpawnAtMapCoords(CellStruct*, HouseClass*) override { throw std::logic_error("unexpected fixture spawn"); }
        ObjectClass* CreateObject(HouseClass*) override { throw std::logic_error("unexpected fixture object creation"); }
    } tank("TANK"), soldier("SOLDIER");
    map.MapRect = {0,0,4,4}; map.VisibleRect = {0,0,4,4};
    EXPECT_TRUE((map.Cells.SetCapacity(MapClass::MaxCells, nullptr))) << "writer map cell storage";
    std::fill_n(map.Cells.Items, map.Cells.Capacity, nullptr);
    map.Cells.Items[2049] = &first; map.Cells.Items[1538] = &second; map.Cells.Items[1027] = &outside;
    runtime.lookup_map = &map; runtime.cell = &invalid;
    struct ResetMap { Runtime& r; ~ResetMap() { r.lookup_map = nullptr; r.cell = nullptr; } } reset_map{runtime};
    game::ScenarioRenderServices render{}; render.map = &map; services.render = &render;
    CountryFixture country("Americans", 0); HouseFixture house(&country, 0);
    std::strcpy(house.PlainName, "Player One"); HouseClass* player = &house;
    game::ScenarioHouseServices world{}; world.current_player = &player; services.houses = &world;
    game::IniRuntimeServices registry{nullptr, IniFixture::name, IniFixture::find, IniFixture::text};
    struct Test { game::ScenarioRuntimeServices* services; TypeFixture* tank; TypeFixture* soldier;
        CellFixture* first; CellFixture* second; CellFixture* outside; CellFixture* invalid; } test{
        &services,&tank,&soldier,&first,&second,&outside,&invalid};
    std::string error;
    const bool success = game::with_ini_runtime(registry, [](void* p) {
        auto& test = *static_cast<Test*>(p);
        EXPECT_TRUE((game::with_scenario_runtime(*test.services, [](void* p) {
            auto& t = *static_cast<Test*>(p);
            ScenarioClass scenario, active;
            auto* previous = ScenarioClass::Instance; ScenarioClass::Instance = &active;
            struct Restore { ScenarioClass* previous; ~Restore() { ScenarioClass::Instance = previous; } } restore{previous};
            std::strcpy(active.BriefingCSF,"MISSION:Brief"); std::strcpy(active.UIName,"MISSION:Name");
            active.Waypoints[0] = {1,4}; active.Waypoints[2] = {2,3};
            std::wcscpy(scenario.Name,L"Mission \U0001F680"); std::strcpy(scenario.BriefingCSF,"wrong-instance");
            scenario.CarryOverCap = 1234; scenario.CarryOverMoney = 0.75;
            scenario.AllowableUnits.AddItem(t.tank); scenario.AllowableUnits.AddItem(t.soldier);
            scenario.AllowableUnitMaximums.AddItem(3); scenario.AllowableUnitMaximums.AddItem(-1);
            std::strcpy(scenario.LocalVariables[3].Name,"Ready"); scenario.LocalVariables[3].Value = 2;
            scenario.NukeAmbientChangeRate = 29;
            INIClass ini;
            ini.WriteString("Basic","OldKey","remove"); ini.WriteString("Header","Custom","keep");
            ini.WriteString("Ranking","Custom","keep"); ini.WriteString("Lighting","NukeAmbient","remove");
            EXPECT_TRUE((scenario.WriteINI(ini, false))) << "complete single-player INI writer";
            char text[128];
            ini.ReadString("Basic","Briefing","",text);
            EXPECT_TRUE((!std::strcmp(text,"MISSION:Brief"))) << "writer reads Briefing from active global instance";
            ini.ReadString("Basic","Name","",text);
            EXPECT_TRUE((std::strstr(text,"d83d,de80,") != nullptr)) << "writer uses UTF-16 surrogate units on every host";
            EXPECT_TRUE((ini.ReadInteger("Header","StartX",0)==254 && ini.ReadInteger("Header","StartY",0)==3 &&
                ini.ReadInteger("Header","Width",0)==1 && ini.ReadInteger("Header","Height",1)==0)) << "header bounds project usable cells and ignore height";
            EXPECT_TRUE((ini.ReadInteger("Header","NumberStartingPoints",0)==2)) << "count all eight waypoint slots";
            Point2D point{}; ini.GetPoint2D("Header","Waypoint2",point);
            EXPECT_TRUE((point == Point2D{256,0})) << "writer consumes prefix including a hole, using invalid cell coords";
            EXPECT_TRUE((t.first->calls==4 && t.second->calls==2 && !t.outside->calls && t.invalid->calls==2)) << "two GetCoords virtual calls per included cell or starting slot";
            EXPECT_TRUE((!ini.Exists("Basic","OldKey") && ini.Exists("Header","Custom") && ini.Exists("Ranking","Custom") &&
                !ini.Exists("Lighting","NukeAmbient") && !ini.Exists("Lighting","NukeAmbientChangeRate"))) << "section replacement and original omitted nuke fields";
            EXPECT_TRUE((ini.ReadInteger("Basic","CarryOverCap",0)==12 && ini.ReadInteger("Basic","NewINIFormat",0)==4 &&
                ini.ReadBool("Basic","Official",false))) << "writer cap scale, format and official flag";
            ini.ReadString("Basic","Theme","",text); EXPECT_TRUE((!std::strcmp(text,"No theme"))) << "invalid theme sentinel";
            ini.ReadString("Basic","Intro","",text); EXPECT_TRUE((!std::strcmp(text,"<none>"))) << "invalid movie sentinel";
            ini.ReadString("Basic","AllowableUnits","",text); EXPECT_TRUE((!std::strcmp(text,"TANK,SOLDIER"))) << "write existing type IDs";
            ini.ReadString("VariableNames","3","",text); EXPECT_TRUE((!std::strcmp(text,"Ready,1"))) << "write local variable boolean";
            EXPECT_TRUE((scenario.WriteINI(ini, true) && !ini.Exists("Basic","Player") && !ini.Exists("Basic","Intro") &&
                !ini.Exists("Basic","CarryOverMoney") && !ini.Exists("Basic","TimerInherit") && !ini.Exists("Basic","HomeCell") &&
                ini.Exists("Basic","Theme"))) << "multiplayer export omits original conditional fields after Basic clear";
            for (int i = 0; i < 100; ++i) scenario.AllowableUnitMaximums.AddItem(-2147483647);
            ini.WriteString("Basic","Limits","preserve-on-failure");
            EXPECT_TRUE((!ini.WriteIntegers("Basic","Limits",scenario.AllowableUnitMaximums))) << "oversized list fails without stack overwrite";
            ini.ReadString("Basic","Limits","",text); EXPECT_TRUE((!std::strcmp(text,"preserve-on-failure"))) << "oversized list keeps prior entry";
        }, &test))) << "writer Scenario services";
    }, &test, error);
    if (!success) throw std::runtime_error(error);
}
struct HouseWorldFixture {
    Runtime* runtime;
    SessionClass session;
    RulesClass rules;
    RulesClass* rules_slot = &rules;
    DynamicVectorClass<NodeNameType*> players;
    DynamicVectorClass<HouseTypeClass*> countries;
    DynamicVectorClass<HouseClass*> houses;
    HouseClass* current = nullptr;
    HouseClass* observer = nullptr;
    int tech_level = 9;
    unsigned char lookup[9]{10, 11, 12, 13, 14, 15, 16, 17, 18};
    std::vector<std::string> events;
    std::vector<int> handicaps;
    ~HouseWorldFixture() { for (auto* house : houses) delete house; }
    static bool create(void* p, HouseTypeClass* type, HouseClass*& house) {
        auto& self = *static_cast<HouseWorldFixture*>(p);
        house = new HouseFixture(type, self.houses.Count);
        if (!self.houses.AddItem(house)) { delete house; return false; }
        self.events.push_back(std::string("create ") + type->ID);
        return true;
    }
    static void color(void* p, HouseClass*) { static_cast<HouseWorldFixture*>(p)->events.emplace_back("color"); }
    static void laser(void* p, HouseClass*) { static_cast<HouseWorldFixture*>(p)->events.emplace_back("laser"); }
    static void handicap(void* p, HouseClass*, int value) {
        auto& self = *static_cast<HouseWorldFixture*>(p);
        self.events.emplace_back("handicap"); self.handicaps.push_back(value);
    }
};
void house_assignment(void* context) {
    auto& world = *static_cast<HouseWorldFixture*>(context);
    world.runtime->mode = -1;
    ScenarioClass scenario;
    auto* previous = ScenarioClass::Instance;
    ScenarioClass::Instance = &scenario;
    struct Restore { ScenarioClass* p; ~Restore() { ScenarioClass::Instance = p; } } restore{previous};
    CountryFixture first("Americans", 0), second("Russians", 1), neutral("Neutral", 2), special("Special", 3);
    for (auto* country : {&first, &second, &neutral, &special}) EXPECT_TRUE((world.countries.AddItem(country))) << "countries";
    NodeNameType players[3]{};
    for (int i = 0; i < 3; ++i) {
        players[i].Country = i == 2 ? -3 : i;
        players[i].Color = 4 - i;
        players[i].Team = -1;
        players[i].SpectatorFlag = i == 2 ? 0xffffffffu : 0;
        players[i].StartPoint = i - 1;
        players[i].InitialStartPoint = -2;
        std::wcscpy(players[i].Name, i == 0 ? L"Local player" : i == 1 ? L"Opponent" : L"Observer");
        EXPECT_TRUE((world.players.AddItem(&players[i]))) << "players";
    }
    world.session.GameMode = GameMode::Internet;
    world.session.Config.Money = 15000; world.session.Config.AIPlayers = 1;
    for (auto& value : world.session.Config.AISlots.Countries) value = -1;
    world.session.Config.AISlots.Countries[3] = 1;
    world.session.Config.AISlots.Colors[3] = -2;
    world.session.Config.AISlots.Starts[3] = 7;
    world.session.Config.AISlots.Allies[3] = 2;
    world.session.Config.AISlots.Difficulties[3] = 2;
    world.rules.MaxIQLevels = 11; world.rules.CompEasyBonus = true;
    ScenarioClass::AssignHouses();
    EXPECT_TRUE((world.houses.Count == 6 && players[2].Country == 0 && players[2].InitialCountry == -1 &&
        players[0].HouseIndex == 2 && players[2].HouseIndex == 0)) << "human houses follow color order and observer country correction";
    EXPECT_TRUE((world.current == world.houses[2] && world.observer == world.houses[0] &&
        world.current->IsInPlayerControl && world.current->IsHumanPlayer &&
        world.current->StartingPoint == -2 && !std::strcmp(world.current->PlainName, "Local player"))) << "local player is selected by original slot despite sorted creation";
    EXPECT_TRUE((world.current->Balance == 15000 && world.current->StartingCredits == 15000 &&
        world.current->ColorSchemeIndex == 14 && world.current->TechLevel == 9)) << "house initialization updates credits, color and build level";
    auto* ai = world.houses[3];
    EXPECT_TRUE((!ai->IsHumanPlayer && ai->IQLevel2 == 11 && ai->StartingPoint == 7 &&
        ai->StartingAllies.data == 2 && ai->ColorSchemeIndex == 18 && scenario.TeamsPresent &&
        !std::wcscmp(ai->UIName, L"Computer fixture") && !std::strcmp(ai->PlainName, "Computer"))) << "AI slot assignment preserves allies, observer color slot and localized label";
    EXPECT_TRUE((world.handicaps == std::vector<int>({1, 1, 1, 1}))) << "multiplayer easy bonus applies after human handicaps";
    EXPECT_TRUE((world.events == std::vector<std::string>({"create Americans", "color", "laser", "handicap",
        "create Russians", "color", "laser", "handicap", "create Americans", "color", "laser", "handicap",
        "create Russians", "color", "laser", "handicap", "create Neutral", "color", "create Special", "color"}))) << "house dependency order and neutral/special color-only initialization";
    world.players.Clear(); world.countries.Clear();
}
void lifecycle(void* context) {
    auto& runtime = *static_cast<Runtime*>(context);
    ScenarioClass scenario;
    runtime.scenario = &scenario;
    const auto previous_instance = ScenarioClass::Instance;
    ScenarioClass::Instance = &scenario;
    struct Restore { ScenarioClass* p; ~Restore() { ScenarioClass::Instance = p; } } restore{previous_instance};
    EXPECT_TRUE((scenario.UniqueID == 1000000 && scenario.TechLevel == 1 && scenario.CampaignIndex == -1)) << "constructor defaults";
    EXPECT_TRUE((scenario.Intro == -1 && scenario.PreMapSelect == -1 && scenario.Briefing[0] == 0)) << "movie indices and briefing";
    EXPECT_TRUE((!scenario.ElapsedTimer.IsTicking() && !scenario.PauseTimer.IsTicking() &&
        !scenario.MissionTimer.IsTicking())) << "initial stopped timers";
    EXPECT_TRUE((scenario.NormalLighting.Tint.Red == 100 && scenario.NukeLighting.Tint.Blue == 125 &&
        scenario.DominatorAmbient == 150)) << "calibrated lighting defaults";
    EXPECT_TRUE((!scenario.IsDefinedWaypoint(-1) && !scenario.IsDefinedWaypoint(702) &&
        !scenario.IsDefinedWaypoint(701))) << "waypoint limits and empty sentinel";
    scenario.Waypoints[701] = {-4, 5};
    EXPECT_TRUE((scenario.IsDefinedWaypoint(701) && scenario.GetWaypointCoords(701) == CellStruct{-4, 5})) << "waypoint preserves signed coordinates";
    EXPECT_TRUE((scenario.SetGlobal(49, 7) == 0 && runtime.notifications == 1 && runtime.global && runtime.index == 49)) << "global update notification";
    EXPECT_TRUE((scenario.SetGlobal(49, 7) == 7 && runtime.notifications == 1)) << "same global value is silent";
    EXPECT_TRUE((scenario.SetLocal(99, -3) == 0 && runtime.notifications == 2 && !runtime.global)) << "local variables store bytes, not booleans";
    char out = 33;
    EXPECT_TRUE((!scenario.GetGlobal(50, &out) && out == 33 && !scenario.GetLocal(-1, nullptr))) << "invalid query leaves output untouched";
    EXPECT_TRUE((scenario.GetLocal(99, &out) && out == -3)) << "local output";
    EXPECT_TRUE((scenario.SetLocal(100, 1) == 0 && runtime.notifications == 2)) << "invalid set is silent";
    auto* borrowed_type = reinterpret_cast<TechnoTypeClass*>(std::uintptr_t(0x1234));
    EXPECT_TRUE((scenario.AllowableUnits.AddItem(borrowed_type) && scenario.DropshipUnitCounts.AddItem(3))) << "real owning lists";
    scenario.Random.Random(); const int rng_index = scenario.Random.Next1;
    scenario.CampaignIndex = 8; scenario.IsRandom = true;
    runtime.mode = 5;
    scenario.Reset();
    EXPECT_TRUE((!std::wcscmp(scenario.Name, L"Skirmish fixture"))) << "skirmish reset uses string table";
    EXPECT_TRUE((scenario.AllowableUnits.Count == 0 && scenario.DropshipUnitCounts.Count == 0)) << "reset clears owned lists";
    EXPECT_TRUE((scenario.GlobalVariables[49].Value == 7 && scenario.LocalVariables[99].Value == -3 &&
        scenario.IsDefinedWaypoint(701) && scenario.Random.Next1 == rng_index &&
        scenario.CampaignIndex == 8 && scenario.IsRandom)) << "reset preserves persistent scenario state";
    EXPECT_TRUE((!scenario.VariablesChanged && scenario.Intro == -1)) << "reset restores change flag and movie defaults";
    scenario.UniqueID = std::numeric_limits<int>::max();
    EXPECT_TRUE((scenario.CreateUniqueID() == std::numeric_limits<int>::min())) << "unique id uses x86 wrap";
    INIClass ini;
    ini.WriteInteger("Header", "NumberStartingPoints", 9);
    ini.WriteString("Header", "Waypoint1", "-3,7");
    scenario.HouseIndices[0] = 12345;
    scenario.ReadStartPoints(ini);
    EXPECT_TRUE((scenario.NumberStartingPoints == 9 && scenario.HouseIndices[0] == 12345 &&
        scenario.StartingPoints[0].X == -3 && scenario.StartingPoints[0].Y == 7)) << "header metadata preserves count without overwriting following fields";
    scenario.ClearWaypoints(); EXPECT_TRUE((!scenario.IsDefinedWaypoint(701))) << "clear waypoints";
    INIClass variables;
    variables.WriteString("VariableNames", "49", "CampaignFlag");
    EXPECT_TRUE((scenario.ReadGlobalVariables(variables) && scenario.FindGlobal("CampaignFlag") == 49 &&
        scenario.FindGlobal("campaignflag") == -1 && scenario.GlobalVariables[49].Value == 7)) << "global names preserve values and use case-sensitive matching";
    EXPECT_TRUE((scenario.SetGlobal("missing", 1) == 1)) << "original missing named setter result";
    variables.Clear("VariableNames");
    variables.WriteString("VariableNames", "99", "DoorOpen,2");
    EXPECT_TRUE((scenario.ReadLocalVariables(variables) && scenario.FindLocal("DoorOpen") == 99 &&
        scenario.LocalVariables[99].Value == 1)) << "local INI initializes a boolean value";
    scenario.LocalVariables[99].Value = -7;
    EXPECT_TRUE((scenario.WriteLocalVariables(variables))) << "write local variable names and values";
    char encoded[64]; variables.ReadString("VariableNames", "99", "", encoded, sizeof(encoded));
    EXPECT_TRUE((!std::strcmp(encoded, "DoorOpen,1"))) << "local INI writer normalizes nonzero values";
    variables.WriteString("VariableNames", "100", "Invalid,1");
    EXPECT_TRUE((!scenario.ReadLocalVariables(variables))) << "malformed variable index cannot overwrite object fields";
    struct CellFixture : CellClass { CellFixture() : CellClass() {} } cell;
    cell.Flags = CellFlags::Explored;
    runtime.cell = &cell;
    INIClass waypoints;
    waypoints.WriteInteger("Waypoints", "701", 5007);
    EXPECT_TRUE((scenario.ReadWaypoints(waypoints) && scenario.GetWaypointCoords(701) == CellStruct{7, 5} &&
        cell.MapCoords == CellStruct{7, 5} && (cell.Flags & CellFlags::IsWaypoint) != CellFlags::Empty &&
        (cell.Flags & CellFlags::Explored) != CellFlags::Empty)) << "waypoint decoding also marks the existing map cell";
    waypoints.WriteInteger("Waypoints", "900", 1001);
    EXPECT_TRUE((scenario.WriteWaypoints(waypoints) && waypoints.GetKeyCount("Waypoints") == 1 &&
        waypoints.ReadInteger("Waypoints", "701", 0) == 5007)) << "waypoint writer replaces the section";
    struct DisplayFixture : DisplayClass {
        ~DisplayFixture() override { Cells.Clear(); } // Fixture Cells are stack-owned.
        bool SetCursor(MouseCursorType, bool) override { return false; }
        bool UpdateCursor(MouseCursorType, bool) override { return false; }
        bool RestoreCursor() override { return false; }
        void UpdateCursorMinimapState(bool) override {}
        MouseCursorType GetLastMouseCursor() override { return MouseCursorType{}; }
    } map;
    EXPECT_TRUE((map.Cells.SetCapacity(MapClass::MaxCells, nullptr))) << "existing map cell registry";
    for (int i = 0; i < map.Cells.Capacity; ++i) map.Cells.Items[i] = nullptr;
    map.MapRect = {0, 0, 2, 2}; map.Cells.Items[1025] = &cell;
    runtime.render->map = &map;
    cell.Ambient = 0; cell.Level = 2; cell.Intensity = 65536;
    scenario.AmbientCurrent = 100;
    ScenarioClass::UpdateLighting();
    EXPECT_TRUE((cell.Intensity_Normal == 966 && cell.Intensity_Terrain == 966 && cell.Color1_Blue == 998 &&
        runtime.redraws == 1 && runtime.palette_visits == 1)) << "lighting traverses actual map cells before sidebar invalidation";
    LightningStorm::Active = true;
    PsyDom::Status = PsychicDominatorStatus::Fire;
    NukeFlash::Status = NukeFlashStatus::FadeIn;
    ScenarioClass::UpdateLighting();
    EXPECT_TRUE((scenario.AmbientTarget == scenario.NukeAmbient && cell.Intensity_Normal == 1000)) << "palette priority and cell-lighting priority are distinct original rules";
    NukeFlash::Status = NukeFlashStatus::FadeOut;
    ScenarioClass::UpdateLighting();
    EXPECT_TRUE((scenario.AmbientTarget == scenario.IonAmbient)) << "lightning wins over dominator";
    LightningStorm::Active = false;
    ScenarioClass::UpdateLighting();
    EXPECT_TRUE((scenario.AmbientTarget == scenario.DominatorAmbient)) << "dominator palette";
    PsyDom::Status = PsychicDominatorStatus::Over;
    ScenarioClass::UpdateLighting();
    EXPECT_TRUE((scenario.AmbientTarget == scenario.AmbientOriginal && cell.Intensity_Normal == 1000)) << "over-state returns palette to normal while cell effect still tests nonzero";
    PsyDom::Status = PsychicDominatorStatus::Inactive;
    for (int quality = 0; quality < 5; ++quality) {
        *runtime.render->quality = quality;
        int red = -20, green = 999, blue = 2000;
        ScenarioClass::ScenarioLighting(&red, &green, &blue);
        const int mask = quality == 0 ? ~127 : quality == 1 ? ~63 : quality == 2 ? ~31 : ~0;
        EXPECT_TRUE((red == 0 && green == (999 & mask) && blue == (1000 & mask))) << "quality quantization";
    }
    cell.Ambient = 0xffff; cell.Level = -128; cell.Intensity = 0xffffffff;
    scenario.AmbientCurrent = std::numeric_limits<int>::max();
    cell.UpdateCellLighting();
    EXPECT_TRUE((cell.Intensity_Normal <= 2000 && cell.Intensity_Terrain <= 2000 && cell.Color1_Blue <= 2000)) << "cell lighting clamps after original 16-bit wrapping";
    int borrowed[]{11, 22};
    scenario.DropshipUnitCounts = TypeList<int>(2, borrowed);
    EXPECT_TRUE((!scenario.DropshipUnitCounts.IsAllocated)) << "borrowed list ownership";
}
}

TEST(Scenario, Contracts) {
    random_vectors();
    Clock clock;
    EXPECT_TRUE((game::with_clock({&clock, Clock::milliseconds}, clocks, &clock))) << "clock binding";
    Runtime runtime;
    int quality = 3;
    DynamicVectorClass<LightConvertClass*> light_converts;
    DynamicVectorClass<ColorScheme*> color_schemes;
    game::ScenarioRenderServices render{&runtime, nullptr, &light_converts, &color_schemes, &quality,
        Runtime::palette, Runtime::palette_count, Runtime::redraw};
    runtime.render = &render;
    game::ScenarioRuntimeServices services{&runtime, Runtime::session, Runtime::changed, Runtime::text,
        Runtime::cell_at, &render};
    clock = Clock{};
    struct Context { Clock* clock; game::ScenarioRuntimeServices* services; Runtime* runtime; } context{&clock,&services,&runtime};
    EXPECT_TRUE((game::with_clock({&clock,Clock::milliseconds}, [](void* p) {
        auto& c = *static_cast<Context*>(p);
        EXPECT_TRUE((game::with_scenario_runtime(*c.services, lifecycle, c.runtime))) << "scenario binding";
    }, &context))) << "scenario clock binding";
    runtime.mode = -1;
    MemoryStream memory;
    game::ScenarioStreamServices io{&memory, MemoryStream::read, MemoryStream::write,
        MemoryStream::pointer_id, MemoryStream::swizzle};
    services.stream = &io;
    EXPECT_TRUE((game::with_clock({&clock, Clock::milliseconds}, [](void* p) {
        auto& c = *static_cast<Context*>(p);
        EXPECT_TRUE((game::with_scenario_runtime(*c.services, streams, c.services->stream->context))) << "stream binding";
    }, &context))) << "stream clock binding";
    PauseFixture pause_fixture{&runtime, &clock};
    game::ScenarioPauseServices pause{&pause_fixture, &pause_fixture.volume_slot, &pause_fixture.tooltip,
        &pause_fixture.enabled, PauseFixture::suspend, PauseFixture::resume, PauseFixture::input,
        PauseFixture::tooltip_state, PauseFixture::capture, PauseFixture::cursor, PauseFixture::restore,
        PauseFixture::hide, PauseFixture::show, PauseFixture::render};
    game::ScenarioRenderServices pause_render{};
    pause_render.context = &pause_fixture; pause_render.redraw_sidebar = PauseFixture::redraw;
    services.pause = &pause; services.render = &pause_render;
    EXPECT_TRUE((game::with_clock({&clock, Clock::milliseconds}, [](void* p) {
        auto& c = *static_cast<Context*>(p);
        EXPECT_TRUE((game::with_scenario_runtime(*c.services, pauses, c.services->pause->context))) << "pause binding";
    }, &context))) << "pause clock binding";
    HouseWorldFixture house_world{&runtime};
    game::ScenarioHouseServices house_services{&house_world, &house_world.session, &house_world.players,
        &house_world.countries, &house_world.houses, &house_world.current, &house_world.observer,
        &house_world.rules_slot, &house_world.tech_level, house_world.lookup, HouseWorldFixture::create,
        HouseWorldFixture::color, HouseWorldFixture::laser, HouseWorldFixture::handicap};
    services.houses = &house_services; services.render = &render;
    EXPECT_TRUE((game::with_clock({&clock, Clock::milliseconds}, [](void* p) {
        auto& c = *static_cast<Context*>(p);
        EXPECT_TRUE((game::with_scenario_runtime(*c.services, house_assignment, c.services->houses->context))) << "house binding";
    }, &context))) << "house clock binding";
    EXPECT_TRUE((!game::with_clock({}, clocks, &clock))) << "invalid clock rejected";
    read_ini(&services);
    write_ini(&services);
}
