#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Helpers/CompileTime.h"
#include <cstddef>

class RawFileClass;
class CCFileClass;
class VocClass;
struct AudioEvent;
struct AudioVolumeControl;  // Original volume object; owned by audio initialization.

// Circular links used by the original audio module (0x004072C0..0x00407480).
struct AudioLink {
    AudioLink* Next;
    AudioLink* Previous;
    AudioLink* Sentinel;
};

struct AudioIDXHeader {
    unsigned int Magic;
    unsigned int Version;
    unsigned int numSamples;
};

struct AudioIDXEntry {  // assert (IDXHeader.version != 1);
    char Name[16];
    int Offset;
    int Size;
    unsigned int SampleRate;
    unsigned int Flags;
    unsigned int ChunkSize;

    bool operator<(const AudioIDXEntry& rhs) const { return _strcmpi(this->Name, rhs.Name) < 0; }
};

struct AudioSampleData {
    AudioSampleData()
        : Data(0), Format(0), SampleRate(0), NumChannels(0), BytesPerSample(0), ByteRate(0), BlockAlign(0), Flags(0) {}

    unsigned int Data;
    unsigned int Format;
    unsigned int SampleRate;
    unsigned int NumChannels;
    unsigned int BytesPerSample;
    unsigned int ByteRate;
    unsigned int BlockAlign;
    unsigned int Flags;
};

class AudioIDXData {
public:
    static AudioIDXData*& Instance;  // Global VA: 0x0087E294; binding in compat.

    // Original 0x4011C0. Caller owns the returned library and deletes it.
    /// VA: 0x004011C0.
    static AudioIDXData* YRPP_FASTCALL Create(const char* pFilename, const char* pPath);
    AudioIDXData();
    AudioIDXData(const AudioIDXData&) = delete;
    AudioIDXData& operator=(const AudioIDXData&) = delete;
    // Releases members. Original 0x401580 also frees this; see compat binding.
    /// VA: 0x00401580.
    ~AudioIDXData();
    /// VA: 0x401910
    void ClearCurrentSample();  // Preserves CurrentSampleSize.
    /// VA: 0x4015C0
    int YRPP_FASTCALL FindSampleIndex(const char* pName) const;
    /// VA: 0x401600
    const char* YRPP_FASTCALL GetSampleName(int index) const;
    /// VA: 0x401620
    int YRPP_FASTCALL GetSampleSize(int index) const;
    /// VA: 0x401640
    AudioSampleData* YRPP_FASTCALL GetSampleInformation(int index, AudioSampleData* pBuffer) const;
    // Existing original module operation, previously missing from this header.
    /// VA: 0x4016F0
    bool YRPP_FASTCALL OpenSample(int index);

    AudioIDXEntry* Samples;
    int SampleCount;
    char Path[MAX_PATH];
    CCFileClass* BagFile;
    RawFileClass* ExternalFile;
    BOOL PathFound;
    RawFileClass* CurrentSampleFile;
    int CurrentSampleSize;
    DWORD unknown_120;
};

class Audio {
public:
    static void UpdateEvents(unsigned long long now) noexcept;
    /// VA: 0x00406F70
    static void AI() noexcept;
    static void SetStreamVolume(float volume) noexcept;
    // Passes of the original event update at 0x004041D0. The native playback
    // adapter supplies live event records in global-list order. These helpers
    // are not callable at that address with these signatures.
    static void EnforceEventLimits(AudioEvent* const* events, unsigned count) noexcept;
    static unsigned BuildPriorityBuckets(AudioEvent* const* events, unsigned count) noexcept;
    static void ClearPriorityBuckets() noexcept;
    // Status adapter: the original entry takes priority in ECX and returns a
    // nullable definition pointer. A miss here preserves output.
    /// VA: 0x00404E20
    static bool FindLowerPriority(int priority, VocClass*& output) noexcept;
    static void RemovePriority(VocClass& type) noexcept;

    // Native device-state reader for the selection at 0x004035F0 and admission
    // check at 0x00405B50. No playback storage/layout crosses this boundary.
    // A failed read/selection preserves output; callbacks must not throw.
    using ReadVoice = bool (*)(void*, unsigned, unsigned& flags, unsigned& level, int& priority) noexcept;
    static bool SelectVoice(unsigned count, void* context, ReadVoice read, int priority, unsigned& output) noexcept;

    /// VA: 0x00406F00
#if defined(RA2_YRPP_GAME)
    static void SuspendAll() { JMP_STD(0x406F00); }
#else
    static void SuspendAll() noexcept;
#endif
    /// VA: 0x00406F40
#if defined(RA2_YRPP_GAME)
    static void ResumeAll() { JMP_STD(0x406F40); }
#else
    static void ResumeAll() noexcept;
#endif

    /// VA: 0x00408390
    static void FadeSounds() noexcept;
    /// VA: 0x00408410
    static bool SoundsFaded() noexcept;
    /// VA: 0x00408200
    static void HoldSounds() noexcept;
    /// VA: 0x00408230
    static void ReleaseSounds() noexcept;
    /// VA: 0x00408270
    static void PauseSounds() noexcept;
    /// VA: 0x004082F0
    static void ResumeSounds() noexcept;
    /// VA: 0x004083D0
    static void RestoreSounds() noexcept;

    /// VA: 0x00407000
    static bool IsAvailable() noexcept;
    /// VA: 0x004093B0
    static unsigned long long GetTime() noexcept;
    // Native uses the original timeGetTime wrap-extension algorithm with the
    // existing authoritative millisecond reader; no second platform clock.
    /// VA: 0x004093F0
    static unsigned long long GetExtendedTime() noexcept;
    /// Global VA: 0x00816320.
    static unsigned long long& LastTime;
    /// Global VA: 0x0087E84C.
    static unsigned& TimeReadSerial;
    /// Global VA: 0x0087E740.
    static AudioVolumeControl*& MasterVolume;
    /// Global VA: 0x0087E750.
    static AudioVolumeControl*& SpeechVolume;
    /// Global VA: 0x00A8ED64.
    static bool& Quiet;

    // Original 0x408610. Leaves the file at the data payload; does not decode it.
    // WAV only. AUD support is intentionally excluded, not a pending dependency.
    /// VA: 0x00408610.
    static bool YRPP_FASTCALL ReadWAVFile(RawFileClass* pFile, AudioSampleData* pAudioSample, int* pDataSize);
};

class AudioStream {
public:
    static AudioStream*& Instance;  // Global VA: 0x00B1D4D8; binding in compat.

    /// VA: 0x407B60
    bool YRPP_FASTCALL PlayWAV(const char* pFilename, bool resourceFile);
    // Creation failure returns null when the audio device is unavailable.
    /// VA: 0x00407860
    // Legacy arguments are the low/high halves of a millisecond duration.
    static AudioStream* Create(int durationLow, int durationHigh) noexcept;
    /// VA: 0x00407A90
    void Destroy() noexcept;
    /// VA: 0x00408080
    void YRPP_FASTCALL SetName(const char* name) noexcept;
    /// VA: 0x00407B40
    void YRPP_FASTCALL SetVolumeControl(AudioVolumeControl* control) noexcept;
    /// VA: 0x00407B50
    void YRPP_FASTCALL SetSecondaryVolumeControl(AudioVolumeControl* control) noexcept;
    /// VA: 0x00407F40
    void Stop() noexcept;
    /// VA: 0x00407FB0
    void Pause() noexcept;
    /// VA: 0x00408000
    void Resume() noexcept;
    /// VA: 0x00408070
    bool IsPlaying() const noexcept;
    /// VA: 0x00408140
    unsigned long long EndTime() const noexcept;
    /// VA: 0x004080B0
    void RestoreVolume() noexcept;
    /// VA: 0x004080C0
    void Fade() noexcept;
    /// VA: 0x004080D0
    bool IsFading() const noexcept;
    // Native service tick for refill and completion on the owner thread.
    void Update(unsigned long long now) noexcept;
    float GetGain() const noexcept;

    AudioLink Link{};
    unsigned Flags = 0;
    void* Device = nullptr;
    void* Playback = nullptr;
    unsigned long long Duration = 0;
    unsigned long long Ended = 0;
    unsigned long long Started = 0;
    void* Loader = nullptr;
    void* Buffers[2]{};
    unsigned unknown_3C[8]{};
    AudioSampleData Sample{};
    RawFileClass* File = nullptr;
    unsigned PauseCount = 0;
    unsigned unknown_84 = 0;
    unsigned Busy = 0;
    unsigned unknown_8C = 0;
    unsigned ChunkRemaining = 0;
    unsigned TotalBytes = 0;
    unsigned FileOffset = 0;
    unsigned Remaining = 0;
    unsigned Preload = 0;
    char Name[84]{};
};

struct TauntDataStruct {
    DWORD tauntIdx : 4;
    DWORD countryIdx : 4;
};

struct AudioController {
    void* Event;  // Original AudioEvent associated with this controller.
    DWORD Stamp;
    VocClass* EventType;
    AudioIDXData** AudioIndex;
    DWORD Unused;  // The constructor at 0x00405BE0 leaves this storage untouched.

    /// VA: 0x00405BE0
    AudioController() : Event(nullptr), Stamp(0), EventType(nullptr), AudioIndex(&AudioIDXData::Instance) {}

    /// VA: 0x405C00
    ~AudioController();

    // Stops the audio event associated with this controller instantly.
    /// VA: 0x405D40
    void Stop();

    // Ends the audio event associated with this controller, allowing it to fade out.
    /// VA: 0x405FD0
    void End();

    // Stops the looping of audio event associated with this controller instantly.
    /// VA: 0x405E80
    void StopLooping();

    // Ends the looping of audio event associated with this controller, allowing it to fade out.
    /// VA: 0x406060
    void EndLooping();

    /// VA: 0x4060F0
    void YRPP_FASTCALL SetEvent(void* event, VocClass* eventType);  // ECX/EDX + stack

    /// VA: 0x406130
    void* GetEvent();

    /// VA: 0x406170
    VocClass* GetEventType();

    // Not actually AudioController function, adjusts volume of a given audio event.
    /// VA: 0x004061D0
    static void YRPP_FASTCALL AdjustAudioEventVolume(void* event, unsigned int volume);

    // Not actually AudioController function, adjusts panning of a given audio event.
    /// VA: 0x00406270
    static void YRPP_FASTCALL AdjustAudioEventPanning(void* event, unsigned int pan);

    // Not actually AudioController function, gets event type (VocClass) of given audio event.
    /// VA: 0x00406310
    static VocClass* YRPP_FASTCALL GetEventType(void* event);
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AudioStream) == 0xF8);
static_assert(offsetof(AudioStream, Ended) == 0x20);
static_assert(offsetof(AudioStream, File) == 0x7C);
static_assert(offsetof(AudioStream, PauseCount) == 0x80);
static_assert(offsetof(AudioStream, Busy) == 0x88);
static_assert(offsetof(AudioStream, Name) == 0xA4);
#endif
