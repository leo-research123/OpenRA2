// Original volume object operations at 0x00407100..0x004072B0.
#include "yrpp/VocClass.h"
#include <algorithm>
#include <bit>
#include <cstdlib>

int VolumeStruct::GetVolume() const { return static_cast<DWORD>(unknown_int_8) >> 16; }
void VolumeStruct::SetVolume(int value) {
    Volume |= 1;
    unknown_int_8 = std::bit_cast<std::int32_t>(static_cast<DWORD>(value) << 16);
}
void YRPP_FASTCALL VolumeStruct::Initialize(unsigned value) noexcept {
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
    reinterpret_cast<void(YRPP_FASTCALL*)(VolumeStruct*, unsigned)>(0x407100)(this, value);
#else
    Volume = 0;
    const auto now = Audio::GetTime();
    nknown_18 = unsigned(now);
    unknown_int_1C = int(now >> 32);
    unknown_int_C = 1073741;
    unknown_int_10 = 1000;
    unknown_int_14 = 0;
    SetVolume(int(value));
    Update();
#endif
}
void YRPP_FASTCALL VolumeStruct::SetTarget(unsigned value) noexcept {
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
    reinterpret_cast<void(YRPP_FASTCALL*)(VolumeStruct*, unsigned)>(0x407170)(this, value);
#else
    Volume &= ~1;
    if (unsigned(unknown_int_8) == unknown_4) {
        const auto now = Audio::GetTime();
        nknown_18 = unsigned(now);
        unknown_int_1C = int(now >> 32);
    }
    unknown_int_8 = std::bit_cast<std::int32_t>(value << 16);
#endif
}
bool VolumeStruct::Update() noexcept {
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
    return reinterpret_cast<int(YRPP_THISCALL*)(VolumeStruct*)>(0x4071C0)(this) != 0;
#else
    if (unknown_4 == unsigned(unknown_int_8)) return false;
    return UpdateAt((Volume & 1) ? 0 : Audio::GetTime());
#endif
}
void VolumeStruct::InitializeAt(unsigned value, unsigned long long now) noexcept {
    *this = {};
    Volume = value ? 3 : 1;
    unknown_4 = value << 16;
    unknown_int_8 = std::bit_cast<std::int32_t>(value << 16);
    unknown_int_C = 1073741;
    unknown_int_10 = 1000;
    nknown_18 = unsigned(now);
    unknown_int_1C = int(now >> 32);
}
void VolumeStruct::SetTargetAt(unsigned value, unsigned long long now) noexcept {
    Volume &= ~1;
    if (unsigned(unknown_int_8) == unknown_4) {
        nknown_18 = unsigned(now);
        unknown_int_1C = int(now >> 32);
    }
    unknown_int_8 = std::bit_cast<std::int32_t>(value << 16);
}
bool VolumeStruct::UpdateAt(unsigned long long now) noexcept {
    // The original gain domain bounds the signed difference to +/-0x40000000.
    const int difference = unknown_int_8 - int(unknown_4);
    if (!difference) return false;
    if (Volume & 1)
        unknown_4 = unsigned(unknown_int_8);
    else {
        const auto previous = (static_cast<unsigned long long>(unsigned(unknown_int_1C)) << 32) | nknown_18;
        const auto maximum =
            (static_cast<unsigned long long>(unsigned(unknown_int_14)) << 32) | unsigned(unknown_int_10);
        const auto elapsed = std::min(now - previous, maximum);
        nknown_18 = unsigned(now);
        unknown_int_1C = int(now >> 32);
        const unsigned step = unsigned(elapsed) * unsigned(unknown_int_C);
        const auto change = std::min(step, unsigned(std::abs(difference)));
        unknown_4 = difference < 0 ? unknown_4 - change : unknown_4 + change;
    }
    Volume |= 2;
    return true;
}
