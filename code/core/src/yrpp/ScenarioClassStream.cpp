// YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc Scenario object model.
// Scenario record format/order calibrated to fixed YR 689310/689470/683560.
// This is the Scenario record, not the containing whole-world save operation.
#include "yrpp/ScenarioClass.h"
#include "scenario_runtime.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <memory>
#include <type_traits>

namespace {
using Record = std::array<unsigned char, 0x3740>;
static_assert(std::endian::native == std::endian::little);

template<class T> void field(Record& record, unsigned offset, T& value, bool load) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (load) std::memcpy(&value, record.data() + offset, sizeof(T));
    else std::memcpy(record.data() + offset, &value, sizeof(T));
}
template<TimerType Clock> void field(Record& record, unsigned offset, TimerStruct<Clock>& value, bool load) {
    field(record, offset, value.StartTime, load);
    field(record, offset + 8, value.TimeLeft, load);
}
template<std::size_t N> void field(Record& record, unsigned offset, wchar_t (&value)[N], bool load) {
    // UTF-16 on disk, including supplementary Unicode on wchar_t=32 hosts.
    auto* bytes = record.data() + offset;
    if (load) {
        unsigned out = 0;
        for (unsigned i = 0; i < N; ++i) {
            unsigned code = bytes[i * 2] | (bytes[i * 2 + 1] << 8);
            if constexpr (sizeof(wchar_t) > 2) {
                if (code >= 0xd800 && code <= 0xdbff && i + 1 < N) {
                    unsigned low = bytes[(i + 1) * 2] | (bytes[(i + 1) * 2 + 1] << 8);
                    if (low >= 0xdc00 && low <= 0xdfff) {
                        code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                        ++i;
                    }
                }
            }
            value[out++] = static_cast<wchar_t>(code);
        }
        while (out < N) value[out++] = L'\0';
    } else {
        unsigned out = 0;
        const auto put = [&](unsigned code) {
            bytes[out * 2] = static_cast<unsigned char>(code);
            bytes[out * 2 + 1] = static_cast<unsigned char>(code >> 8);
            ++out;
        };
        for (unsigned i = 0; i < N && out < N && value[i]; ++i) {
            unsigned code = static_cast<unsigned>(value[i]);
            if (code > 0xffff) {
                if (code > 0x10ffff) code = 0xfffd;
                else {
                    if (out + 2 >= N) break;
                    put(0xd800 + ((code - 0x10000) >> 10));
                    code = 0xdc00 + ((code - 0x10000) & 0x3ff);
                }
            }
            put(code);
        }
        while (out < N) put(0);
    }
}

void fields(ScenarioClass& self, Record& record, bool load) {
    // Fixed x86 field offsets. Pointer/list representations are handled below.
#if defined(_MSC_VER) && defined(_M_IX86)
#define FIELD(name, offset) static_assert(offsetof(ScenarioClass, name) == offset); field(record, offset, self.name, load)
#else
#define FIELD(name, offset) field(record, offset, self.name, load)
#endif
    FIELD(SpecialFlags, 0x0);
    FIELD(NextScenario, 0x4);
    FIELD(AltNextScenario, 0x108);
    FIELD(HomeCell, 0x20c);
    FIELD(AltHomeCell, 0x210);
    FIELD(UniqueID, 0x214);
    FIELD(Random, 0x218);
    FIELD(Difficulty1, 0x60c);
    FIELD(Difficulty2, 0x610);
    FIELD(ElapsedTimer, 0x614);
    FIELD(PauseTimer, 0x620);
    FIELD(unknown_62C, 0x62c);
    FIELD(IsGamePaused, 0x630);
    FIELD(Waypoints, 0x632);
    FIELD(StartX, 0x112c);
    FIELD(StartY, 0x1130);
    FIELD(Width, 0x1134);
    FIELD(Height, 0x1138);
    FIELD(NumberStartingPoints, 0x113c);
    FIELD(StartingPoints, 0x1140);
    FIELD(HouseIndices, 0x1180);
    FIELD(HouseHomeCells, 0x11c0);
    FIELD(TeamsPresent, 0x11e0);
    FIELD(NumCoopHumanStartSpots, 0x11e4);
    FIELD(MissionTimer, 0x11e8);
    FIELD(MissionTimerText, 0x11f8);
    FIELD(ShroudRegrowTimer, 0x1218);
    FIELD(FogTimer, 0x1224);
    FIELD(IceTimer, 0x1230);
    FIELD(unknown_timer_123c, 0x123c);
    FIELD(AmbientTimer, 0x1248);
    FIELD(TechLevel, 0x1254);
    FIELD(Theater, 0x1258);
    FIELD(FileName, 0x125c);
    FIELD(Name, 0x1360);
    FIELD(UIName, 0x13ba);
    FIELD(UINameLoaded, 0x13da);
    FIELD(Intro, 0x1434);
    FIELD(Brief, 0x1438);
    FIELD(Win, 0x143c);
    FIELD(Lose, 0x1440);
    FIELD(Action, 0x1444);
    FIELD(PostScore, 0x1448);
    FIELD(PreMapSelect, 0x144c);
    FIELD(Briefing, 0x1450);
    FIELD(BriefingCSF, 0x1c50);
    FIELD(ThemeIndex, 0x1c70);
    FIELD(HumanPlayerHouseTypeIndex, 0x1c74);
    FIELD(CarryOverMoney, 0x1c78);
    FIELD(CarryOverCap, 0x1c80);
    FIELD(Percent, 0x1c84);
    FIELD(GlobalVariables, 0x1c88);
    FIELD(LocalVariables, 0x248a);
    FIELD(View1, 0x348e);
    FIELD(View2, 0x3492);
    FIELD(View3, 0x3496);
    FIELD(View4, 0x349a);
    FIELD(unknown_34A0, 0x34a0);
    FIELD(FreeRadar, 0x34a4);
    FIELD(TrainCrate, 0x34a5);
    FIELD(TiberiumGrowthEnabled, 0x34a6);
    FIELD(VeinGrowthEnabled, 0x34a7);
    FIELD(IceGrowthEnabled, 0x34a8);
    FIELD(BridgeDestroyed, 0x34a9);
    FIELD(VariablesChanged, 0x34aa);
    FIELD(AmbientChanged, 0x34ab);
    FIELD(EndOfGame, 0x34ac);
    FIELD(TimerInherit, 0x34ad);
    FIELD(SkipScore, 0x34ae);
    FIELD(OneTimeOnly, 0x34af);
    FIELD(SkipMapSelect, 0x34b0);
    FIELD(TruckCrate, 0x34b1);
    FIELD(FillSilos, 0x34b2);
    FIELD(TiberiumDeathToVisceroid, 0x34b3);
    FIELD(IgnoreGlobalAITriggers, 0x34b4);
    FIELD(unknown_bool_34B5, 0x34b5);
    FIELD(unknown_bool_34B6, 0x34b6);
    FIELD(unknown_bool_34B7, 0x34b7);
    FIELD(PlayerSideIndex, 0x34b8);
    FIELD(MultiplayerOnly, 0x34bc);
    FIELD(IsRandom, 0x34bd);
    FIELD(PickedUpAnyCrate, 0x34be);
    FIELD(unknown_timer_34C0, 0x34c0);
    FIELD(CampaignIndex, 0x34cc);
    FIELD(StartingDropships, 0x34d0);
    FIELD(AmbientOriginal, 0x3528);
    FIELD(AmbientCurrent, 0x352c);
    FIELD(AmbientTarget, 0x3530);
    FIELD(NormalLighting, 0x3534);
    FIELD(IonAmbient, 0x3548);
    FIELD(IonLighting, 0x354c);
    FIELD(NukeAmbient, 0x3560);
    FIELD(NukeLighting, 0x3564);
    FIELD(NukeAmbientChangeRate, 0x3578);
    FIELD(DominatorAmbient, 0x357c);
    FIELD(DominatorLighting, 0x3580);
    FIELD(DominatorAmbientChangeRate, 0x3594);
    FIELD(unknown_3598, 0x3598);
    FIELD(InitTime, 0x359c);
    FIELD(Stage, 0x35a0);
    FIELD(UserInputLocked, 0x35a2);
    FIELD(unknown_35A3, 0x35a3);
    FIELD(ParTimeEasy, 0x35a4);
    FIELD(ParTimeMedium, 0x35a8);
    FIELD(ParTimeDifficult, 0x35ac);
    FIELD(UnderParTitle, 0x35b0);
    FIELD(UnderParMessage, 0x35cf);
    FIELD(OverParTitle, 0x35ee);
    FIELD(OverParMessage, 0x360d);
    FIELD(LSLoadMessage, 0x362c);
    FIELD(LSBrief, 0x364b);
    FIELD(LS640BriefLocX, 0x366c);
    FIELD(LS640BriefLocY, 0x3670);
    FIELD(LS800BriefLocX, 0x3674);
    FIELD(LS800BriefLocY, 0x3678);
    FIELD(LS640BkgdName, 0x367c);
    FIELD(LS800BkgdName, 0x36bc);
    FIELD(LS800BkgdPal, 0x36fc);
#undef FIELD
}

template<class T> void list_record(Record& record, unsigned offset, TypeList<T>& list, bool load) {
    if (!load) {
        unsigned vtable = std::is_pointer_v<T> ? 0x7e4e18u : 0x7e4dd8u;
        field(record, offset, vtable, false);
        field(record, offset + 8, list.Capacity, false);
        field(record, offset + 12, list.IsInitialized, false);
        field(record, offset + 13, list.IsAllocated, false);
        field(record, offset + 16, list.Count, false);
        field(record, offset + 20, list.CapacityIncrement, false);
    }
    field(record, offset + 24, list.unknown_18, load);
}

void encode(ScenarioClass& self, Record& record) {
    if constexpr (sizeof(void*) == 4 && sizeof(wchar_t) == 2) {
        static_assert(sizeof(ScenarioClass) >= 0x3740);
        std::memcpy(record.data(), &self, record.size());
    } else {
        fields(self, record, false);
        list_record(record, 0x34d4, self.AllowableUnits, false);
        list_record(record, 0x34f0, self.AllowableUnitMaximums, false);
        list_record(record, 0x350c, self.DropshipUnitCounts, false);
        // The list Items addresses and resolved CSF pointer are reconstruction
        // artifacts, never restored by Load. Portable records leave them zero.
    }
}

template<class T> void reset_list(TypeList<T>& list) {
    std::destroy_at(&list);
    std::construct_at(&list);
}
void decode(ScenarioClass& self, Record& record) {
    // Unlike the original raw overwrite, release existing owned arrays before
    // reconstruction. The objects referenced by AllowableUnits remain borrowed.
    reset_list(self.AllowableUnits);
    reset_list(self.AllowableUnitMaximums);
    reset_list(self.DropshipUnitCounts);
    if constexpr (sizeof(void*) == 4 && sizeof(wchar_t) == 2) {
        std::memcpy(&self, record.data(), 0x34d4);
        std::memcpy(&self.AmbientOriginal, record.data() + 0x3528, record.size() - 0x3528);
    } else fields(self, record, true);
    list_record(record, 0x34d4, self.AllowableUnits, true);
    list_record(record, 0x34f0, self.AllowableUnitMaximums, true);
    list_record(record, 0x350c, self.DropshipUnitCounts, true);
    std::construct_at(&self.Random, 0u); // Discards saved RNG state, retaining its representation padding.
    self.PauseTimer.Start(0);
    self.MissionTimerTextCSF = const_cast<wchar_t*>(self.MissionTimerText[0] ?
        game::scenario_text(self.MissionTimerText, 344) : L"");
}

struct ResumeElapsed {
    SysElapsedTimerClass& timer;
    bool resume;
    ~ResumeElapsed() { if (resume) timer.Resume(); }
};
}

bool ScenarioClass::Save(IStream* stream) {
    const auto* io = game::scenario_runtime().stream;
    if (!stream || !io || !io->write || !io->pointer_id) return false;
    ResumeElapsed restore{ElapsedTimer, ElapsedTimer.IsTicking()};
    ElapsedTimer.Pause();
    Record record{};
    encode(*this, record);
    if (!io->write(io->context, stream, record.data(), record.size())) return false;
    const auto write = [&](const auto& value) { return io->write(io->context, stream, &value, 4); };
    if (!write(AllowableUnits.Count)) return false;
    for (int i = 0; i < AllowableUnits.Count; ++i) {
        unsigned id = 0;
        if (!io->pointer_id(io->context, AllowableUnits[i], id) || !write(id)) return false;
    }
    if (!write(AllowableUnitMaximums.Count)) return false;
    for (int i = 0; i < AllowableUnitMaximums.Count; ++i) if (!write(AllowableUnitMaximums[i])) return false;
    if (!write(DropshipUnitCounts.Count)) return false;
    for (int i = 0; i < DropshipUnitCounts.Count; ++i) if (!write(DropshipUnitCounts[i])) return false;
    return true;
}

bool ScenarioClass::Load(IStream* stream) {
    const auto* io = game::scenario_runtime().stream;
    if (!stream || !io || !io->read || !io->swizzle) return false;
    ElapsedTimer.Pause();
    Record record{};
    if (!io->read(io->context, stream, record.data(), record.size())) return false;
    // Resource labels must terminate before being passed to the string table.
    if (!std::memchr(record.data() + 0x11f8, 0, 32)) return false;
    decode(*this, record);
    const auto read = [&](auto& value) { return io->read(io->context, stream, &value, 4); };
    int count = 0;
    try {
        if (!read(count) || count < 0) return false;
        for (int i = 0; i < count; ++i) {
            unsigned id = 0;
            if (!read(id) || !AllowableUnits.AddItem(
                    reinterpret_cast<TechnoTypeClass*>(static_cast<std::uintptr_t>(id)))) return false;
        }
        if (!read(count) || count < 0) return false;
        for (int i = 0; i < count; ++i) {
            int value = 0;
            if (!read(value) || !AllowableUnitMaximums.AddItem(value)) return false;
        }
        if (!read(count) || count < 0) return false;
        for (int i = 0; i < count; ++i) {
            int value = 0;
            if (!read(value) || !DropshipUnitCounts.AddItem(value)) return false;
        }
    } catch (const std::bad_alloc&) { return false; }
    // Register only after the pointer vector has its final capacity: the
    // swizzler retains addresses of slots, so subsequent growth would dangle.
    for (int i = 0; i < AllowableUnits.Count; ++i) {
        const auto id = static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(AllowableUnits[i]));
        if (!io->swizzle(io->context, id, &AllowableUnits[i])) return false;
    }
    MissionTimerTextCSF = const_cast<wchar_t*>(MissionTimerText[0] ?
        game::scenario_text(MissionTimerText, 6608) : L"");
    ElapsedTimer.Resume(); // Original always resumes, including a paused save.
    return true;
}
