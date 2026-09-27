#pragma once
#include "api/audio_backend.hpp"

namespace game {
// Exactly one provider per target: standalone host or original-game compat.
bool get_default_audio_backend(AudioBackend*& output) noexcept;
}
