#pragma once
#include "verify/draw_coverage.hpp"
#include "verify/draw_purity.hpp"

namespace game {
// Brackets one tactical Render with the drawing verification modes (RA2_VERIFY_DRAW_PURE,
// RA2_VERIFY_DRAW_COVERAGE). Both are off by default; a disabled scope costs two flag tests.
class DrawVerifyScope {
public:
    DrawVerifyScope() noexcept { draw_coverage_render_begin(); }
    ~DrawVerifyScope() { draw_coverage_render_end(); }
    DrawVerifyScope(const DrawVerifyScope&) = delete;
    DrawVerifyScope& operator=(const DrawVerifyScope&) = delete;

private:
    DrawPurityScope purity_;
};
}
