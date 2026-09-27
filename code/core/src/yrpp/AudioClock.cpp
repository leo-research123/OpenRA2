// Original audio module 0x004093B0/0x004093F0. Native deliberately selects
// the original wrapping-millisecond fallback with the shared SystemTimer
// authority, not a second QueryPerformanceCounter/platform source.
#include "yrpp/Audio.h"
#include "yrpp/Timer.h"
#if defined(RA2_AUDIO_GAME)
unsigned long long& Audio::LastTime=*reinterpret_cast<unsigned long long*>(0x816320);
unsigned& Audio::TimeReadSerial=*reinterpret_cast<unsigned*>(0x87E84C);
AudioVolumeControl*& Audio::MasterVolume=*reinterpret_cast<AudioVolumeControl**>(0x87E740);
AudioVolumeControl*& Audio::SpeechVolume=*reinterpret_cast<AudioVolumeControl**>(0x87E750);
bool& Audio::Quiet=*reinterpret_cast<bool*>(0xA8ED64);
unsigned long long Audio::GetTime() noexcept {return reinterpret_cast<unsigned long long(YRPP_CDECL*)()>(0x4093B0)();}
unsigned long long Audio::GetExtendedTime() noexcept {return reinterpret_cast<unsigned long long(YRPP_CDECL*)()>(0x4093F0)();}
#else
namespace {
constinit unsigned long long last_time=0x100000000ULL;
constinit unsigned serial=0;
AudioVolumeControl* master{};AudioVolumeControl* speech{};bool quiet{};
}
unsigned long long& Audio::LastTime=last_time;
unsigned& Audio::TimeReadSerial=serial;
AudioVolumeControl*& Audio::MasterVolume=master;
AudioVolumeControl*& Audio::SpeechVolume=speech;
bool& Audio::Quiet=quiet;
unsigned long long Audio::GetTime() noexcept {return GetExtendedTime();}
unsigned long long Audio::GetExtendedTime() noexcept {
    auto read=++TimeReadSerial;
    unsigned high=static_cast<unsigned>(LastTime>>32),previous=static_cast<unsigned>(LastTime);
    for(;;) {
        const unsigned now=SystemTimer::GetMilliseconds();
        if(now<previous)++high;
        LastTime=(static_cast<unsigned long long>(high)<<32)|now;
        if(read==TimeReadSerial)return LastTime;
        read=TimeReadSerial;
    }
}
#endif
