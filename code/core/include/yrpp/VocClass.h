/*
    Sound effects!
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Audio.h"

#include "yrpp/Helpers/CompileTime.h"
#include <cstddef>

struct VocClassHeader {
    VocClassHeader* Next;
    VocClassHeader* Prev;
    DWORD Magic;
};

struct VolumeStruct  //pretty uncreative name, but it's all I can come up with atm
{
    /// VA: 0x004072B0
    int GetVolume() const;
    /// VA: 0x00407150
    void SetVolume(int value);
    /// VA: 0x00407100
    void YRPP_FASTCALL Initialize(unsigned value) noexcept;
    /// VA: 0x00407170
    void YRPP_FASTCALL SetTarget(unsigned value) noexcept;
    /// VA: 0x004071C0
    bool Update() noexcept;
    // Native callers can use the scheduling sample already obtained from Audio.
    // These explicit-time helpers are not original executable entry points.
    void InitializeAt(unsigned value, unsigned long long now) noexcept;
    void SetTargetAt(unsigned value, unsigned long long now) noexcept;
    bool UpdateAt(unsigned long long now) noexcept;
    // Reconstruction-only accessor, not an original executable entry point.
    // Read the integer part of the existing Q16 current-volume field.
    unsigned GetCurrentVolume() const noexcept { return unknown_4 >> 16; }
    int Volume;          // Flags: bit 0 applies immediately, bit 1 marks a change.
    DWORD unknown_4;     // Current Q16 volume.
    int unknown_int_8;   // Target Q16 value; unknown_4 holds the current value.
    int unknown_int_C;   // default is 1073741
    int unknown_int_10;  // default is 1000
    int unknown_int_14;  // default is 0
    DWORD nknown_18;     // Low word of the last ramp-update time.
    int unknown_int_1C;  // High word of the last ramp-update time.
};

// Original audio event storage initialized by 0x00405190 (0x280 bytes on x86).
// Native PCM ownership is kept outside this record.
struct AudioEvent {
    AudioLink GlobalLink{};
    AudioLink DefinitionLink{};
    unsigned Flags = 0;
    unsigned State = 0;
    unsigned ResumeState = 0;
    VocClass* type = nullptr;
    void* Samples[32]{};
    int SampleCount = 0;
    int CurrentSample = 0;
    void* Playback = nullptr;
    unsigned unknown_B4 = 0;
    VolumeStruct gain{};
    unsigned unknown_D8[2]{};
    VolumeStruct pitch_gain{};
    unsigned unknown_100[2]{};
    VolumeStruct pan_gain{};
    unsigned unknown_128[4]{};
    unsigned stamp = 0;
    unsigned unknown_13C = 0;
    unsigned long long deadline = 0;
    void* CurrentDescriptor = nullptr;
    int PitchPercent = 100;
    int VolumeVariation = 0;
    int PriorityBias = 0;
    unsigned PauseCount = 0;
    unsigned unknown_15C = 0;
    int Order[32]{};
    int Remaining = 0;
    int Cycles = 0;
    int DelayedOrder[32]{};
    int DelayedCount = 0;
    int DelayedPasses = 0;
    int DelayedCursor = 0;
    int DelayedChoice = 0;
    AudioController* controller = nullptr;
    unsigned unknown_27C = 0;

    // Native slot adapters for the original sample selectors. The caller supplies
    // cache indices/storage; all selection state belongs to this event.
    // A false selection preserves output. These adapters have native signatures.
    using RandomRange = int (*)(void*, int, int);
    /// VA: 0x004048B0
    void PrepareSamples(int (&indices)[32], RandomRange random, void* context) noexcept;
    /// VA: 0x00404700
    bool FirstSample(RandomRange random, void* context, int& output) noexcept;
    /// VA: 0x004047B0
    bool NextSample(RandomRange random, void* context, int& output) noexcept;
    /// VA: 0x00404BB0
    bool NextDelayedSample(RandomRange random, void* context, int& output) noexcept;
    void Release() noexcept;
    bool StartSample() noexcept;
    bool Finish() noexcept;
    void ApplyGain() noexcept;
    bool AcquireDevice() noexcept;
    void Complete(unsigned long long now) noexcept;
};

class CCINIClass;
// The original index array stores a pointer to this one-pointer record.
// The 0x148-byte definition is owned separately by the audio module.
struct VocClassRecord {
    VocClass* Sound;
    /// VA: 0x007515C0
#if defined(RA2_YRPP_GAME)
    int GetArrayIndex() const noexcept JMP_THIS(0x7515C0);
#else
    int GetArrayIndex() const noexcept;
#endif
    /// VA: 0x00750440
#if defined(RA2_YRPP_GAME)
    bool LoadFromINI(CCINIClass* ini) JMP_THIS(0x750440);
#else
    bool LoadFromINI(CCINIClass* ini) noexcept;
#endif
};

class VocClass {
public:
    /// Global VA: 0x00B1D378.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<VocClassRecord*>, Array, 0xB1D378u)
    DEFINE_REFERENCE(bool, VoicesEnabled, 0x8464ACu)
#else
    static DynamicVectorClass<VocClassRecord*>& Array;
    static bool& VoicesEnabled;
#endif
    static VocClass* Get(int index) noexcept {
        return index >= 0 && index < Array.Count && Array[index] ? Array[index]->Sound : nullptr;
    }
    static VocClass* Find(const char* name) { return Get(FindIndex(name)); }
    /// VA: 0x00751520
#if defined(RA2_YRPP_GAME)
    static VocClassRecord* YRPP_FASTCALL FindRecord(const char* name) noexcept JMP_STD(0x751520);
#else
    static VocClassRecord* YRPP_FASTCALL FindRecord(const char* name) noexcept;
#endif
    /// VA: 0x007514D0
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL FindIndex(const char* name) JMP_STD(0x7514D0);
#else
    static int YRPP_FASTCALL FindIndex(const char* name);
#endif
    /// VA: 0x007510D0
#if defined(RA2_YRPP_GAME)
    static bool YRPP_FASTCALL LoadAllFromINI(CCINIClass* ini) JMP_STD(0x7510D0);
#else
    static bool YRPP_FASTCALL LoadAllFromINI(CCINIClass* ini) noexcept;
#endif
    /// VA: 0x007513F0
#if defined(RA2_YRPP_GAME)
    static void DeleteAll() JMP_STD(0x7513F0);
#else
    static void DeleteAll() noexcept;
#endif
    /// VA: 0x00750EC0
#if defined(RA2_YRPP_GAME)
    static void UpdatePositionedSounds() JMP_STD(0x750EC0);
#else
    static void UpdatePositionedSounds() noexcept;
    static void ClearPositionedSounds() noexcept;
#endif

    /* Play a sound independant of the position.
        n = Index of VocClass in Array to be played
        Volume = 0.0f to 1.0f
        Panning = 0x0000 (left) to 0x4000 (right) (0x2000 is center)
    */
    /// VA: 0x00750920.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlayGlobal(int n, int Panning, float Volume, AudioController* pCtrl = nullptr)
        JMP_STD(0x750920);
#else
    static void YRPP_FASTCALL PlayGlobal(int n, int Panning, float Volume, AudioController* pCtrl = nullptr);
#endif

    /* Play a sound at a certain Position.
        n = Index of VocClass in Array to be played
    */
    /// VA: 0x007509E0.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlayAt(int n, const CoordStruct& coords, AudioController* pCtrl = nullptr)
        JMP_STD(0x7509E0);
#else
    static void YRPP_FASTCALL PlayAt(int n, const CoordStruct& coords, AudioController* pCtrl = nullptr);
#endif

    /// VA: 0x00750D40
#if defined(RA2_YRPP_GAME)
    static void* YRPP_FASTCALL UpdatePosition(const CoordStruct* coords, AudioController* controller) {
        JMP_STD(0x750D40);
    }
#else
    static void* YRPP_FASTCALL UpdatePosition(const CoordStruct* coords, AudioController* controller);
#endif

    // calls the one above ^ - probably sanity checks and whatnot
    /// VA: 0x00750E20.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL PlayIndexAtPos(int n, const CoordStruct& coords, int a3 = 0) JMP_STD(0x750E20);
#else
    static void YRPP_FASTCALL PlayIndexAtPos(int n, const CoordStruct& coords, int a3 = 0);
#endif

    /// VA: 0x00750F40
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL StopIndexAtPos(const CoordStruct& coords, int mask) { JMP_STD(0x750F40); }
#else
    static void YRPP_FASTCALL StopIndexAtPos(const CoordStruct& coords, int mask);
#endif

    // Properties

public:
    VocClassHeader Header;
    int SamplesOK;  //0 or 1, determines whether all samples are OK to use
    SoundControl Control;
    SoundType Type;
    VolumeStruct Volume;
    DWORD unknown_38;
    DWORD unknown_3C;
    SoundPriority Priority;
    unsigned EventCount;
    int Limit;        //as in sound.ini
    int Loop;         //as in sound.ini
    int Range;        //as in sound.ini
    float MinVolume;  //as in sound.ini
    int MinDelay;     //as in sound.ini
    int MaxDelay;     //as in sound.ini
    int MinFDelta;    //as in sound.ini
    int MaxFDelta;    //as in sound.ini
    int VShift;       //as in sound.ini
    char Name[0x20];  //as in sound.ini
    AudioLink EventList;
    unsigned LimitPass;
    unsigned PriorityPass;
    int EffectivePriority;
    AudioLink PriorityLink;
    unsigned VolumeBucket;

    int SampleIndex[0x20];

    int NumSamples;
    int Attack;
    int Decay;
    DWORD unknown_140;
    DWORD unknown_144;

private:
    friend struct VocClassRecord;
    bool ReadDefinition(CCINIClass* ini) noexcept;
    VocClass() = default;
    ~VocClass() = default;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(VocClass) == 0x148);
static_assert(sizeof(VocClassRecord) == 4);
static_assert(sizeof(AudioLink) == 0xC);
static_assert(sizeof(AudioEvent) == 0x280);
static_assert(offsetof(AudioEvent, type) == 0x24);
static_assert(offsetof(AudioEvent, gain) == 0xB8);
static_assert(offsetof(AudioEvent, pitch_gain) == 0xE0);
static_assert(offsetof(AudioEvent, pan_gain) == 0x108);
static_assert(offsetof(AudioEvent, stamp) == 0x138);
static_assert(offsetof(AudioEvent, deadline) == 0x140);
static_assert(offsetof(AudioEvent, Order) == 0x160);
static_assert(offsetof(AudioEvent, Remaining) == 0x1E0);
static_assert(offsetof(AudioEvent, DelayedOrder) == 0x1E8);
static_assert(offsetof(AudioEvent, controller) == 0x278);
static_assert(offsetof(VocClass, EventList) == 0x8C);
static_assert(offsetof(VocClass, PriorityLink) == 0xA4);
static_assert(offsetof(VocClass, SampleIndex) == 0xB4);
#endif
