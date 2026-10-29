#pragma once

enum class AbstractType : unsigned int;

namespace game {
// RA2_VERIFY_DRAW_COVERAGE: counts what a run actually draws per category, so a golden scene can be
// shown to exercise the category a refactoring step touches. Off by default; every entry point is a
// single flag test when off. Totals are printed to stderr at exit as DRAW_COVERAGE lines.
//
// instances: objects handed to drawing (cells/items for overlay, smudge and tile), or for the
//   effect classes the live objects present when a Render starts.
// requests: drawing submissions made while the category scope was current.
// frames: rendered frames in which the category had an instance; max: its largest per-frame count.
enum class DrawCategory : unsigned {
    infantry,
    unit,
    aircraft,
    building,
    anim,
    terrain,
    bullet,
    particle,
    voxel_anim,
    overlay,
    smudge,
    tile,
    ebolt,
    laser,
    radbeam,
    wave,
    spotlight,
    line_trail,
    ion_blast,
    tactical,  // requests of the tactical Render that no narrower category claims
    ui_radar,
    ui_sidebar,
    ui_power,
    ui_messages,
    ui_tooltips,
    ui_other,  // requests made outside the tactical Render and outside the scopes above
    count
};

bool draw_coverage_enabled() noexcept;
// A submission through the drawing ABI.
void draw_coverage_request() noexcept;
// One object of a class handed to drawing; objects of other classes are ignored.
void draw_coverage_object(AbstractType kind) noexcept;
void draw_coverage_instance(DrawCategory category) noexcept;
void draw_coverage_render_begin() noexcept;
void draw_coverage_render_end() noexcept;
// After a frame's drawing finished: closes the per-frame counters.
void draw_coverage_frame_end() noexcept;

// Requests made while the scope lives belong to the category.
class DrawCategoryScope {
public:
    explicit DrawCategoryScope(DrawCategory category) noexcept;
    ~DrawCategoryScope();
    DrawCategoryScope(const DrawCategoryScope&) = delete;
    DrawCategoryScope& operator=(const DrawCategoryScope&) = delete;

private:
    DrawCategory previous_;
    bool active_;
};
}
