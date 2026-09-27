#include "audio_backend.hpp"

namespace game {
namespace {
AudioBackend* installed = nullptr;
}
void set_audio_backend(AudioBackend& backend) noexcept {
    installed = &backend;
}
void reset_audio_backend() noexcept {
    installed = nullptr;
}
bool get_audio_backend(AudioBackend*& output) noexcept {
    if (!installed) return get_default_audio_backend(output);
    output = installed;
    return true;
}
}
