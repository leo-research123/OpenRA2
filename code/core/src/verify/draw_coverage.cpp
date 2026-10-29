#include "verify/draw_coverage.hpp"
#include "yrpp/EBolt.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/IonBlastClass.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/RadBeam.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/WaveClass.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace game {
namespace {
constexpr unsigned category_count = unsigned(DrawCategory::count);
const char* const names[category_count] = {
    "infantry",   "unit",      "aircraft",    "building",   "anim",      "terrain",     "bullet",
    "particle",   "voxel_anim", "overlay",    "smudge",     "tile",      "ebolt",       "laser",
    "radbeam",    "wave",      "spotlight",   "line_trail", "ion_blast", "tactical",    "ui_radar",
    "ui_sidebar", "ui_power",  "ui_messages", "ui_tooltips", "ui_other"};

struct Counters {
    std::uint64_t instances = 0, requests = 0, frames = 0, max_frame = 0;
};
struct State {
    Counters total[category_count];
    std::uint64_t frame_instances[category_count]{};
    DrawCategory current = DrawCategory::ui_other;
    unsigned render_depth = 0, frames = 0;
};
State state;

void print_totals() noexcept {
    std::fprintf(stderr, "DRAW_COVERAGE frames=%u\n", state.frames);
    for (unsigned i = 0; i < category_count; ++i) {
        const auto& c = state.total[i];
        std::fprintf(stderr, "DRAW_COVERAGE category=%s instances=%llu requests=%llu frames=%llu max_per_frame=%llu\n",
                     names[i], (unsigned long long)c.instances, (unsigned long long)c.requests,
                     (unsigned long long)c.frames, (unsigned long long)c.max_frame);
    }
}
void instances(DrawCategory category, std::uint64_t count) noexcept {
    if (!count) return;
    state.total[unsigned(category)].instances += count;
    state.frame_instances[unsigned(category)] += count;
}
bool category_of(AbstractType kind, DrawCategory& category) noexcept {
    switch (kind) {
        case AbstractType::Infantry:
            category = DrawCategory::infantry;
            return true;
        case AbstractType::Unit:
            category = DrawCategory::unit;
            return true;
        case AbstractType::Aircraft:
            category = DrawCategory::aircraft;
            return true;
        case AbstractType::Building:
            category = DrawCategory::building;
            return true;
        case AbstractType::Anim:
            category = DrawCategory::anim;
            return true;
        case AbstractType::Terrain:
            category = DrawCategory::terrain;
            return true;
        case AbstractType::Bullet:
            category = DrawCategory::bullet;
            return true;
        case AbstractType::Particle:
            category = DrawCategory::particle;
            return true;
        case AbstractType::VoxelAnim:
            category = DrawCategory::voxel_anim;
            return true;
        default:
            return false;
    }
}
}

bool draw_coverage_enabled() noexcept {
    static const bool value = [] {
        const char* text = std::getenv("RA2_VERIFY_DRAW_COVERAGE");
        const bool on = text && text[0] && std::strcmp(text, "0") != 0;
        if (on) std::atexit(print_totals);
        return on;
    }();
    return value;
}
void draw_coverage_request() noexcept {
    if (!draw_coverage_enabled()) return;
    ++state.total[unsigned(state.current)].requests;
}
void draw_coverage_object(AbstractType kind) noexcept {
    if (!draw_coverage_enabled()) return;
    DrawCategory category;
    if (category_of(kind, category)) instances(category, 1);
}
void draw_coverage_instance(DrawCategory category) noexcept {
    if (!draw_coverage_enabled()) return;
    instances(category, 1);
}
void draw_coverage_render_begin() noexcept {
    if (!draw_coverage_enabled()) return;
    if (state.render_depth++) return;
    state.current = DrawCategory::tactical;
    // Effects are drawn from their registries; what is registered when the pass starts is what it draws.
    instances(DrawCategory::ebolt, std::uint64_t(EBolt::Array.Count));
    instances(DrawCategory::laser, std::uint64_t(LaserDrawClass::Array.Count));
    instances(DrawCategory::radbeam, std::uint64_t(RadBeam::Array.Count));
    instances(DrawCategory::wave, std::uint64_t(WaveClass::Array.Count));
    instances(DrawCategory::spotlight, std::uint64_t(SpotlightClass::Array.Count));
    instances(DrawCategory::line_trail, std::uint64_t(LineTrail::Array.Count));
    instances(DrawCategory::ion_blast, std::uint64_t(IonBlastClass::Array.Count));
}
void draw_coverage_render_end() noexcept {
    if (!draw_coverage_enabled() || !state.render_depth) return;
    if (--state.render_depth == 0) state.current = DrawCategory::ui_other;
}
void draw_coverage_frame_end() noexcept {
    if (!draw_coverage_enabled()) return;
    ++state.frames;
    for (unsigned i = 0; i < category_count; ++i) {
        auto& total = state.total[i];
        if (state.frame_instances[i]) {
            ++total.frames;
            if (state.frame_instances[i] > total.max_frame) total.max_frame = state.frame_instances[i];
        }
        state.frame_instances[i] = 0;
    }
}

DrawCategoryScope::DrawCategoryScope(DrawCategory category) noexcept
    : previous_(state.current), active_(draw_coverage_enabled()) {
    if (active_) state.current = category;
}
DrawCategoryScope::~DrawCategoryScope() {
    if (active_) state.current = previous_;
}
}
