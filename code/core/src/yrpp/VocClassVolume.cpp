// Existing VolumeStruct operations, calibrated to YR 407150 / 4072B0.
#include "yrpp/VocClass.h"
#include <bit>
#include "api/audio_backend.hpp"
#if !defined(RA2_YRPP_GAME)
void YRPP_FASTCALL VocClass::PlayGlobal(int index, int panning, float volume, AudioController* controller) {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend)) (void)backend->play_global(index,panning,volume,controller);
}
#endif
int VolumeStruct::GetVolume() const { return static_cast<DWORD>(unknown_int_8) >> 16; }
void VolumeStruct::SetVolume(int value) {
    Volume |= 1;
    unknown_int_8 = std::bit_cast<std::int32_t>(static_cast<DWORD>(value) << 16);
}
