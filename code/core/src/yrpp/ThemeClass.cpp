// Theme records and playback state, calibrated to 0x00720480..0x007212A0.
#include "yrpp/ThemeClass.h"
#include "yrpp/Audio.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/Randomizer.h"
#include "audio_session.hpp"
#include "yrpp/SessionClass.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>

#if !defined(RA2_YRPP_GAME)
namespace {
ThemeClass theme;
bool scores = true;
char filename[512]{};
}
ThemeClass& ThemeClass::Instance = theme;
bool& ThemeClass::ScoresPresen = scores;
ThemeClass::ThemeClass() noexcept
    : CurrentTheme(-1), LastTheme(-1), QueuedTheme(-1), Volume(255), IsScoreRepeat(false), IsFading(false),
      IsScoreShuffle(true), Themes{}, Stream(nullptr) {}
const char* ThemeClass::GetID(unsigned int index) const {
    return index < unsigned(Themes.Count) ? Themes[index]->ID : "<none>";
}
const char* ThemeClass::GetName(unsigned int index) const {
    return index < unsigned(Themes.Count) ? Themes[index]->ID : "No theme";
}
const char* ThemeClass::GetFilename(unsigned int index) const {
    if (index >= unsigned(Themes.Count)) return "";
    std::snprintf(filename, sizeof(filename), "%s.WAV", static_cast<const char*>(Themes[index]->Sound));
    return filename;
}
const wchar_t* ThemeClass::GetUIName(unsigned int index) const {
    return index < unsigned(Themes.Count) ? Themes[index]->UIName : L"";
}
int ThemeClass::GetLength(unsigned int index) const {
    return index < unsigned(Themes.Count) ? int(Themes[index]->Length) : 0;
}
bool ThemeClass::IsNormal(int index) const { return index >= 0 && index < Themes.Count && Themes[index]->Normal; }
bool ThemeClass::IsAvailable(int index) const {
    if (index == -2 || index == -3) return true;
    if (index < 0 || index >= Themes.Count) return false;
    const auto& item = *Themes[index];
    if (!item.Exists || !item.Normal) return false;
    if (HouseClass::CurrentPlayer && HouseClass::CurrentPlayer->Type && item.Side != -1 &&
        HouseClass::CurrentPlayer->Type->SideIndex != item.Side)
        return false;
    return SessionClass::Instance.GameMode != GameMode::Campaign || !ScenarioClass::Instance ||
           ScenarioClass::Instance->TechLevel >= item.Scenario;
}
int ThemeClass::FindIndex(const char* id) const {
    if (id && *id)
        for (int i = 0; i < Themes.Count; ++i)
            if (!_strcmpi(id, Themes[i]->ID)) return i;
    return -1;
}
int ThemeClass::LoadFromINI(CCINIClass* ini) noexcept {
    if (!ini || !ini->GetSection("Themes")) return false;
    try {
        const int count = ini->GetKeyCount("Themes");
        for (int i = 0; i < count; ++i) {
            char name[32];
            if (!ini->ReadString("Themes", ini->GetKeyName("Themes", i), "", name, sizeof(name))) continue;
            const int index = FindIndex(name);
            ThemeControl* item = index < 0 ? nullptr : Themes[index];
            if (!item) {
                auto owned = std::make_unique<ThemeControl>();
                owned->ID = name;
                owned->Scenario = 0;
                owned->Length = 0;
                owned->Normal = true;
                owned->Repeat = owned->Exists = false;
                owned->Side = -1;
                if (!Themes.AddItem(owned.get())) return false;
                item = owned.release();
            }
            char text[256];
            ini->ReadString(name, "Sound", "", text, sizeof(text));
            const char* sound = text;
            while (*sound == '$' || *sound == '#')
                ++sound;
            item->Sound = sound;
            item->Scenario = ini->ReadInteger(name, "Scenario", item->Scenario);
            item->Normal = ini->ReadBool(name, "Normal", item->Normal);
            item->Repeat = ini->ReadBool(name, "Repeat", item->Repeat);
            ini->ReadString(name, "Side", "", text, sizeof(text));
            if (!_strcmpi(text, "GDI") || !_strcmpi(text, "Allied"))
                item->Side = 0;
            else if (!_strcmpi(text, "Nod") || !_strcmpi(text, "Soviet"))
                item->Side = 1;
            else if (!_strcmpi(text, "ThirdSide") || !_strcmpi(text, "Yuri"))
                item->Side = 2;
            if (item->Normal) ini->ReadStringtableEntry(name, "Name", item->UIName.data(), 0x40);
        }
        if (!Stream && Audio::IsAvailable()) Stream = AudioStream::Create(20000, 0);
        if (Stream) {
            Stream->SetName("Music Stream");
            if (auto* session = game::audio_session())
                Stream->SetVolumeControl(reinterpret_cast<AudioVolumeControl*>(&session->music_control));
        }
        return Stream != nullptr;
    } catch (...) {
        return false;
    }
}
void ThemeClass::Scan() {
    if (!ScoresPresen || !Audio::IsAvailable() || Audio::Quiet) return;
    for (int i = 0; i < Themes.Count; ++i) {
        auto& item = *Themes[i];
        CCFileClass file(GetFilename(unsigned(i)));
        item.Exists = file.Exists();
        if (!item.Exists || !file.Open(FileAccessMode::Read)) continue;
        AudioSampleData info;
        int bytes = 0;
        if (Audio::ReadWAVFile(&file, &info, &bytes) && bytes > 0 && info.ByteRate)
            item.Length = float(unsigned(bytes) / info.ByteRate);
    }
}
int ThemeClass::GetRandomIndex(unsigned last) const {
    if (!Themes.Count) return -1;
    if (last < unsigned(Themes.Count) && (IsScoreRepeat || Themes[last]->Repeat)) return int(last);
    if (IsScoreShuffle) {
        for (int tries = 0; tries < 1000; ++tries) {
            const int index = Randomizer::Global.RandomRanged(0, Themes.Count - 1);
            if (unsigned(index) != last && IsAvailable(index)) return index;
        }
    } else {
        int index = last < unsigned(Themes.Count) ? int(last) : -1;
        for (int i = 0; i <= Themes.Count; ++i) {
            if (++index >= Themes.Count) index = 0;
            if (IsAvailable(index)) return index;
        }
    }
    return 0;
}
void ThemeClass::Queue(int index) {
    if (!ScoresPresen || !Audio::IsAvailable() || Audio::Quiet) return;
    QueuedTheme = index;
    if (index != -1 && index != -2 && index != LastTheme && Stream && Stream->IsPlaying()) {
        Stream->Fade();
        IsFading = true;
    }
}
int ThemeClass::Play(int index) {
    if (!ScoresPresen || !Audio::IsAvailable() || Audio::Quiet) return CurrentTheme;
    if (Stream && Stream->IsPlaying() && LastTheme == index && !IsFading) return CurrentTheme;
    Stop();
    IsFading = false;
    if (index < 0 || index >= Themes.Count || Volume <= 0) return CurrentTheme;
    LastTheme = index;
    if (Stream && Stream->PlayWAV(GetFilename(unsigned(index)), true)) CurrentTheme = index;
    if (IsScoreRepeat || Themes[index]->Repeat) QueuedTheme = index;
    return CurrentTheme;
}
void ThemeClass::Stop(bool fade) {
    if (fade && Stream && Stream->IsPlaying()) {
        Stream->Fade();
        IsFading = true;
    } else {
        if (Stream) Stream->Stop();
        IsFading = false;
    }
    CurrentTheme = LastTheme = QueuedTheme = -1;
}
void ThemeClass::Suspend() {
    if (CurrentTheme == -1) return;
    if (Stream) Stream->Stop();
    QueuedTheme = LastTheme;
    CurrentTheme = LastTheme = -1;
}
void ThemeClass::AI() {
    if (!ScoresPresen || !Audio::IsAvailable() || Audio::Quiet || Volume <= 0) return;
    if (IsFading && Stream && !Stream->IsFading()) Stream->Stop();
    if (Stream && Stream->IsPlaying()) return;
    if (QueuedTheme == -1 || QueuedTheme == -3) return;
    const int next = QueuedTheme == -2 ? GetRandomIndex(unsigned(LastTheme)) : QueuedTheme;
    // Compact installations may omit all music. Avoid reopening a missing
    // fallback file on every scheduler tick when random selection has no match.
    if (QueuedTheme == -2 && !IsAvailable(next)) {
        Stop();
        return;
    }
    Play(next);
    QueuedTheme = -2;
}
void ThemeClass::Clear() noexcept {
    Stop();
    if (Stream) {
        Stream->Destroy();
        Stream = nullptr;
    }
    while (Themes.Count) {
        auto* item = Themes[Themes.Count - 1];
        Themes.RemoveItem(Themes.Count - 1);
        delete item;
    }
}
#endif
