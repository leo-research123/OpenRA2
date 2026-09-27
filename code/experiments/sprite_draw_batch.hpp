#pragma once
#include <span>

namespace ra2::experiments {

// Derived display data, not a game object or an original-game ABI structure.
// Rect is the final cropped screen rectangle, including the frame's anchor offset.
// UV selects a frame in the batch's shared atlas. Input order is drawing order.
struct SpriteDrawInstance {
    float x, y, width, height;
    float u, v, uv_width, uv_height;
    float red, green, blue, alpha;
};

struct SpriteDrawBatch {
    // Borrowed only for the duration of submission; the renderer copies the values.
    std::span<const SpriteDrawInstance> instances;
};

}
