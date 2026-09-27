#include "video_backend.hpp"

namespace game {
namespace {
VideoBackend* installed = nullptr;
}
void set_video_backend(VideoBackend& backend) noexcept { installed = &backend; }
void reset_video_backend() noexcept { installed = nullptr; }
bool get_video_backend(VideoBackend*& output) noexcept {
    if (!installed) return get_default_video_backend(output);
    output = installed;
    return true;
}
}
