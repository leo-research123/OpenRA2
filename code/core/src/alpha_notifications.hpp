#pragma once
class AlphaShapeClass;
namespace game {
// The original attachment registry changed; forget a retired presentation memo before pointer reuse.
void alpha_shapes_changed(const AlphaShapeClass* expired = nullptr) noexcept;
}
