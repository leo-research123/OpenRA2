#include "audio_backend.hpp"

namespace game {
bool get_default_audio_backend(AudioBackend*&) noexcept { return false; }
}
