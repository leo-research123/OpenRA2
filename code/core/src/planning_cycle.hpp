#pragma once
namespace game {
// Original planning hit-test nodes and graph colors, invalidated by original
// node/token lifecycle methods. No raster output crosses this boundary.
void advance_planning_view() noexcept;
}
