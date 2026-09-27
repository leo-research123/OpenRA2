#include "video_backend.hpp"

namespace game {
bool get_default_video_backend(VideoBackend*&) noexcept { return false; }
}
