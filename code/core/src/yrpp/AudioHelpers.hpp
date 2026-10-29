#pragma once
#include "yrpp/platform/ABI.h"

namespace game {
// Audio.h module helper. The original playback object's type is still opaque.
// 0x4025B0 takes that pointer in ECX and has no stack arguments.
// Native execution submits a stop through the PCM consumer queue.
void YRPP_FASTCALL audio_stop_playback_4025b0(void* playback) noexcept;
// Native PCM storage is retired after the consumer fence.
void YRPP_FASTCALL audio_release_buffer_408e70(void* buffer) noexcept;
}
