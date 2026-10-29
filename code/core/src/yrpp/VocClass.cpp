// Original sound definition registry and INI reader, 0x007510D0/0x00750440.
#include "yrpp/VocClass.h"
#include "yrpp/CCINIClass.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <new>

#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<VocClassRecord*> definitions;
bool enabled = true;
struct Defaults {
    double volume = 100, minimum = 20;
    int range = 10, limit = 3;
    unsigned control = 0, type = 0x20;
} defaults;
struct Flag {
    const char* name;
    unsigned value;
};
constexpr Flag controls[] = {{"ALL", 4},          {"LOOP", 1},      {"RANDOM", 2},   {"PREDELAY", 8},
                             {"INTERRUPT", 0x10}, {"ATTACK", 0x20}, {"DECAY", 0x40}, {"AMBIENT", 0x80}};
constexpr Flag types[] = {{"AMBIENT", 0x1000}, {"VIOLENT", 1},   {"MOVEMENT", 2},    {"QUIET", 4},
                          {"LOUD", 8},         {"GLOBAL", 0x10}, {"SCREEN", 0x20},   {"LOCAL", 0x40},
                          {"PLAYER", 0x80},    {"NORMAL", 0},    {"GUN_SHY", 0x200}, {"NOISE_SHY", 0x100},
                          {"UNSHROUD", 0x400}, {"SHROUD", 0x800}};
char* token(char*& cursor) {
    cursor += std::strspn(cursor, " \t\r\n");
    if (!*cursor) return nullptr;
    char* result = cursor;
    cursor += std::strcspn(cursor, " \t\r\n");
    if (*cursor) *cursor++ = 0;
    return result;
}
template <std::size_t N>
unsigned flags(CCINIClass& ini, const char* section, const char* key, unsigned fallback, const Flag (&table)[N],
               bool type = false) {
    char text[2048];
    if (!ini.ReadString(section, key, "", text, sizeof(text))) return fallback;
    char* cursor = text;
    char* word = token(cursor);
    if (!word) return fallback;
    unsigned result = type ? 0x20 : 0;
    do {
        for (const auto& flag : table)
            if (!_strcmpi(word, flag.name)) {
                if (type && (flag.value & 0x60)) result &= ~0x60u;
                if (type && (flag.value & 0xC00)) result &= ~0xC00u;
                result |= flag.value;
                break;
            }
    } while ((word = token(cursor)));
    return result;
}
void pair(CCINIClass& ini, const char* section, const char* key, int& low, int& high) {
    char text[2048];
    ini.ReadString(section, key, "", text, sizeof(text));
    char* cursor = text;
    auto* word = token(cursor);
    low = word ? std::atoi(word) : 0;
    word = token(cursor);
    high = word ? std::atoi(word) : low;
    if (high < low) std::swap(low, high);
}
}
DynamicVectorClass<VocClassRecord*>& VocClass::Array = definitions;
bool& VocClass::VoicesEnabled = enabled;
int YRPP_FASTCALL VocClass::FindIndex(const char* name) {
    if (name)
        for (int i = 0; i < Array.Count; ++i) {
            const auto* sound = Get(i);
            if (!_strcmpi(name, sound ? sound->Name : "Invalid Voc")) return i;
        }
    return -1;
}
VocClassRecord* YRPP_FASTCALL VocClass::FindRecord(const char* name) noexcept {
    // 0x751520 rejects <none>, while the scalar index lookup does not.
    if (!name || !_strcmpi(name, "<none>")) return nullptr;
    const int index = FindIndex(name);
    return index >= 0 ? Array[index] : nullptr;
}
int VocClassRecord::GetArrayIndex() const noexcept {
    for (int i = 0; i < VocClass::Array.Count; ++i)
        if (VocClass::Array[i] == this) return i;
    return -1;
}
bool VocClassRecord::LoadFromINI(CCINIClass* ini) noexcept { return Sound && Sound->ReadDefinition(ini); }
bool VocClass::ReadDefinition(CCINIClass* ini) noexcept {
    if (!ini || !ini->GetSection(Name)) return false;
    try {
        NumSamples = 0;
        SamplesOK = 1;
        char text[2048];
        ini->ReadString(Name, "Sounds", "", text, sizeof(text));
        char* cursor = text;
        while (auto* name = token(cursor)) {
            while (*name == '$' || *name == '#')
                ++name;
            if (NumSamples == 32) break;
            const int index = AudioIDXData::Instance ? AudioIDXData::Instance->FindSampleIndex(name) : -1;
            if (index < 0)
                SamplesOK = 0;
            else
                SampleIndex[NumSamples++] = index;
        }
        Volume.SetVolume(int(std::clamp(ini->ReadDouble(Name, "Volume", defaults.volume) * 0.01, 0.0, 1.0) * 0x4000));
        MinVolume = float(std::clamp(ini->ReadDouble(Name, "MinVolume", defaults.minimum) * 0.01, 0.0, 1.0));
        ini->ReadString(Name, "Priority", "NORMAL", text, sizeof(text));
        Priority = SoundPriority::Normal;
        const char* priorities[] = {"LOWEST", "LOW", "NORMAL", "HIGH", "CRITICAL"};
        for (int i = 0; i < 5; ++i)
            if (!_strcmpi(text, priorities[i])) Priority = SoundPriority(i);
        Control = SoundControl(flags(*ini, Name, "Control", defaults.control, controls));
        Type = SoundType(flags(*ini, Name, "Type", defaults.type, types, true));
        Limit = ini->ReadInteger(Name, "Limit", defaults.limit);
        Loop = ini->ReadInteger(Name, "Loop", 0);
        Range = ini->ReadInteger(Name, "Range", defaults.range);
        // 0x00406570 supplies one attack/decay sample when the corresponding
        // control is present, and clears the count when it is absent.
        Attack = (unsigned(Control) & 0x20)
                     ? std::clamp(ini->ReadInteger(Name, "Attack", 1), 1, std::max(1, NumSamples))
                     : 0;
        Decay = (unsigned(Control) & 0x40)
                    ? std::clamp(ini->ReadInteger(Name, "Decay", 1), 1, std::max(1, NumSamples - Attack))
                    : 0;
        VShift = std::clamp(ini->ReadInteger(Name, "VShift", 0), 0, 100);
        pair(*ini, Name, "Delay", MinDelay, MaxDelay);
        pair(*ini, Name, "FShift", MinFDelta, MaxFDelta);
        return true;
    } catch (...) {
        return false;
    }
}
bool YRPP_FASTCALL VocClass::LoadAllFromINI(CCINIClass* ini) noexcept {
    if (!ini || !ini->GetSection("SoundList")) return false;
    try {
        defaults = {};
        defaults.volume = ini->ReadDouble("Defaults", "Volume", 100);
        defaults.minimum = ini->ReadDouble("Defaults", "MinVolume", 20);
        defaults.limit = ini->ReadInteger("Defaults", "Limit", 3);
        defaults.range = ini->ReadInteger("Defaults", "Range", 10);
        defaults.control = flags(*ini, "Defaults", "Control", 0, controls);
        defaults.type = flags(*ini, "Defaults", "Type", 0x20, types, true);
        bool result = false;
        const int count = ini->GetKeyCount("SoundList");
        for (int i = 0; i < count; ++i) {
            char name[32];
            result = false;
            if (!ini->ReadString("SoundList", ini->GetKeyName("SoundList", i), "", name, sizeof(name))) continue;
            auto* sound = Find(name);
            if (!sound) {
                sound = new (std::nothrow) VocClass{};
                if (!sound) return false;
                auto* record = new (std::nothrow) VocClassRecord{sound};
                if (!record) {
                    delete sound;
                    return false;
                }
                try {
                    if (!Array.AddItem(record)) {
                        delete record;
                        delete sound;
                        return false;
                    }
                } catch (...) {
                    delete record;
                    delete sound;
                    throw;
                }
                std::snprintf(sound->Name, sizeof(sound->Name), "%s", name);
                sound->Header.Next = sound->Header.Prev = &sound->Header;
                sound->Volume.unknown_int_C = 1073741;
                sound->Volume.unknown_int_10 = 1000;
                sound->Volume.unknown_int_1C = 0x4000;
            }
            result = Array[FindIndex(name)]->LoadFromINI(ini);
        }
        return result;
    } catch (...) {
        return false;
    }
}
void VocClass::DeleteAll() noexcept {
    Audio::ClearPriorityBuckets();
    ClearPositionedSounds();
    while (Array.Count) {
        auto* record = Array[Array.Count - 1];
        Array.RemoveItem(Array.Count - 1);
        if (record) {
            delete record->Sound;
            delete record;
        }
    }
}
#endif
