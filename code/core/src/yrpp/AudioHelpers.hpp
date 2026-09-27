#pragma once
#include "yrpp/platform/ABI.h"

namespace game {
// Audio.h module helper. The original playback object's type is still opaque.
// 0x4025B0 takes that pointer in ECX and has no stack arguments.
// Dispatches through the host AudioBackend.
void YRPP_FASTCALL audio_stop_playback_4025b0(void* playback) noexcept;
// Buffer ownership and release semantics belong to the selected backend.
void YRPP_FASTCALL audio_release_buffer_408e70(void* buffer) noexcept;
}
