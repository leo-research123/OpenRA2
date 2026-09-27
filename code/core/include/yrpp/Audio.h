#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Helpers/CompileTime.h"

class RawFileClass;
class CCFileClass;
class VocClass;
struct AudioVolumeControl; // Original volume object; owned by audio initialization.

struct AudioIDXHeader {
    unsigned int Magic;
    unsigned int Version;
    unsigned int numSamples;
};

struct AudioIDXEntry { // assert (IDXHeader.version != 1);
    char Name[16];
    int Offset;
    int Size;
    unsigned int SampleRate;
    unsigned int Flags;
    unsigned int ChunkSize;

    bool operator < (const AudioIDXEntry& rhs) const {
        return _strcmpi(this->Name, rhs.Name) < 0;
    }
};

struct AudioSampleData {
    AudioSampleData() :
        Data(0),
        Format(0),
        SampleRate(0),
        NumChannels(0),
        BytesPerSample(0),
        ByteRate(0),
        BlockAlign(0),
        Flags(0)
    { }

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
    static AudioIDXData*& Instance; // Global VA: 0x0087E294; binding in compat.

    // Original 0x4011C0. Caller owns the returned library and deletes it.
    /// VA: 0x004011C0.
    static AudioIDXData* YRPP_FASTCALL Create(const char* pFilename, const char* pPath);
    AudioIDXData();
    AudioIDXData(const AudioIDXData&) = delete;
    AudioIDXData& operator=(const AudioIDXData&) = delete;
    // Releases members. Original 0x401580 also frees this; see compat binding.
    /// VA: 0x00401580.
    ~AudioIDXData();
    void ClearCurrentSample(); // 0x401910; preserves CurrentSampleSize.
    int YRPP_FASTCALL FindSampleIndex(const char* pName) const; // 0x4015C0
    const char* YRPP_FASTCALL GetSampleName(int index) const; // 0x401600
    int YRPP_FASTCALL GetSampleSize(int index) const; // 0x401620
    AudioSampleData* YRPP_FASTCALL GetSampleInformation(int index, AudioSampleData* pBuffer) const; // 0x401640
    // Existing original module operation, previously missing from this header.
    bool YRPP_FASTCALL OpenSample(int index); // 0x4016F0

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
    static AudioStream*& Instance; // Global VA: 0x00B1D4D8; binding in compat.

    // Dispatches through api/audio_backend.hpp; the host owns stream storage.
    bool YRPP_FASTCALL PlayWAV(const char* pFilename, bool resourceFile); // 0x407B60
    // These operations borrow backend-owned stream storage. Creation failure
    // returns null; no stream object is fabricated for an unavailable device.
    /// VA: 0x00407860
    static AudioStream* Create(int bufferSize, int flags) noexcept;
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
};

struct TauntDataStruct {
    DWORD tauntIdx : 4;
    DWORD countryIdx : 4;
};

struct AudioController
{
    void* Event; // Pointer to audio event (AudioEventTag) associated with this controller, type not implemented in YRpp as of current.
    DWORD Stamp;
    VocClass* EventType;
    AudioIDXData** AudioIndex;
    DWORD Unused; // 0x00405BE0 leaves this storage untouched.

    /// VA: 0x00405BE0
    AudioController() :
        Event(nullptr),
        Stamp(0),
        EventType(nullptr),
        AudioIndex(&AudioIDXData::Instance)
    { }

    // Event-dependent methods dispatch through the host AudioBackend.
    ~AudioController(); // 0x405C00

    // Stops the audio event associated with this controller instantly.
    void Stop(); // 0x405D40

    // Ends the audio event associated with this controller, allowing it to fade out.
    void End(); // 0x405FD0

    // Stops the looping of audio event associated with this controller instantly.
    void StopLooping(); // 0x405E80

    // Ends the looping of audio event associated with this controller, allowing it to fade out.
    void EndLooping(); // 0x406060

    void YRPP_FASTCALL SetEvent(void* event, VocClass* eventType); // 0x4060F0; ECX/EDX + stack

    void* GetEvent(); // 0x406130

    VocClass* GetEventType(); // 0x406170

    // Not actually AudioController function, adjusts volume of a given audio event.
    static void YRPP_FASTCALL AdjustAudioEventVolume(void* event, unsigned int volume); // Global VA: 0x004061D0

    // Not actually AudioController function, adjusts panning of a given audio event.
    static void YRPP_FASTCALL AdjustAudioEventPanning(void* event, unsigned int pan); // Global VA: 0x00406270

    // Not actually AudioController function, gets event type (VocClass) of given audio event.
    static VocClass* YRPP_FASTCALL GetEventType(void* event); // Global VA: 0x00406310
};
