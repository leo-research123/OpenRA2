#pragma once
#include <cstdint>

namespace render {
enum class Status : std::uint32_t { ok, unavailable, invalid_argument, backend_failure };
struct Point {
    std::int32_t X = 0, Y = 0;
    constexpr Point() = default;
    constexpr Point(std::int32_t x, std::int32_t y) : X(x), Y(y) {}
    template<class T> constexpr Point(const T& p) : X(p.X), Y(p.Y) {}
    template<class T> constexpr operator T() const requires requires { T{X, Y}; } { return T{X, Y}; }
    bool operator==(const Point&) const = default;
};
struct Rect {
    std::int32_t X = 0, Y = 0, Width = 0, Height = 0;
    constexpr Rect() = default;
    constexpr Rect(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
        : X(x), Y(y), Width(width), Height(height) {}
    template<class T> constexpr Rect(const T& r) : X(r.X), Y(r.Y), Width(r.Width), Height(r.Height) {}
    template<class T> constexpr operator T() const requires requires { T{X, Y, Width, Height}; } {
        return T{X, Y, Width, Height};
    }
    bool operator==(const Rect&) const = default;
};
enum class VoxelKind : std::uint32_t { raster, shadow, composite };
enum class ScreenEffectStage : std::uint32_t { inactive, old_world, white, new_world, restore };
enum class ScreenEffectOperation : std::uint32_t { apply, capture_begin, capture_end, ion_blast };
struct ScreenEffect {
    std::uint64_t capture_id = 0;
    ScreenEffectStage stage = ScreenEffectStage::inactive;
    ScreenEffectOperation operation = ScreenEffectOperation::apply;
    std::int32_t width = 0, height = 0, brightness = 0;
    std::int32_t warp_step = 0, initial_duration = 1;
    std::int32_t center_x = 0, center_y = 0, depth_base = 0;
};
struct SlotRange {
    std::uint32_t instance = 0, first = 0, count = 0;
};
// Renderer-owned immutable resource/scene graphs retain shared chunks internally.
// These typed snapshots contain no original game or host objects.
struct MapBufferData;
struct MapSceneData;
struct MapSequenceSet;
struct ReferenceFrame;
struct ResourceUpdate {
    std::uint64_t id = 0, version = 0;
    std::uint32_t kind = 0, width = 0, height = 0;
    const void* data = nullptr;
    std::uint32_t size = 0;
    const MapBufferData* snapshot = nullptr;
};
struct SceneDelta {
    const MapSceneData* snapshot = nullptr;
    Point origin{};
    Rect clip{};
    const SlotRange* visible = nullptr;
    const SlotRange* window = nullptr;
    std::uint32_t range_count = 0;
};
enum class PassKind : std::uint32_t { raster, compute, lighting, light_field, scene, effect };
struct PassRange {
    PassKind kind = PassKind::raster;
    std::uint32_t first = 0, count = 1;
    std::int32_t variant = 0, bin_offset = 0;
};
struct LightField {
    bool world = false;
    Rect view{};
    Point camera{}, origin{};
    std::int32_t program = 0;
    std::uint64_t image_version = 0;
};
struct Instance {
    std::int32_t words[16]{};
    std::int32_t& operator[](std::uint32_t i) noexcept { return words[i]; }
    const std::int32_t& operator[](std::uint32_t i) const noexcept { return words[i]; }
    constexpr std::uint32_t size() const noexcept { return 16; }
    std::int32_t* data() noexcept { return words; }
    const std::int32_t* data() const noexcept { return words; }
    bool operator==(const Instance&) const = default;
};
// A frame is an immutable borrowed view. Its owner keeps every array and typed
// snapshot alive through submit's return. Instance/compute strides are 16/20
// signed words respectively; pass ranges preserve original submission order.
struct Frame {
    std::uint64_t serial = 0, generation = 0;
    std::int32_t width = 0, height = 0, camera_x = 0, camera_y = 0;
    const ResourceUpdate* resources = nullptr;
    std::uint32_t resource_count = 0;
    const MapSequenceSet* sequences = nullptr;
    const SceneDelta* scene = nullptr;
    std::uint32_t scene_count = 0;
    const Instance* instances = nullptr;
    std::uint32_t instance_count = 0;
    const std::int32_t* compute_words = nullptr;
    std::uint32_t compute_count = 0;
    const std::int32_t* compute_resources = nullptr;
    std::uint32_t compute_resource_count = 0;
    const std::int32_t* light_bins = nullptr;
    std::uint32_t light_bin_count = 0;
    const std::int32_t* field_words = nullptr;
    std::uint32_t field_word_count = 0;
    const LightField* light_fields = nullptr;
    std::uint32_t light_field_count = 0;
    const PassRange* passes = nullptr;
    std::uint32_t command_count = 0;
    ScreenEffect screen_effect{};
    const ScreenEffect* effects = nullptr;
    std::uint32_t effect_count = 0;
    bool capture = false;
    const ReferenceFrame* reference = nullptr;
};
}
