#pragma once
#include "api/video_backend.hpp"

namespace game {
// Exactly one provider per target: standalone host or original-game compat.
bool get_default_video_backend(VideoBackend*& output) noexcept;
}
