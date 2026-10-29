#include "presentation/recorder.hpp"
#include "presentation/frame_compiler.hpp"
#include "yrpp/Fundamentals.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace game {
namespace {
enum Kind : unsigned {
    kind_shape,
    kind_tile,
    kind_raster,
    kind_indexed,
    kind_voxel_indexed,
    kind_lighting_shape,
    kind_lighting_shape_batch,
    kind_mask,
    kind_shape_strip,
    kind_screen_effect,
    kind_background_complete,
    kind_background_scene,
    kind_shape_sequence,
    kind_count
};
const char* const kind_names[kind_count] = {"shape",       "tile",        "raster",       "indexed",
                                            "voxel_indexed", "lighting_shape", "lighting_shape_batch", "mask",
                                            "shape_strip", "screen_effect", "background_complete",
                                            "background_scene", "shape_sequence"};

DrawingStatus checked_status(DrawingStatus value) noexcept {
    switch (value) {
        case DrawingStatus::drawn:
        case DrawingStatus::skipped:
        case DrawingStatus::unavailable:
        case DrawingStatus::unsupported:
        case DrawingStatus::invalid_argument:
        case DrawingStatus::backend_failure:
            return value;
    }
    return DrawingStatus::backend_failure;
}

// FNV-1a over the fields of each request. Pointers are replaced by the order in which distinct
// pointers first appear, so the hash of a run does not depend on where the allocator placed anything.
// Targets are not hashed (they name stack objects of nested drawing contexts); pixel storage is hashed
// by content.
class Hash {
public:
    void bytes(const void* data, std::size_t size) noexcept {
        const auto* p = static_cast<const unsigned char*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            value_ ^= p[i];
            value_ *= 1099511628211ull;
        }
    }
    void u32(std::uint32_t v) noexcept { bytes(&v, sizeof v); }
    void i32(std::int32_t v) noexcept { bytes(&v, sizeof v); }
    void u64(std::uint64_t v) noexcept { bytes(&v, sizeof v); }
    void flag(bool v) noexcept { u32(v ? 1u : 0u); }
    void point(const Point2D& p) noexcept {
        i32(p.X);
        i32(p.Y);
    }
    void rect(const RectangleStruct& r) noexcept {
        i32(r.X);
        i32(r.Y);
        i32(r.Width);
        i32(r.Height);
    }
    void cell(const CellStruct& c) noexcept {
        i32(c.X);
        i32(c.Y);
    }
    void pointer(const void* p) {
        if (!p) return u32(0);
        static std::unordered_map<const void*, std::uint32_t> ordinals;
        const auto found = ordinals.try_emplace(p, std::uint32_t(ordinals.size() + 1)).first;
        u32(found->second);
    }
    std::uint64_t value() const noexcept { return value_; }
    void reset() noexcept { value_ = 1469598103934665603ull; }

private:
    std::uint64_t value_ = 1469598103934665603ull;
};

struct Recorder {
    Hash frame;          // the stream of the frame being built
    Hash total;          // the frame hashes in order
    std::uint64_t frames = 0, requests = 0, frame_requests = 0;
    std::uint64_t frame_kinds[kind_count]{}, total_kinds[kind_count]{};
    Hash frame_kind_hash[kind_count];
    bool dump = false;  // RA2_VERIFY_DRAW_STREAM=2: one line per entry
};
Recorder& recorder() {
    static Recorder value;
    return value;
}
void print_total() noexcept {
    auto& r = recorder();
    std::fprintf(stderr, "DRAW_STREAM_TOTAL frames=%llu requests=%llu hash=%016llx\n", (unsigned long long)r.frames,
                 (unsigned long long)r.requests, (unsigned long long)r.total.value());
    for (unsigned i = 0; i < kind_count; ++i)
        std::fprintf(stderr, "DRAW_STREAM_TOTAL kind=%s requests=%llu\n", kind_names[i],
                     (unsigned long long)r.total_kinds[i]);
}
bool active() noexcept {
    static const bool value = [] {
        const char* text = std::getenv("RA2_VERIFY_DRAW_STREAM");
        const bool on = text && text[0] && std::strcmp(text, "0") != 0;
        if (on) {
            recorder().dump = std::strcmp(text, "2") == 0;  // constructed first, so it outlives the exit handler
            std::atexit(print_total);
        }
        return on;
    }();
    return value;
}
// Opens an entry: kind, then the request fields; close() adds the status and counts it.
struct Entry {
    Recorder& r = recorder();
    Hash local;
    Kind kind;
    explicit Entry(Kind k) : kind(k) { local.u32(k); }
    void close(DrawingStatus status) noexcept {
        local.u32(static_cast<std::uint32_t>(status));
        const auto value = local.value();
        r.frame.u64(value);
        r.frame_kind_hash[kind].u64(value);
        if (r.dump)
            std::fprintf(stderr, "DRAW_STREAM_ENTRY serial=%llu index=%llu kind=%s hash=%016llx\n",
                         (unsigned long long)r.frames, (unsigned long long)r.frame_requests, kind_names[kind],
                         (unsigned long long)value);
        ++r.frame_requests;
        ++r.frame_kinds[kind];
    }
    Hash& h() noexcept { return local; }
};
void hash_shape(Hash& h, const ShapeDrawingRequest& q) {
    h.pointer(q.palette);
    h.pointer(q.image);
    h.i32(q.frame);
    h.point(q.position);
    h.rect(q.clip);
    h.u32(q.flags);
    h.i32(q.depth_adjustment);
    h.i32(q.gradient);
    h.i32(q.intensity);
    h.i32(q.tint);
    h.u32(static_cast<std::uint32_t>(q.depth_mode));
    h.u32(static_cast<std::uint32_t>(q.blend_mode));
    h.i32(q.absolute_depth);
    h.pointer(q.depth_image);
    h.i32(q.depth_frame);
    h.point(q.depth_offset);
}
void hash_lighting_shape(Hash& h, const LightingShapeDrawingRequest& q) {
    h.pointer(q.image);
    h.i32(q.frame);
    h.point(q.position);
    h.rect(q.clip);
    h.u32(static_cast<std::uint32_t>(q.operation));
}
}

bool recorder_enabled() noexcept { return active(); }

namespace {

void recorder_note(const ShapeDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_shape);
    hash_shape(e.h(), q);
    e.close(status);
}
void recorder_note(const TileDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_tile);
    auto& h = e.h();
    h.pointer(q.palette);
    h.pointer(q.resource);
    h.pointer(q.image);
    h.i32(q.sub_tile);
    h.point(q.position);
    h.rect(q.clip);
    h.i32(q.level);
    h.i32(q.intensity);
    h.flag(q.use_depth);
    h.flag(q.flat);
    h.flag(q.flag16);
    h.flag(q.flag17);
    h.i32(q.color);
    h.i32(q.buffer_offset_y);
    h.flag(q.translucent);
    e.close(status);
}
void recorder_note(const RasterDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_raster);
    auto& h = e.h();
    h.point(q.position);
    h.rect(q.clip);
    h.i32(q.width);
    h.i32(q.height);
    h.i32(q.pitch);
    h.u32(q.pixel_count);
    h.u32(q.color);
    h.flag(q.original_line);
    h.u32(static_cast<std::uint32_t>(q.blend_mode));
    h.u32(q.spotlight_flags);
    h.i32(q.light_strength);
    h.i32(q.line_z);
    h.u32(q.line_rgb);
    h.i32(q.line_opacity);
    h.point(q.line_neighbor);
    h.i32(q.line_weight);
    h.i32(q.line_neighbor_weight);
    h.u32(q.line_channels);
    h.i32(q.line_light_x);
    h.i32(q.line_sample_x);
    h.i32(q.line_red);
    h.i32(q.line_green);
    h.i32(q.line_blue);
    h.flag(q.content_identity != 0);
    h.u32(q.wave_pixel_count);
    if (q.pixels && q.width > 0 && q.height > 0 && q.pitch >= q.width)
        for (int y = 0; y < q.height; ++y) h.bytes(q.pixels + std::size_t(y) * q.pitch, std::size_t(q.width) * 2);
    if (q.wave_pixels)
        for (std::uint32_t i = 0; i < q.wave_pixel_count; ++i) {
            h.point(q.wave_pixels[i].destination);
            h.point(q.wave_pixels[i].sample);
            h.i32(q.wave_pixels[i].strength);
        }
    e.close(status);
}
void recorder_note(const IndexedDrawingRequest& q, bool voxel, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(voxel ? kind_voxel_indexed : kind_indexed);
    auto& h = e.h();
    h.pointer(q.palette);
    h.point(q.position);
    h.rect(q.clip);
    h.i32(q.width);
    h.i32(q.height);
    h.i32(q.intensity);
    h.i32(q.absolute_depth);
    h.u32(q.pixel_count);
    // Voxel image graphs are created and freed per frame; their addresses are reused unpredictably.
    h.flag(q.voxel != nullptr);
    h.u32(static_cast<std::uint32_t>(q.depth_mode));
    h.flag(q.shadow);
    h.u32(q.flags);
    h.u32(q.tint);
    h.i32(q.gradient);
    if (q.pixels && q.width > 0 && q.height > 0 && std::uint64_t(q.width) * q.height <= q.pixel_count)
        h.bytes(q.pixels, std::size_t(q.width) * q.height * 4);
    e.close(status);
}
void recorder_note(const LightingShapeDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_lighting_shape);
    hash_lighting_shape(e.h(), q);
    e.close(status);
}
void recorder_note(const LightingShapeBatchRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_lighting_shape_batch);
    auto& h = e.h();
    h.rect(q.view);
    h.u32(q.count);
    h.pointer(q.shroud);
    h.point(q.shroud_origin);
    h.point(q.shroud_camera);
    if (q.entries)
        for (std::uint32_t i = 0; i < q.count; ++i) {
            hash_lighting_shape(h, q.entries[i].request);
            h.cell(q.entries[i].cell);
        }
    e.close(status);
}
void recorder_note(const MaskDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_mask);
    auto& h = e.h();
    h.point(q.position);
    h.rect(q.clip);
    h.i32(q.width);
    h.i32(q.height);
    h.i32(q.pitch);
    h.u32(q.byte_count);
    h.u32(q.color);
    if (q.bits) h.bytes(q.bits, q.byte_count);
    e.close(status);
}
void recorder_note(const ShapeStripDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_shape_strip);
    auto& h = e.h();
    hash_shape(h, q.shape);
    h.flag(q.resource_revision != 0);
    h.point(q.step);
    h.i32(q.count);
    e.close(status);
}
void recorder_note(const ScreenEffectDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_screen_effect);
    auto& h = e.h();
    h.u64(q.capture_id);
    h.u32(static_cast<std::uint32_t>(q.stage));
    h.u32(static_cast<std::uint32_t>(q.operation));
    h.i32(q.width);
    h.i32(q.height);
    h.i32(q.brightness);
    h.i32(q.warp_step);
    h.i32(q.initial_duration);
    h.i32(q.center_x);
    h.i32(q.center_y);
    h.i32(q.depth_base);
    e.close(status);
}
void recorder_note(const BackgroundDrawingRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_background_complete);
    e.h().point(q.view_origin);
    e.close(status);
}
void recorder_note(const BackgroundSceneRequest& q, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_background_scene);
    auto& h = e.h();
    // world and object are pointer-derived identities; the layer and cell say which item it is.
    h.u32(static_cast<std::uint32_t>(q.operation));
    h.u32(q.layer);
    h.cell(q.cell);
    h.i32(q.lighting_quality);
    h.point(q.origin);
    h.rect(q.clip);
    h.u32(q.info_capacity);
    h.u32(q.slot_count);
    if (q.slots)
        for (std::uint32_t i = 0; i < q.slot_count; ++i) {
            h.u32(q.slots[i].instance);
            h.u32(q.slots[i].slot);
        }
    h.u32(q.release_count);
    if (q.releases)
        for (std::uint32_t i = 0; i < q.release_count; ++i) {
            h.u32(q.releases[i].instance);
            h.u32(q.releases[i].slot);
        }
    h.u32(q.instance);
    h.u32(q.instance_slots);
    h.u32(q.instance_state);
    h.u32(q.dirty_count);
    if (q.dirty)
        for (std::uint32_t i = 0; i < q.dirty_count; ++i) {
            h.u32(q.dirty[i].instance);
            h.u32(q.dirty[i].first);
            h.u32(q.dirty[i].count);
        }
    h.u32(q.order_count);
    if (q.orders)
        for (std::uint32_t i = 0; i < q.order_count; ++i) {
            h.u32(q.orders[i].instance);
            h.u32(q.orders[i].first);
            h.u32(q.orders[i].size);
            if (q.orders[i].slots)
                for (std::uint32_t k = q.orders[i].first; k < q.orders[i].size; ++k) h.u32(q.orders[i].slots[k - q.orders[i].first]);
        }
    h.u32(q.draw_count);
    for (const auto* ranges : {q.visible, q.window})
        if (ranges)
            for (std::uint32_t i = 0; i < q.draw_count; ++i) {
                h.u32(ranges[i].instance);
                h.u32(ranges[i].first);
                h.u32(ranges[i].count);
            }
    h.u32(q.light_count);
    h.u32(q.stale_capacity);
    if (q.lights)
        for (std::uint32_t i = 0; i < q.light_count; ++i) {
            const auto& l = q.lights[i];
            h.cell(l.cell);
            h.u32(l.normal);
            h.u32(l.terrain);
            for (const auto& t : l.tint) {
                h.i32(t.red);
                h.i32(t.green);
                h.i32(t.blue);
                h.i32(t.shades);
                h.i32(t.factor_red);
                h.i32(t.factor_green);
                h.i32(t.factor_blue);
            }
        }
    h.pointer(q.converter);
    h.u64(q.revision);
    h.u32(q.shroud_count);
    if (q.shroud)
        for (std::uint32_t i = 0; i < q.shroud_count; ++i) {
            h.cell(q.shroud[i].cell);
            h.i32(q.shroud[i].frame);
        }
    h.u32(q.alpha_count);
    if (q.alphas)
        for (std::uint32_t i = 0; i < q.alpha_count; ++i) {
            h.pointer(q.alphas[i].image);
            h.point(q.alphas[i].position);
        }
    // What the callback wrote back.
    if (q.info_count) {
        h.u32(*q.info_count);
        if (q.infos)
            for (std::uint32_t i = 0; i < *q.info_count && i < q.info_capacity; ++i) {
                h.u32(q.infos[i].kind);
                h.u32(q.infos[i].state);
                h.rect(q.infos[i].bounds);
            }
    }
    if (q.changed) h.flag(*q.changed);
    if (q.stale_count) {
        h.u32(*q.stale_count);
        if (q.stale)
            for (std::uint32_t i = 0; i < *q.stale_count && i < q.stale_capacity; ++i) h.cell(q.stale[i]);
    }
    e.close(status);
}
void recorder_note_shape_sequence(const SHPStruct* image, std::uint32_t use, DrawingStatus status) noexcept {
    if (!active()) return;
    Entry e(kind_shape_sequence);
    e.h().pointer(image);
    e.h().u32(use);
    e.close(status);
}
}  // namespace

bool valid_drawing_context(const TypeDrawingContext& c) noexcept {
    return c.backend.struct_size==sizeof(TypeDrawingBackend) &&
        (c.backend.version==31 || (c.backend.version==32 && c.backend_context));
}
bool supports_drawing(const TypeDrawingContext& c, DrawingKind kind) noexcept {
    if (!valid_drawing_context(c)) return false;
    if (c.backend.version==32) {
        if (static_cast<FrameCompiler*>(c.backend_context)->reference()) {
            switch(kind) {
                case DrawingKind::shape: case DrawingKind::tile: case DrawingKind::raster: case DrawingKind::indexed:
                    return true;
                default: return false;
            }
        }
        if (kind!=DrawingKind::voxel_indexed) return true;
        const char* gpu=std::getenv("RA2_GPU_VOXELS");
        return !gpu || std::strcmp(gpu,"0")!=0;
    }
    switch(kind) {
        case DrawingKind::shape: return c.backend.shape!=nullptr;
        case DrawingKind::tile: return c.backend.tile!=nullptr;
        case DrawingKind::raster: return c.backend.raster!=nullptr;
        case DrawingKind::indexed: return c.backend.indexed!=nullptr;
        case DrawingKind::voxel_indexed: return c.backend.voxel_indexed!=nullptr;
        case DrawingKind::lighting_shape: return c.backend.lighting_shape!=nullptr;
        case DrawingKind::lighting_shape_batch: return c.backend.lighting_shape_batch!=nullptr;
        case DrawingKind::mask: return c.backend.mask!=nullptr;
        case DrawingKind::shape_strip: return c.backend.shape_strip!=nullptr;
        case DrawingKind::screen_effect: return c.backend.screen_effect!=nullptr;
        case DrawingKind::background_complete: return c.backend.background_complete!=nullptr;
        case DrawingKind::background_scene: return c.backend.background_scene!=nullptr;
        case DrawingKind::shape_sequence: return c.backend.shape_sequence!=nullptr;
    }
    return false;
}
DrawingStatus record_shape(const TypeDrawingContext& c, const ShapeDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::shape)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_shape(q)
        : c.backend.shape(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_tile(const TypeDrawingContext& c, const TileDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::tile)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_tile(q)
        : c.backend.tile(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_raster(const TypeDrawingContext& c, const RasterDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::raster)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_raster(q)
        : c.backend.raster(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_indexed(const TypeDrawingContext& c, const IndexedDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::indexed)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_indexed(q)
        : c.backend.indexed(c.backend_context,q));
    recorder_note(q, false, status);
    return status;
}
DrawingStatus record_voxel_indexed(const TypeDrawingContext& c, const IndexedDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::voxel_indexed)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_indexed(q)
        : c.backend.voxel_indexed(c.backend_context,q));
    recorder_note(q, true, status);
    return status;
}
DrawingStatus record_lighting_shape(const TypeDrawingContext& c, const LightingShapeDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::lighting_shape)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_lighting_shape(q)
        : c.backend.lighting_shape(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_lighting_shape_batch(const TypeDrawingContext& c, const LightingShapeBatchRequest& q) {
    if (!supports_drawing(c, DrawingKind::lighting_shape_batch)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_lighting_shape_batch(q)
        : c.backend.lighting_shape_batch(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_mask(const TypeDrawingContext& c, const MaskDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::mask)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_mask(q)
        : c.backend.mask(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_shape_strip(const TypeDrawingContext& c, const ShapeStripDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::shape_strip)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_shape_strip(q)
        : c.backend.shape_strip(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_screen_effect(const TypeDrawingContext& c, const ScreenEffectDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::screen_effect)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_screen_effect(q)
        : c.backend.screen_effect(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_background_complete(const TypeDrawingContext& c, const BackgroundDrawingRequest& q) {
    if (!supports_drawing(c, DrawingKind::background_complete)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_background_complete(q)
        : c.backend.background_complete(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_background_scene(const TypeDrawingContext& c, const BackgroundSceneRequest& q) {
    if (!supports_drawing(c, DrawingKind::background_scene)) return DrawingStatus::unavailable;
    const auto status=checked_status(c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_background_scene(q)
        : c.backend.background_scene(c.backend_context,q));
    recorder_note(q, status);
    return status;
}
DrawingStatus record_shape_sequence(const TypeDrawingContext& c, SHPStruct* image, std::uint32_t use) noexcept {
    if (!supports_drawing(c, DrawingKind::shape_sequence)) return DrawingStatus::unavailable;
    const auto status=c.backend.version==32
        ? static_cast<FrameCompiler*>(c.backend_context)->submit_shape_sequence(image,use)
        : c.backend.shape_sequence(c.backend_context,image,use);
    recorder_note_shape_sequence(image,use,status);
    return status;
}
void recorder_frame_end() noexcept {
    if (!active()) return;
    auto& r = recorder();
    const auto hash = r.frame.value();
    std::fprintf(stderr, "DRAW_STREAM frame=%d serial=%llu requests=%llu hash=%016llx", Unsorted::CurrentFrame,
                 (unsigned long long)r.frames, (unsigned long long)r.frame_requests, (unsigned long long)hash);
    for (unsigned i = 0; i < kind_count; ++i)
        if (r.frame_kinds[i])
            std::fprintf(stderr, " %s=%llu:%08llx", kind_names[i], (unsigned long long)r.frame_kinds[i],
                         (unsigned long long)(r.frame_kind_hash[i].value() & 0xFFFFFFFFull));
    std::fprintf(stderr, "\n");
    r.total.u64(hash);
    for (unsigned i = 0; i < kind_count; ++i) {
        r.total_kinds[i] += r.frame_kinds[i];
        r.frame_kinds[i] = 0;
        r.frame_kind_hash[i].reset();
    }
    r.requests += r.frame_requests;
    r.frame_requests = 0;
    ++r.frames;
    r.frame.reset();
}
}
