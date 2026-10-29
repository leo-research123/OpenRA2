#include "api/drawing_diagnostics.hpp"
#include "presentation/frame_compiler.hpp"
#include "lighting_batch.hpp"
#include "api/drawing_resources.hpp"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/FileFormats/SHP.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <limits>
#include <chrono>
namespace game {
namespace {
std::uint32_t rgba565(WORD word) {
    const unsigned r = (word >> 11) & 31, g = (word >> 5) & 63, b = word & 31;
    // Exact normalized RGB565 -> RGB8, rounded to nearest. Bit replication
    // differs by one for some channels (for example red 3 becomes 24, not 25).
    return ((r * 255u + 15u) / 31u) | (((g * 255u + 31u) / 63u) << 8) | (((b * 255u + 15u) / 31u) << 16) | 0xff000000u;
}
// The packet's image lies inside its clip: the clip removed nothing, so the
// request draws the same pixels relative to its position at any view.
bool contained(const TypeGpuPacket& packet) {
    const auto& a = packet.parameters;
    return a[8] > 0 && a[9] > 0 && a[4] >= a[6] && a[5] >= a[7] &&
           std::int64_t(a[4]) + a[2] <= std::int64_t(a[6]) + a[8] &&
           std::int64_t(a[5]) + a[3] <= std::int64_t(a[7]) + a[9];
}
// Sampling never retires resources. Cache residency ends at real pressure or map teardown.
constexpr std::uint64_t ResourceSampleFrames = 256;
}
std::shared_ptr<MapBufferData> FrameCompiler::buffer(const std::vector<std::uint32_t>& words) {
    return buffer(words.data(), words.size());
}
std::shared_ptr<MapBufferData> FrameCompiler::buffer(const std::uint32_t* words, std::size_t count) {
    auto data = std::make_shared<MapBufferData>();
    data->id = next_id_++;
    if (count > std::numeric_limits<std::uint32_t>::max() / 4) throw std::length_error("drawing buffer size");
    data->byte_count = count * 4;
    data->bytes = std::make_unique_for_overwrite<std::uint8_t[]>(data->byte_count);
    if (count) std::memcpy(data->bytes.get(), words, count * 4);
    return data;
}
void FrameCompiler::allocate_resource(const std::shared_ptr<MapBufferData>& data, int width, int height) {
    if (!data) throw std::invalid_argument("direct drawing resource");
    if (!data->allocation) {
        if (data->byte_count % 4) throw std::invalid_argument("direct drawing resource words");
        try {
            data->allocation =
                (data->voxel_model ? voxel_model_pool_ : resource_pool_).allocate(std::uint32_t(data->word_count()));
        } catch (const std::length_error&) {
            log_resource_pool("allocation-failed");
            for (;;) {
                try {
                    data->allocation = (data->voxel_model ? voxel_model_pool_ : resource_pool_)
                        .allocate(std::uint32_t(data->word_count()));
                    break;
                } catch (const std::length_error&) {
                    if (!reclaim_resource(data->voxel_model ? ResourcePressure::voxel_models : ResourcePressure::pixels)) throw;
                }
            }
        }
        data->image_width = width;
        data->image_height = height;
    } else if (width && (data->image_width != width || data->image_height != height))
        throw std::invalid_argument("drawing resource dimensions changed");
}
void FrameCompiler::retain_resource(const std::shared_ptr<MapBufferData>& data, int width, int height) {
    if (data && data->submit_frame == item_serial_) {
        if (width && (data->image_width != width || data->image_height != height))
            throw std::invalid_argument("drawing resource dimensions changed");
        return;
    }
    allocate_resource(data, width, height);
    if (data->submit_frame != item_serial_) {
        frame_->resources.push_back(data);
        data->submit_frame = item_serial_;
    }
}
bool FrameCompiler::append_direct(const PixelParameters& words, const std::shared_ptr<MapBufferData>& source,
                                 const std::shared_ptr<MapBufferData>& palette, bool constant) {
    retain_resource(source, constant ? 0 : words[2], constant ? 0 : words[3]);
    retain_resource(palette);
    auto& frame = *frame_;
    int variant = -1;
    const bool raster = raster_packet(words, frame.width, frame.height, variant);
    const auto first = std::uint32_t(raster ? frame.instances.size() : frame.compute.size());
    if (raster)
        frame.instances.push_back(
            raster_instance(words, int(source->allocation->slot), int(palette->allocation->first)));
    else {
        variant = -1;
        frame.compute.push_back(words);
        frame.compute_resources.push_back(int(source->allocation->first));
        frame.compute_resources.push_back(int(palette->allocation->first));
    }
    append_run(first, variant);
    return raster;
}
void FrameCompiler::append_lighting(LightingParameters words, const std::shared_ptr<MapBufferData>& source) {
    retain_resource(source);
    words[17] = int(source->allocation->first);
    const auto first = std::uint32_t(frame_->compute.size());
    frame_->compute.push_back(words);
    frame_->compute_resources.insert(frame_->compute_resources.end(), {0, 0});
    append_run(first, -2);
}

void FrameCompiler::append_instance(RasterInstance instance, int variant, const std::shared_ptr<MapBufferData>& source,
                                   const std::shared_ptr<MapBufferData>& palette) {
    retain_resource(source, instance[2] & 0xFFFF, int(std::uint32_t(instance[2]) >> 16));
    retain_resource(palette);
    instance[0] = int(source->allocation->slot);
    instance[1] = int(palette->allocation->first);
    const auto first = std::uint32_t(frame_->instances.size());
    frame_->instances.push_back(instance);
    append_run(first, variant);
}
void FrameCompiler::append_run(std::uint32_t first, int variant) {
    auto& frame = *frame_;
    std::size_t laser_a = 0, laser_b = 0;
    if (variant == -1) {
        const auto& p = frame.compute[first];
        if ((p[19] == 9 || p[19] == 10) && p[2] == 1 && p[3] == 1 && p[0] == frame.width && p[1] == frame.height) {
            const auto x = std::int64_t(p[4]), y = std::int64_t(p[5]);
            const auto nx = x + (p[19] == 10 ? p[17] : 0), ny = y + (p[19] == 10 ? p[18] : 0);
            if (x >= 0 && y >= 0 && x < frame.width && y < frame.height && nx >= 0 && ny >= 0 && nx < frame.width &&
                ny < frame.height) {
                variant = -3;
                laser_a = std::size_t(y) * frame.width + x;
                laser_b = std::size_t(ny) * frame.width + nx;
                laser_pixel_epochs_.resize(std::size_t(frame.width) * frame.height, 0);
            }
        }
    }
    const auto kind = variant >= 0    ? MapCommandKind::raster
                      : variant == -2 ? MapCommandKind::lighting
                                      : MapCommandKind::compute;
    if (!frame.commands.empty()) {
        auto& last = frame.commands.back();
        if (last.kind == kind && last.variant == variant && last.first + last.count == first) {
            const bool compatible =
                variant >= 0 || variant == -2 ||
                (variant == -3 ? last.count < 65536 && laser_pixel_epochs_[laser_a] != laser_epoch_ &&
                                     laser_pixel_epochs_[laser_b] != laser_epoch_
                               : (render::pixel_local_packet(frame.compute[first], frame.width, frame.height) &&
                                  render::pixel_local_packet(frame.compute[last.first], frame.width, frame.height) &&
                                  frame.compute[first][16] == frame.compute[last.first][16]));
            if (compatible) {
                ++last.count;
                if (variant == -3) laser_pixel_epochs_[laser_a] = laser_pixel_epochs_[laser_b] = laser_epoch_;
                return;
            }
        }
    }
    if (variant == -3) {
        if (++laser_epoch_ == 0) {
            std::fill(laser_pixel_epochs_.begin(), laser_pixel_epochs_.end(), 0);
            laser_epoch_ = 1;
        }
        laser_pixel_epochs_[laser_a] = laser_pixel_epochs_[laser_b] = laser_epoch_;
    }
    frame.commands.push_back({kind, first, 1, variant});
}
void FrameCompiler::clear() {
    log_resource_pool("map-clear");
    print_shape_stats(true);
    shape_stats_.first_serial = 0;
    shape_stats_.total_decodes = shape_stats_.total_late = shape_stats_.total_descriptions = 0;
    shape_stats_.total_sequences = shape_stats_.total_sequence_frames = shape_stats_.total_sequence_bytes =
        shape_stats_.total_prewarmed = shape_stats_.total_prewarm_bytes = 0;
    sequences_.clear();
    sequence_index_.clear();
    sequence_cache_[0] = sequence_cache_[1] = nullptr;
    sequence_set_.reset();
    retired_sequences_.clear();
    sequence_dirty_ = false;
    frame_pool_.clear();
    frame_ = nullptr;
    scene_pending_.clear();
    masks_.clear();
    strips_.clear();
    mask_bytes_ = strip_bytes_ = 0;
    scene_.reset();
    clear_scene_slots();
    scene_recording_ = false;
    scene_world_ = 0;
    clear_scene_lighting();
    base_palettes_.clear();
    voxel_images_.clear();
    voxel_sources_.clear();
    voxel_lru_.clear();
    voxel_cache_words_ = 0;
    raster_contents_.clear();
    tiles_.clear();
    palettes_.clear();
    converters_.clear();
    fills_.clear();
    lighting_assets_.clear();
    lighting_frame_count_ = 0;
    indexed_.clear();
}
std::shared_ptr<MapFrameData> FrameCompiler::acquire_frame() {
    for (auto& entry : frame_pool_)
        if (entry.use_count() == 1) {
            auto& frame = *entry;
            frame.generation = 0;
            frame.width = frame.height = 0;
            frame.screen_effect = {};
            frame.view_origin = {};
            frame.screen_effect_marks.clear();
            frame.light_fields.clear();
            frame.scenes.clear();
            frame.instances.clear();
            frame.compute.clear();
            frame.compute_resources.clear();
            frame.light_bins.clear();
            frame.commands.clear();
            frame.resources.clear();
            frame.sequence_resources.clear();
            frame.field_words.clear();
            publish_sequences();
            frame.sequences = sequence_set_;
            return entry;
        }
    // Keep captures alive rather than dropping their one-shot commands.
    // Backpressure beyond the bounded queue is a reported backend failure.
    if (frame_pool_.size() == 8) throw std::length_error("drawing frame queue exhausted");
    auto frame = std::make_shared<MapFrameData>();
    frame_pool_.push_back(frame);
    publish_sequences();
    frame->sequences = sequence_set_;
    return frame;
}
bool FrameCompiler::end_frame() noexcept {
    bool prepared = true;
    try {
        if (frame_) {
            render_compiler_.compile(*frame_);
        }
    } catch (...) {
        std::snprintf(error_, sizeof(error_), "Ordered pixel run preparation failed");
        record_drawing_diagnostic(DrawingStatus::backend_failure, "bridge_end_frame",
                                  "ordered_run_preparation_exception", error_, __FILE__, __LINE__);
        prepared = false;
    }

    // Indexed copies live while consecutive drawn frames use them, so the
    // renderer keeps the same identity resident in its resource arena too.
    if (frame_) {
        for (auto it = indexed_.begin(); it != indexed_.end();)
            it = it->second.used != frame_serial_ ? indexed_.erase(it) : std::next(it);
        ++frame_serial_;
    }
    if (item_serial_ % ResourceSampleFrames == 0) log_resource_pool("sample");
    if (frame_) frame_->retired_sequences.swap(retired_sequences_);
    if (frame_ && shape_stats_.enabled) {
        auto& stats = shape_stats_;
        ++stats.frames;
        stats.max_calls = std::max(stats.max_calls, stats.frame_calls);
        stats.max_shapes = std::max(stats.max_shapes, stats.frame_shapes);
        stats.max_modes = std::max(stats.max_modes, stats.frame_modes);
        if (stats.frames >= 150) print_shape_stats(false);
    }
    frame_ = nullptr;
    return prepared;
}
void FrameCompiler::print_shape_stats(bool final) noexcept {
    auto& stats = shape_stats_;
    if (!stats.enabled || !stats.frames) return;
    const double frames = double(stats.frames);
    std::fprintf(
        stderr,
        "SHAPE_STATS %s serial=%llu frames=%u calls=%.1f max_calls=%u shapes=%.1f max_shapes=%u modes=%.1f "
        "max_modes=%u "
        "raster=%.1f compute=%.1f scene=%.1f sha=%.1f descriptions=%llu decodes=%llu late_decodes=%llu "
        "sequences=%llu sequence_frames=%llu sequence_bytes=%llu prewarmed=%llu prewarm_bytes=%llu total_decodes=%llu "
        "total_late=%llu "
        "total_descriptions=%llu total_sequences=%llu total_sequence_frames=%llu total_sequence_bytes=%llu "
        "total_prewarmed=%llu total_prewarm_bytes=%llu assets=%zu\n",
        final ? "final" : "window", (unsigned long long)(item_serial_ - stats.first_serial), stats.frames,
        double(stats.calls) / frames, stats.max_calls, double(stats.shapes) / frames, stats.max_shapes,
        double(stats.modes) / frames, stats.max_modes, double(stats.raster) / frames, double(stats.compute) / frames,
        double(stats.scene) / frames, double(stats.sha) / frames, (unsigned long long)stats.descriptions,
        (unsigned long long)stats.decodes, (unsigned long long)stats.late_decodes, (unsigned long long)stats.sequences,
        (unsigned long long)stats.sequence_frames, (unsigned long long)stats.sequence_bytes,
        (unsigned long long)stats.prewarmed, (unsigned long long)stats.prewarm_bytes,
        (unsigned long long)stats.total_decodes, (unsigned long long)stats.total_late,
        (unsigned long long)stats.total_descriptions, (unsigned long long)stats.total_sequences,
        (unsigned long long)stats.total_sequence_frames, (unsigned long long)stats.total_sequence_bytes,
        (unsigned long long)stats.total_prewarmed, (unsigned long long)stats.total_prewarm_bytes, sequences_.size());
    std::fflush(stderr);
    stats.frames = stats.max_calls = stats.max_shapes = stats.max_modes = 0;
    stats.calls = stats.shapes = stats.modes = stats.raster = stats.compute = stats.scene = stats.sha =
        stats.descriptions = stats.decodes = stats.late_decodes = stats.skipped = stats.sequences =
            stats.sequence_frames = stats.sequence_bytes = stats.prewarmed = stats.prewarm_bytes = 0;
}
MapDrawingContext FrameCompiler::begin_frame(MapFrameData& frame) {
    frame_ = &frame;
    frame.reference = reference_ ? std::make_shared<render::ReferenceFrame>() : nullptr;
    error_[0] = 0;
    ++item_serial_;
    if (item_serial_ == 2) log_resource_pool("first-frame");
    frame.serial = item_serial_;
    auto& stats = shape_stats_;
    if (!stats.checked) {
        stats.checked = true;
        const auto* text = std::getenv("RA2_SHAPE_STATS");
        stats.enabled = text && std::strcmp(text, "0") != 0;
        if (const auto* after = std::getenv("RA2_SHAPE_STATS_AFTER"))
            stats.late_after = std::strtoull(after, nullptr, 10);
    }
    if (stats.enabled) {
        if (!stats.first_serial) stats.first_serial = item_serial_;
        stats.frame_calls = stats.frame_shapes = stats.frame_modes = 0;
    }
    MapDrawingContext context;
    context.types.backend_context = this;
    context.types.target = reinterpret_cast<DrawingTargetHandle*>(this);
    // Version 32 binds the concrete presentation compiler, without drawing callbacks.
    // Version 31 remains the source-compatible diagnostic/software adapter.
    context.types.backend.version = 32;
    context.terrain_palette = palette;
    context.shape_palette = shape_palette;
    context.color_scheme_palette = color_scheme_palette;
    context.converter_palette = converter_palette;
    context.plain_palette = [](void* p, const BytePalette& source, const DrawingPaletteHandle*& out) noexcept {
        return shape_palette(p, source, 1, out);
    };
    return context;
}
DrawingStatus FrameCompiler::palette(void* pointer, const BytePalette& source, int r, int g, int b, int shades,
                                    const DrawingPaletteHandle*& output) noexcept {
    return prepare_palette(pointer, source, r, g, b, shades, PaletteKind::terrain, output);
}
DrawingStatus FrameCompiler::shape_palette(void* pointer, const BytePalette& source, int shades,
                                          const DrawingPaletteHandle*& output) noexcept {
    return prepare_palette(pointer, source, 1000, 1000, 1000, shades, PaletteKind::shape, output);
}
DrawingStatus FrameCompiler::color_scheme_palette(void* pointer, const BytePalette& source, int shades,
                                                 const DrawingPaletteHandle*& output) noexcept {
    return prepare_palette(pointer, source, 1000, 1000, 1000, shades, PaletteKind::color_scheme, output);
}
DrawingStatus FrameCompiler::converter_palette(void* pointer, const ConvertClass& source, std::uint64_t revision,
                                              const DrawingPaletteHandle*& output) noexcept {
    output = nullptr;
    if (source.BytesPerPixel != 2) return DrawingStatus::unsupported;
    if (source.ShadeCount < 1 || source.ShadeCount > 256 || !source.PaletteData || !source.FullColorData)
        return DrawingStatus::invalid_argument;
    try {
        auto& self = *static_cast<FrameCompiler*>(pointer);
        auto& cached = self.converters_[&source];
        // The revision names the table contents, so an unchanged revision
        // needs no copy or comparison.
        if (!cached.palette.buffer || cached.revision != revision) {
            const auto shade_words = std::size_t(source.ShadeCount) * 256;
            std::vector<std::uint32_t> rgba(shade_words + 256);
            const auto* palette = static_cast<const WORD*>(source.PaletteData);
            const auto* full = static_cast<const WORD*>(source.FullColorData);
            std::transform(palette, palette + 256, rgba.begin(), rgba565);
            std::transform(full, full + shade_words, rgba.begin() + 256, rgba565);
            // Packets already queued retain their immutable buffer even if
            // UpdateColors changes this same converter later in the frame.
            cached.palette = {self.buffer(rgba), source.ShadeCount};
            cached.palette.converter = true;
            cached.palette.identity = &source;
            cached.revision = revision;
        }
        output = reinterpret_cast<const DrawingPaletteHandle*>(&cached.palette);
        return DrawingStatus::drawn;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus FrameCompiler::prepare_palette(void* pointer, const BytePalette& source, int r, int g, int b, int shades,
                                            PaletteKind kind, const DrawingPaletteHandle*& output) noexcept {
    auto& self = *static_cast<FrameCompiler*>(pointer);
    try {
        const PaletteKey key{&source, r, g, b, shades, int(kind)};
        auto found = self.palettes_.find(key);
        if (found == self.palettes_.end()) {
            std::vector<WORD> words(std::size_t(shades + 1) * 256);
            if (!ConvertClass::BuildColorTable(source, words.data(), 256, 1, 2) ||
                !(kind == PaletteKind::shape
                      ? ConvertClass::BuildColorTable(source, words.data() + 256, words.size() - 256, shades, 2)
                  : kind == PaletteKind::color_scheme
                      ? ColorScheme::BuildColorTable(source, words.data() + 256, words.size() - 256, shades, 2, false)
                      : LightConvertClass::BuildColorTable(source, words.data() + 256, words.size() - 256, shades, r, g,
                                                           b, nullptr, 0, 2, false)))
                return DrawingStatus::backend_failure;
            std::vector<std::uint32_t> rgba(words.size());
            for (std::size_t i = 0; i < words.size(); ++i)
                rgba[i] = rgba565(words[i]);
            MapPaletteData data{self.buffer(rgba), shades};
            if (kind == PaletteKind::terrain) {
                data.tint_source = &source;
                data.red = r;
                data.green = g;
                data.blue = b;
            }
            found = self.palettes_.emplace(key, std::move(data)).first;
        }
        output = reinterpret_cast<const DrawingPaletteHandle*>(&found->second);
        return DrawingStatus::drawn;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus FrameCompiler::tile(void* pointer, const TileDrawingRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "tile", condition, request, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);
    if (!self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self) || !request.palette)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self) "
                        "|| !request.palette",
                        __LINE__);
    const auto& palette = *reinterpret_cast<const MapPaletteData*>(request.palette);
    TypeGpuTarget target{self.frame_->width, self.frame_->height, palette.shades, 0, 0x8000, true};
    TypeGpuPacket base, extra;
    TypeTileMeta meta;
    auto result = describe_type_tile(request, meta);
    if (result == DrawingStatus::drawn) result = prepare_type_tile_parameters(request, meta, target, base, extra);
    if (result != DrawingStatus::drawn) {
        // A 60x29 TMP skipped here was clipped away; this view decided it.

        if (self.scene_recording_ && result == DrawingStatus::skipped) {
            std::snprintf(self.error_, sizeof(self.error_), "TMP outside the background recording view");
            return DrawingStatus::unsupported;
        }
        return diagnose(result, "backend_result", __LINE__);
    }
    if (self.scene_recording_ && (!contained(base) || ((meta.flags & 1) && meta.extra_width > 0 &&
                                                       meta.extra_height > 0 && !contained(extra)))) {
        std::snprintf(self.error_, sizeof(self.error_), "TMP clipped by the background recording view");
        return DrawingStatus::unsupported;
    }
    const auto key = std::make_pair(request.image, request.use_depth);
    auto found = self.tiles_.find(key);
    if (found == self.tiles_.end()) {
        std::vector<std::uint32_t> base_pixels, extra_pixels;
        result = decode_type_tile(request, base_pixels, extra_pixels);
        if (result != DrawingStatus::drawn) return diagnose(result, "backend_result", __LINE__);
        MapTileData data{self.buffer(base_pixels), {}};
        if (!extra_pixels.empty()) data.extra = self.buffer(extra_pixels);
        found = self.tiles_.emplace(key, std::move(data)).first;
    }
    const auto push = [&](const TypeGpuPacket& packet, const std::shared_ptr<MapBufferData>& source) {
        if (self.scene_recording_) {
            self.scene_kind_ = &packet == &base ? std::uint8_t(BackgroundRecordKind::tile)
                                                : std::uint8_t(BackgroundRecordKind::tile_extra);
            self.append_scene(packet.parameters, source, palette, request.intensity);
            return;
        }
        self.append_direct(packet.parameters, source, palette.buffer);
    };
    if (type_gpu_packet_visible(base)) push(base, found->second.base);
    if (type_gpu_packet_visible(extra) && found->second.extra) push(extra, found->second.extra);
    return DrawingStatus::drawn;
}
namespace {
std::size_t sequence_hash(std::uintptr_t key) {
    std::uint64_t h = key >> 3;
    h *= 0x9E3779B97F4A7C15ull;
    return std::size_t(h >> 32);
}
}
FrameCompiler::ShapeSequence* FrameCompiler::find_sequence(const SHPStruct* image, bool deltas, bool add) {
    if (sequence_index_.empty()) {
        if (!add) return nullptr;
        sequence_index_.resize(1024);
    }
    // SHP objects are at least 2-byte aligned, so the low bit tells the role.
    const auto key = reinterpret_cast<std::uintptr_t>(image) | std::uintptr_t(deltas);
    auto probe = [&](std::uintptr_t wanted) -> SequenceSlot* {
        const auto mask = sequence_index_.size() - 1;
        for (auto at = sequence_hash(wanted) & mask;; at = (at + 1) & mask)
            if (!sequence_index_[at].key || sequence_index_[at].key == wanted) return &sequence_index_[at];
    };
    auto* slot = probe(key);
    if (slot->key) return sequences_[slot->id].get();
    if (!add) return nullptr;
    if ((sequences_.size() + 1) * 2 > sequence_index_.size()) {
        std::vector<SequenceSlot> grown(sequence_index_.size() * 2);
        sequence_index_.swap(grown);
        for (const auto& old : grown)
            if (old.key) *probe(old.key) = old;
        slot = probe(key);
    }
    auto created = std::make_unique<ShapeSequence>();
    created->image = image;
    created->deltas = deltas;
    created->id = std::uint32_t(sequences_.size());
    slot->key = key;
    slot->id = created->id;
    sequences_.push_back(std::move(created));
    return sequences_.back().get();
}
FrameCompiler::ShapeSequence& FrameCompiler::sequence(SHPStruct* image, bool deltas) {
    auto*& cached = sequence_cache_[deltas];
    auto* found = cached;
    if (!found || found->image != image) cached = found = find_sequence(image, deltas, true);
    found->used = item_serial_;
    // The version is rechecked only after a resource lifetime changed anywhere.
    if (found->generation != shape_resource_generation()) {
        std::uint64_t version = 0;
        if (!get_shape_resource_version(image, version)) throw std::runtime_error("shape resource version unavailable");
        if (found->version != version) {
            retire_sequence(*found);
            found->version = version;
        }
        found->generation = shape_resource_generation();
    }
    return *found;
}
void FrameCompiler::retire_sequence(ShapeSequence& sequence) {
    // Queued frames keep the storage alive through their sequence set or resources.
    if (sequence.data) sequence_dirty_ = true;
    sequence.data.reset();
    sequence.frames.clear();
    sequence.modes.clear();
    sequence.loaded = false;
}
void FrameCompiler::publish_sequences() {
    if (!sequence_dirty_ && sequence_set_) return;
    auto set = std::make_shared<MapSequenceSet>();
    set->revision = ++sequence_revision_;
    for (const auto& entry : sequences_)
        if (entry->data) set->data.push_back(entry->data);
    if (sequence_set_) retired_sequences_.push_back(std::move(sequence_set_));
    sequence_set_ = std::move(set);
    sequence_dirty_ = false;
}
void FrameCompiler::log_resource_pool(const char* event, std::size_t count, std::size_t bytes, double elapsed_us) const {
    static const bool enabled = [] {
        const auto* value = std::getenv("RA2_RESOURCE_STATS");
        return value && value[0] && std::strcmp(value, "0") != 0;
    }();
    if (!enabled) return;
    const auto pool = resource_pool_.statistics(), table = sequence_table_pool_.statistics();
    std::size_t resident_count = 0, resident_bytes = 0;
    for (const auto& entry : sequences_) if (entry->data) {
        ++resident_count;
        resident_bytes += entry->data->byte_count;
    }
    std::fprintf(stderr, "RESOURCE_POOL event=%s frame=%llu free_bytes=%llu largest_bytes=%llu slots=%u "
                 "table_free=%u table_largest=%u table_slots=%u releases=%llu released_bytes=%llu "
                 "evicted=%zu evicted_bytes=%zu sequences=%zu resident=%zu resident_bytes=%zu elapsed_us=%.3f\n", event,
                 (unsigned long long)item_serial_, (unsigned long long)pool.free_words * 4,
                 (unsigned long long)pool.largest_hole * 4, pool.free_slots,
                 table.free_words, table.largest_hole, table.free_slots,
                 (unsigned long long)pool.releases, (unsigned long long)pool.released_words * 4,
                 count, bytes, sequences_.size(), resident_count, resident_bytes, elapsed_us);
}
bool FrameCompiler::reclaim_resource(ResourcePressure pressure) {
    const auto started = std::chrono::steady_clock::now();
    // Used sequences have explicit frame ownership. Drop only this producer's
    // snapshots on actual pressure; queued consumers keep their own references.
    if (frame_) frame_->sequences.reset();
    sequence_set_.reset();
    retired_sequences_.clear();
    ShapeSequence* sequence = nullptr;
    VoxelEntry* voxel = nullptr;
    RasterContent* raster = nullptr;
    MapBufferData* candidate = nullptr;
    auto oldest = std::numeric_limits<std::uint64_t>::max();
    if (pressure != ResourcePressure::voxel_models)
        for (const auto& entry : sequences_)
            if (entry->data && entry->used != item_serial_ && entry->data.use_count() == 1 && entry->used < oldest) {
                oldest = entry->used;
                sequence = entry.get();
                candidate = entry->data.get();
            }
    if (pressure != ResourcePressure::sequence_table) {
        for (auto& [identity, entry] : voxel_images_)
            if (entry.resident && entry.used != item_serial_ && entry.resident.use_count() == 1 && entry.used < oldest) {
                oldest = entry.used;
                sequence = nullptr;
                voxel = &entry;
                candidate = entry.resident.get();
            }
        if (pressure == ResourcePressure::pixels)
            for (auto& [identity, entry] : raster_contents_)
                if (entry.source && entry.used != item_serial_ && entry.source.use_count() == 1 && entry.used < oldest) {
                    oldest = entry.used;
                    sequence = nullptr;
                    voxel = nullptr;
                    raster = &entry;
                    candidate = entry.source.get();
                }
    }
    const bool reclaimed = candidate != nullptr;
    const auto bytes = candidate ? candidate->word_count() * 4 : 0;
    if (sequence) retire_sequence(*sequence);
    else if (voxel) {
        voxel_cache_words_ -= voxel->resident->word_count();
        voxel->resident.reset();
        voxel_lru_.erase(voxel->position);
    } else if (raster) raster->source.reset();
    publish_sequences();
    if (frame_) frame_->sequences = sequence_set_;
    if (reclaimed) log_resource_pool("pressure-retire", 1, bytes,
        std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - started).count());
    return reclaimed;
}
std::shared_ptr<MapBufferData> FrameCompiler::byte_buffer(const std::vector<std::uint8_t>& bytes) {
    auto data = std::make_shared<MapBufferData>();
    data->id = next_id_++;
    // Whole 32-bit words; the pad bytes are never addressed.
    if (bytes.size() > std::numeric_limits<std::uint32_t>::max() - 3) throw std::length_error("drawing buffer size");
    data->byte_count = (bytes.size() + 3) / 4 * 4;
    data->bytes = std::make_unique_for_overwrite<std::uint8_t[]>(data->byte_count);
    if (data->byte_count > bytes.size())
        std::memset(data->bytes.get() + bytes.size(), 0, data->byte_count - bytes.size());
    if (!bytes.empty()) std::memcpy(data->bytes.get(), bytes.data(), bytes.size());
    return data;
}
DrawingStatus FrameCompiler::load_sequence(ShapeSequence& sequence, SHPStruct* image, bool prefix) {
    TypeShapeSequence decoded;
    const auto status =
        sequence.deltas ? decode_type_shape_deltas(image, decoded) : decode_type_shape_sequence(image, decoded, prefix);
    if (status != DrawingStatus::drawn) return status;
    const auto count = decoded.frames.size();
    sequence.frames.resize(count);
    std::shared_ptr<MapBufferData> data;
    if (!decoded.bytes.empty() && count) {
        auto table = std::make_shared<MapSequenceTable>();
        table->entries.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto& frame = decoded.frames[i];
            if (frame.status == DrawingStatus::drawn)
                table->entries[i] = {std::int32_t(frame.color), frame.bounds.Width, frame.bounds.Height,
                                     frame.prefix == TypeShapeNoPlane ? -1 : std::int32_t(frame.prefix)};
        }
        data = byte_buffer(decoded.bytes);
        try {
            table->allocation = sequence_table_pool_.allocate(std::uint32_t(count));
        } catch (const std::length_error&) {
            log_resource_pool("table-allocation-failed");
            for (;;) {
                if (!reclaim_resource(ResourcePressure::sequence_table)) throw;
                try {
                    table->allocation = sequence_table_pool_.allocate(std::uint32_t(count));
                    break;
                } catch (const std::length_error&) {}
            }
        }
        allocate_resource(data);
        data->table = table;
        sequence.entry_base = MapSequenceFrameBase + table->allocation->first;
        frame_->resources.push_back(data);
        sequence_dirty_ = true;
    }
    for (std::size_t i = 0; i < count; ++i)
        sequence.frames[i].status = decoded.frames[i].status;
    sequence.data = std::move(data);
    sequence.loaded = true;
    sequence.prefix = prefix;
    if (shape_stats_.enabled) {
        auto& stats = shape_stats_;
        const auto bytes = sequence.data ? sequence.data->byte_count : 0;
        ++stats.sequences;
        ++stats.total_sequences;
        stats.sequence_frames += count;
        stats.total_sequence_frames += count;
        stats.sequence_bytes += bytes;
        stats.total_sequence_bytes += bytes;
        ++stats.decodes;
        ++stats.total_decodes;
        if (announcing_) {
            ++stats.prewarmed;
            ++stats.total_prewarmed;
            stats.prewarm_bytes += bytes;
            stats.total_prewarm_bytes += bytes;
        } else if (item_serial_ > stats.first_serial + stats.late_after) {
            ++stats.late_decodes;
            ++stats.total_late;
            const auto* reference = image->AsReference();
            std::fprintf(stderr, "SHAPE_LATE image=%s deltas=%d frames=%zu bytes=%zu\n",
                         reference && reference->Filename ? reference->Filename : "<raw>", int(sequence.deltas), count,
                         std::size_t(bytes));
        }
    }
    return DrawingStatus::drawn;
}
DrawingStatus FrameCompiler::shape_sequence(void* pointer, SHPStruct* image, std::uint32_t use) noexcept {
    auto& self = *static_cast<FrameCompiler*>(pointer);
    if (!self.frame_ || !image || use > shape_sequence_depth) return DrawingStatus::invalid_argument;
    // Announcements may use half of the pool; the rest stays for drawing on demand.
    if (self.resource_pool_.free_words() < DrawResourcePool::DefaultWords / 2) return DrawingStatus::unavailable;
    struct Scope {
        bool& flag;
        explicit Scope(bool& f) : flag(f) { flag = true; }
        ~Scope() { flag = false; }
    } scope(self.announcing_);
    try {
        auto& sequence = self.sequence(image, use == shape_sequence_depth);
        if (sequence.loaded && (sequence.deltas || sequence.prefix || use != shape_sequence_color_depth))
            return DrawingStatus::drawn;
        if (sequence.loaded) self.retire_sequence(sequence);
        return self.load_sequence(sequence, image, use == shape_sequence_color_depth);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
// The frame table entry of one frame of a sequence, loading the sequence first.
DrawingStatus FrameCompiler::sequence_entry(ShapeSequence& sequence, SHPStruct* image, int frame, std::uint32_t& entry,
                                           bool prefix) {
    if (!sequence.loaded) {
        const auto loaded = load_sequence(sequence, image, prefix);
        if (loaded != DrawingStatus::drawn) return loaded;
    }
    if (frame < 0 || std::size_t(frame) >= sequence.frames.size()) {
        char detail[512];
        const auto* reference = image ? image->AsReference() : nullptr;
        std::snprintf(detail, sizeof(detail), "image=%s frame=%d expected_frame=[0,%zu)",
                      reference ? reference->Filename : "<raw>", frame, sequence.frames.size());
        return record_drawing_diagnostic(DrawingStatus::invalid_argument, "sequence_entry", "shape_frame_out_of_range",
                                         detail, __FILE__, __LINE__);
    }
    if (sequence.frames[std::size_t(frame)].status != DrawingStatus::drawn)
        return sequence.frames[std::size_t(frame)].status;
    if (sequence.data && sequence.data->submit_frame != item_serial_) {
        frame_->sequence_resources.push_back(sequence.data);
        sequence.data->submit_frame = item_serial_;
    }
    entry = sequence.entry_base + std::uint32_t(frame);
    return DrawingStatus::drawn;
}
bool FrameCompiler::append_plane(PixelParameters words, std::uint32_t entry,
                                const std::shared_ptr<MapBufferData>& palette) {
    retain_resource(palette);
    words[13] |= ShapePlanesFlag;
    auto& frame = *frame_;
    int variant = -1;
    const bool raster = raster_packet(words, frame.width, frame.height, variant);
    const auto first = std::uint32_t(raster ? frame.instances.size() : frame.compute.size());
    if (raster)
        frame.instances.push_back(raster_instance(words, int(entry), int(palette->allocation->first)));
    else {
        variant = -1;
        frame.compute.push_back(words);
        frame.compute_resources.push_back(int(entry));
        frame.compute_resources.push_back(int(palette->allocation->first));
    }
    append_run(first, variant);
    return raster;
}
// Find or create the mode program of a frame. A frame has few modes: the first
// InlineModes live in the frame, the rest in an overflow list.
FrameCompiler::ShapeMode& FrameCompiler::sequence_mode(ShapeSequence& sequence, ShapeFrame& frame,
                                                     const ShapeDrawingRequest& request, std::uint32_t aux,
                                                     bool& created) {
    const auto matches = [&](const ShapeMode& mode) {
        return mode.aux == aux && mode.flags == request.flags && mode.depth == request.depth_mode &&
               mode.blend == request.blend_mode && mode.gradient == request.gradient;
    };
    const unsigned inline_count = std::min<unsigned>(frame.modes, InlineModes);
    for (unsigned i = 0; i < inline_count; ++i) {
        auto& mode = sequence.modes[frame.inline_modes[i]];
        if (matches(mode)) {
            created = false;
            return mode;
        }
    }
    if (frame.more_modes)
        for (const auto index : *frame.more_modes) {
            auto& mode = sequence.modes[index];
            if (matches(mode)) {
                created = false;
                return mode;
            }
        }
    created = true;
    const auto index = std::uint32_t(sequence.modes.size());
    auto& mode = sequence.modes.emplace_back();
    mode.flags = request.flags;
    mode.depth = request.depth_mode;
    mode.blend = request.blend_mode;
    mode.gradient = request.gradient;
    mode.aux = aux;
    if (frame.modes < InlineModes)
        frame.inline_modes[frame.modes] = index;
    else {
        if (!frame.more_modes) frame.more_modes = std::make_unique<std::vector<std::uint32_t>>();
        frame.more_modes->push_back(index);
    }
    ++frame.modes;
    return mode;
}
void FrameCompiler::append_sequence_instance(RasterInstance instance, int variant, std::uint32_t entry,
                                            const std::shared_ptr<MapBufferData>& palette) {
    retain_resource(palette);
    instance[0] = int(entry);
    instance[1] = int(palette->allocation->first);
    const auto first = std::uint32_t(frame_->instances.size());
    frame_->instances.push_back(instance);
    append_run(first, variant);
}
DrawingStatus FrameCompiler::shape(void* pointer, const ShapeDrawingRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "shape", condition, request, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);
    const auto checked = [&](DrawingStatus status, const char* stage) {
        if (status != DrawingStatus::drawn && status != DrawingStatus::skipped && !self.error_[0]) {
            const auto* image = request.image ? request.image->AsReference() : nullptr;
            const auto* depth = request.depth_image ? request.depth_image->AsReference() : nullptr;
            std::snprintf(self.error_, sizeof(self.error_),
                          "SHP %s: %s; image=%s frame=%d flags=0x%X position=(%d,%d) clip=(%d,%d,%d,%d) depth_mode=%u "
                          "gradient=%d depth_image=%s depth_frame=%d depth_offset=(%d,%d)",
                          stage, drawing_status_name(status), image ? image->Filename : "<raw>", request.frame,
                          request.flags, request.position.X, request.position.Y, request.clip.X, request.clip.Y,
                          request.clip.Width, request.clip.Height, unsigned(request.depth_mode), request.gradient,
                          depth ? depth->Filename : "<none/raw>", request.depth_frame, request.depth_offset.X,
                          request.depth_offset.Y);
        }
        return diagnose(status, "backend_result", __LINE__);
    };
    if (!self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self) || !request.palette)
        return diagnose(checked(DrawingStatus::invalid_argument, "target/palette"), "backend_result", __LINE__);
    const auto& palette = *reinterpret_cast<const MapPaletteData*>(request.palette);
    // Immutable frame facts are decoded once per sequence. Modes are part of the
    // key: auxiliary-image validity depends on legacy flags, not only the image.
    if (!request.image) return DrawingStatus::skipped;
    request.image->Load();
    if (request.depth_image) request.depth_image->Load();
    auto& sequence = self.sequence(request.image);
    // Prefix planes exist only for images announced or first drawn with a SHA image; a plain
    // load that is later drawn with one is prepared again with them.
    if (request.depth_image && sequence.loaded && !sequence.prefix) self.retire_sequence(sequence);
    auto& stats = self.shape_stats_;
    if (stats.enabled) {
        ++stats.calls;
        ++stats.frame_calls;
        if (request.depth_image) ++stats.sha;
        if (sequence.counted != self.item_serial_) {
            sequence.counted = self.item_serial_;
            ++stats.shapes;
            ++stats.frame_shapes;
        }
    }
    const auto auxiliary = request.depth_image ? self.sequence(request.depth_image, true).version : 0;
    const auto* reference = request.image->AsReference();
    const auto* shape = reference ? reference->Data : request.image;
    if (!shape) return diagnose(checked(DrawingStatus::unavailable, "source"), "backend_result", __LINE__);
    if (request.frame < 0 || request.frame >= shape->Frames) {
        char detail[512];
        std::snprintf(detail, sizeof(detail), "image=%s frame=%d expected_frame=[0,%d)",
                      reference ? reference->Filename : "<raw>", request.frame, int(shape->Frames));
        record_drawing_diagnostic(DrawingStatus::invalid_argument, "shape", "shape_frame_out_of_range", detail,
                                  __FILE__, __LINE__, ordinal);
        return checked(DrawingStatus::invalid_argument, "frame");
    }
    if (sequence.frames.size() != std::size_t(shape->Frames)) sequence.frames.resize(shape->Frames);
    auto& frame = sequence.frames[request.frame];
    std::uint32_t aux = 0;
    if (request.depth_image) {
        if (!frame.aux) frame.aux = std::make_unique<std::vector<ShapeAux>>();
        auto& list = *frame.aux;
        std::size_t index = 0;
        for (; index < list.size(); ++index) {
            const auto& p = list[index];
            if (p.auxiliary == request.depth_image && p.version == auxiliary && p.frame == request.depth_frame &&
                p.x == request.depth_offset.X && p.y == request.depth_offset.Y)
                break;
        }
        if (index == list.size())
            list.push_back(
                {request.depth_image, auxiliary, request.depth_frame, request.depth_offset.X, request.depth_offset.Y});
        aux = std::uint32_t(index) + 1;
    }
    bool created = false;
    auto& mode = self.sequence_mode(sequence, frame, request, aux, created);
    auto& description = mode.description;
    if (created) {
        description.status = describe_type_shape(request, description.meta);
        if (description.status != DrawingStatus::drawn && description.status != DrawingStatus::skipped)
            return checked(description.status, "description");
        if (description.status == DrawingStatus::drawn)
            description.compiled =
                compile_type_shape(request, description.meta, description.program) == DrawingStatus::drawn;
        if (description.compiled && !raster_packet(description.program.words, 8192, 8192, description.variant))
            description.variant = -1;
        if (stats.enabled) {
            ++stats.descriptions;
            ++stats.total_descriptions;
        }
    }
    if (stats.enabled && mode.counted != self.item_serial_) {
        mode.counted = self.item_serial_;
        ++stats.modes;
        ++stats.frame_modes;
    }
    const auto described = description.status;
    const auto& meta = description.meta;
    auto& counters = self.shape_stats_;
    // The planes of this frame and, for auxiliary depth, of its SHA frame.
    const auto planes = [&](bool auxiliary_depth, std::uint32_t& entry,
                            std::uint32_t& auxiliary_entry) -> DrawingStatus {
        auto status =
            self.sequence_entry(sequence, request.image, request.frame, entry, request.depth_image != nullptr);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        if (auxiliary_depth)
            status = self.sequence_entry(self.sequence(request.depth_image, true), request.depth_image,
                                         request.depth_frame, auxiliary_entry, false);
        return diagnose(status, "backend_result", __LINE__);
    };
    if (described == DrawingStatus::drawn && description.variant >= 0 && !self.scene_recording_) {
        RasterInstance instance;
        const auto status = emit_type_shape_instance(
            request, description.program, {self.frame_->width, self.frame_->height, palette.shades, 0, 0x8000, true},
            instance);
        if (status != DrawingStatus::drawn) return checked(status, "instance");
        std::uint32_t entry = 0, auxiliary_entry = 0;
        const auto loaded =
            planes(description.program.auxiliary_depth() && request.depth_mode == ShapeDepthMode::legacy, entry,
                   auxiliary_entry);
        if (loaded != DrawingStatus::drawn) return checked(loaded, "decode");
        if (auxiliary_entry) instance[14] = std::int32_t(auxiliary_entry);
        instance[9] |= ShapePlanesFlag;
        self.append_sequence_instance(instance, description.variant, entry, palette.buffer);
        if (counters.enabled) ++counters.raster;
        return DrawingStatus::drawn;
    }
    TypeGpuPacket packet;
    auto status = described;
    if (status == DrawingStatus::drawn) {
        const TypeGpuTarget target{self.frame_->width, self.frame_->height, palette.shades, 0, 0x8000, true};
        status = description.compiled ? emit_type_shape(request, description.program, target, packet)
                                      : prepare_type_shape_parameters(request, meta, target, packet);
    }
    if (status != DrawingStatus::drawn) {
        // Skips from the image alone (an empty frame or auxiliary image) hold
        // at every view; a clip skip or a failure does not.
        const bool image_skip = status == DrawingStatus::skipped &&
                                (described == DrawingStatus::skipped || meta.body.Width <= 0 || meta.body.Height <= 0);
        if (self.scene_recording_ && status == DrawingStatus::skipped && !image_skip)
            return diagnose(checked(DrawingStatus::unsupported, "background recording clip"), "backend_result",
                            __LINE__);
        return checked(status, "parameters");
    }
    // Compute packets (plain warp reads) and background scene records draw from the same planes.
    auto& words = packet.parameters;
    const bool auxiliary_depth =
        (words[13] & 0x20000) != 0 && (words[13] & 0xFF) != 0 && request.depth_mode == ShapeDepthMode::legacy;
    std::uint32_t entry = 0, auxiliary_entry = 0;
    const auto loaded = planes(auxiliary_depth, entry, auxiliary_entry);
    if (loaded != DrawingStatus::drawn) return checked(loaded, "decode");
    if (auxiliary_depth) {
        words[15] = std::int32_t(auxiliary_entry);
        words[18] = std::int32_t(unsigned(meta.auxiliary_offset.X) | (unsigned(meta.auxiliary_offset.Y) << 16));
    }
    if (!self.scene_recording_) {
        const bool raster = self.append_plane(words, entry, palette.buffer);
        if (counters.enabled) ++(raster ? counters.raster : counters.compute);
        return DrawingStatus::drawn;
    }
    if (counters.enabled) ++counters.scene;
    self.scene_kind_ = std::uint8_t(BackgroundRecordKind::shape);
    self.append_scene(words, sequence.data, palette, request.intensity,
                      request.depth_mode == ShapeDepthMode::legacy ? request.gradient : -1, request.depth_adjustment,
                      meta.rle, entry, auxiliary_depth ? self.sequence(request.depth_image, true).data : nullptr);
    return DrawingStatus::drawn;
}

FrameCompiler::LightingAsset& FrameCompiler::lighting_asset(SHPStruct* image) {
    auto& asset = lighting_assets_[image];
    if (asset.generation != shape_resource_generation()) {
        std::uint64_t version = 0;
        if (!get_shape_resource_version(image, version)) throw std::runtime_error("Lighting image version unavailable");
        if (asset.version != version) {
            for (const auto& frame : asset.frames)
                if (frame.image.source) --lighting_frame_count_;
            asset.frames.clear();
            asset.set.reset();
            asset.version = version;
        }
        asset.generation = shape_resource_generation();
    }
    return asset;
}
DrawingStatus FrameCompiler::lighting_image(SHPStruct* image, int frame, const MapLightingImage*& output) {
    if (frame < 0 || frame > 0xFFFF) return DrawingStatus::invalid_argument;
    auto& asset = lighting_asset(image);
    if (asset.frames.size() <= std::size_t(frame)) asset.frames.resize(std::size_t(frame) + 1);
    auto& entry = asset.frames[std::size_t(frame)];
    if (!entry.decoded) {
        LightingImageData decoded;
        entry.status = decode_lighting_image(image, frame, decoded);
        if (entry.status == DrawingStatus::drawn) {
            entry.image = {decoded.bounds, buffer(decoded.texels)};
            allocate_resource(entry.image.source);
            ++lighting_frame_count_;
        }
        entry.decoded = true;
    }
    output = &entry.image;
    return entry.status;
}
DrawingStatus FrameCompiler::lighting_shape(void* pointer, const LightingShapeDrawingRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "lighting_shape", condition, request, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);

    if (!self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self)",
                        __LINE__);
    const MapLightingImage* image = nullptr;
    const auto decoded = self.lighting_image(request.image, request.frame, image);
    if (decoded != DrawingStatus::drawn) return diagnose(decoded, "lighting_image_decode_failed", __LINE__);
    LightingParameters words;
    const auto status =
        prepare_lighting_parameters(request, image->bounds, self.frame_->width, self.frame_->height, words);
    if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
    self.append_lighting(words, image->source);
    return DrawingStatus::drawn;
}
DrawingStatus FrameCompiler::lighting_shape_batch(void* pointer, const LightingShapeBatchRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "lighting_shape_batch", condition, request, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);

    if (!self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self)",
                        __LINE__);
    const auto one_by_one = [&] {
        auto result = DrawingStatus::skipped;
        for (std::uint32_t i = 0; i < request.count; ++i) {
            const auto status = lighting_shape(pointer, request.entries[i].request);
            if (status != DrawingStatus::drawn && status != DrawingStatus::skipped)
                return diagnose(status, "backend_result", __LINE__);
            if (status == DrawingStatus::drawn) result = status;
        }
        return diagnose(result, "backend_result", __LINE__);
    };
    MapLightField field;
    LightFieldPlan plan;
    auto& frame = *self.frame_;
    std::shared_ptr<const MapLightingSet> image_set;
    std::vector<LightingParameters> others;
    std::shared_ptr<MapLightingSet> images;
    if (request.shroud) {
        if (request.count) return diagnose(DrawingStatus::invalid_argument, "rejected when request.count", __LINE__);
        field.world = true;
        field.view = request.view;
        field.camera = request.shroud_camera;
        plan.kx = request.shroud_origin.X;
        plan.ky = request.shroud_origin.Y;
        plan.cell_image = request.shroud;
        plan.frames = request.shroud->Frames;
        auto& asset = self.lighting_asset(request.shroud);
        if (!asset.set) {
            images = std::make_shared<MapLightingSet>();
            images->frames.resize(std::size_t(std::max(0, plan.frames)));
            for (int index = 0; index < plan.frames; ++index) {
                const MapLightingImage* image = nullptr;
                const auto status = self.lighting_image(request.shroud, index, image);
                if (status == DrawingStatus::skipped) continue;
                if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
                images->frames[std::size_t(index)] = *image;
            }
        } else
            image_set = asset.set;
    } else {
        if (!plan_light_field(request, plan)) return one_by_one();
        images = std::make_shared<MapLightingSet>();
        images->frames.resize(std::size_t(plan.frames));
        for (std::uint32_t i = 0; i < plan.first_other; ++i) {
            const int index = request.entries[i].request.frame;
            if (images->frames[index].source) continue;
            const MapLightingImage* image = nullptr;
            const auto status = self.lighting_image(request.entries[i].request.image, index, image);
            if (status == DrawingStatus::skipped) continue;
            if (status != DrawingStatus::drawn) return one_by_one();
            images->frames[index] = *image;
        }
    }
    if (images) {
        std::vector<std::uint32_t> table;
        for (const auto& image : images->frames) {
            const auto& b = image.bounds;
            table.insert(table.end(), {image.source ? image.source->allocation->first : 0xFFFFFFFFu, std::uint32_t(b.X),
                                       std::uint32_t(b.Y), std::uint32_t(b.Width), std::uint32_t(b.Height)});
        }
        if (table.empty()) table.push_back(0);
        images->residency = self.buffer(table);
        for (const auto& image : images->frames)
            if (image.source) images->residency->dependencies.push_back(image.source);
        self.allocate_resource(images->residency);
        image_set = images;
        if (request.shroud) self.lighting_asset(request.shroud).set = images;
    }
    self.retain_resource(image_set->residency);
    field.image_version = image_set->residency->allocation->version;
    for (std::uint32_t i = plan.first_other; i < request.count; ++i) {
        const auto& q = request.entries[i].request;
        const MapLightingImage* image = nullptr;
        auto status = self.lighting_image(q.image, q.frame, image);
        if (status == DrawingStatus::skipped) continue;
        if (status != DrawingStatus::drawn) return field.world ? status : one_by_one();
        LightingParameters words;
        status = prepare_lighting_parameters(q, image->bounds, frame.width, frame.height, words);
        if (status == DrawingStatus::skipped) continue;
        if (status != DrawingStatus::drawn) return field.world ? status : one_by_one();
        self.retain_resource(image->source);
        words[17] = int(image->source->allocation->first);
        others.push_back(words);
    }
    auto& words = frame.field_words;
    field.program = std::int32_t(words.size());
    field.origin = {plan.kx, plan.ky};
    words.resize(words.size() + 20);
    const auto put = [&](const std::vector<std::int32_t>& data) {
        const auto at = std::int32_t(words.size());
        words.insert(words.end(), data.begin(), data.end());
        return at;
    };
    const auto grid = put(plan.grid), cells = put(plan.cells), other_base = std::int32_t(words.size());
    for (const auto& entry : others)
        words.insert(words.end(), entry.begin(), entry.end());
    std::vector<std::int32_t> bins;
    if (!others.empty()) append_lighting_bins(others.data(), 0, int(others.size()), frame.width, frame.height, bins);
    const auto bin_table = put(bins);
    const std::int32_t header[20]{plan.kx,
                                  plan.ky,
                                  plan.a0,
                                  plan.s0,
                                  plan.columns,
                                  plan.rows,
                                  grid,
                                  cells,
                                  int(image_set->residency->allocation->first),
                                  int(image_set->frames.size()),
                                  other_base,
                                  bin_table,
                                  (frame.width + LightingBinSize - 1) / LightingBinSize,
                                  int(others.size()),
                                  field.world ? 1 : 0,
                                  field.view.X,
                                  field.view.Y,
                                  field.view.Width,
                                  field.view.Height,
                                  0};
    std::copy(std::begin(header), std::end(header), words.begin() + field.program);
    frame.commands.push_back({MapCommandKind::light_field, std::uint32_t(frame.light_fields.size())});
    frame.light_fields.push_back(std::move(field));
    return DrawingStatus::drawn;
}
DrawingStatus FrameCompiler::raster(void* pointer, const RasterDrawingRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "raster", condition, request, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);

    if (!self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !self.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&self)",
                        __LINE__);
    const int w = self.frame_->width, h = self.frame_->height;
    if (request.blend_mode == RasterBlendMode::wave) {
        std::vector<std::uint32_t> words;
        words.reserve(std::size_t(request.wave_pixel_count) * 5);
        for (unsigned i = 0; i < request.wave_pixel_count; ++i) {
            const auto& p = request.wave_pixels[i];
            words.insert(words.end(), {unsigned(p.destination.X), unsigned(p.destination.Y), unsigned(p.sample.X),
                                       unsigned(p.sample.Y), unsigned(p.strength)});
        }
        PixelParameters parameters{
            w, h, 1, 1, 0, 0, request.clip.X, request.clip.Y, request.clip.Width, request.clip.Height};
        parameters[10] = int(request.wave_pixel_count);
        parameters[11] = request.color;
        parameters[14] = 0x8000 + request.line_z;
        parameters[19] = 12;
        const auto source = self.buffer(words);
        self.append_direct(parameters, source, source);
        return DrawingStatus::drawn;
    }

    const auto left = std::max({std::int64_t(0), std::int64_t(request.position.X), std::int64_t(request.clip.X)});
    const auto top = std::max({std::int64_t(0), std::int64_t(request.position.Y), std::int64_t(request.clip.Y)});
    const auto right = std::min({std::int64_t(w), std::int64_t(request.position.X) + request.width,
                                 std::int64_t(request.clip.X) + request.clip.Width});
    const auto bottom = std::min({std::int64_t(h), std::int64_t(request.position.Y) + request.height,
                                  std::int64_t(request.clip.Y) + request.clip.Height});
    if (right <= left || bottom <= top) return DrawingStatus::skipped;
    if (request.content_identity) {
        auto& content = self.raster_contents_[request.content_identity];
        content.used = self.item_serial_;
        if (!content.source || content.version != request.content_version) {
            std::vector<std::uint32_t> words(std::size_t(request.width) * request.height);
            for (int y = 0; y < request.height; ++y)
                for (int x = 0; x < request.width; ++x) {
                    const auto pixel = request.pixels[std::size_t(y) * request.pitch + x];
                    words[std::size_t(y) * request.width + x] =
                        request.blend_mode == RasterBlendMode::copy ? rgba565(pixel) : pixel;
                }
            content.source = self.buffer(words);
            content.version = request.content_version;
            content.width = request.width;
            content.height = request.height;
            content.pitch = request.pitch;
            content.mode = request.blend_mode;
        } else if (content.width != request.width || content.height != request.height ||
                   content.pitch != request.pitch || content.mode != request.blend_mode)
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when content.width != request.width || content.height != request.height || "
                            "content.pitch != request.pitch || content.mode != request.blend_mode",
                            __LINE__);
        auto& dummy = self.fills_[0];
        if (!dummy) dummy = self.buffer({rgba565(0)});
        PixelParameters words{
            w,         h,        request.width,     request.height,   request.position.X, request.position.Y,
            int(left), int(top), int(right - left), int(bottom - top)};
        words[19] = request.blend_mode == RasterBlendMode::copy ? 1 : 5;
        if (request.blend_mode == RasterBlendMode::spotlight) {
            words[10] = 1;
            words[11] = int(request.spotlight_flags);
            words[12] = request.color;
            words[14] = std::uint16_t(0x8000u - std::uint32_t(request.position.Y) - std::uint32_t(request.clip.Y) +
                                      std::uint32_t(request.line_z));
        }
        self.append_direct(words, content.source, dummy);
        return DrawingStatus::drawn;
    }
    if (request.blend_mode == RasterBlendMode::shroud || request.blend_mode == RasterBlendMode::fog ||
        request.blend_mode == RasterBlendMode::alpha_shape) {
        std::vector<std::uint32_t> pixels(std::size_t(right - left) * std::size_t(bottom - top));
        for (auto y = top; y < bottom; ++y)
            for (auto x = left; x < right; ++x) {
                const auto value =
                    request.pixels ? request.pixels[(y - request.position.Y) * request.pitch + x - request.position.X]
                                   : request.color;
                pixels[(y - top) * (right - left) + x - left] = value <= 255 ? (0x10000u | value) : 0;
            }
        LightingParameters words{w,        h,         int(right - left), int(bottom - top), int(left),
                                 int(top), int(left), int(top),          int(right - left), int(bottom - top)};
        words[19] = request.blend_mode == RasterBlendMode::shroud ? 1
                    : request.blend_mode == RasterBlendMode::fog  ? 2
                                                                  : 3;
        self.append_lighting(words, self.buffer(pixels));
        return DrawingStatus::drawn;
    }
    std::shared_ptr<MapBufferData> source;
    auto& dummy = self.fills_[0];
    if (!dummy) dummy = self.buffer({rgba565(0)});
    if (request.pixels) {
        std::vector<std::uint32_t> pixels(std::size_t(right - left) * std::size_t(bottom - top));
        for (auto y = top; y < bottom; ++y)
            for (auto x = left; x < right; ++x)
                pixels[(y - top) * (right - left) + x - left] =
                    (request.blend_mode == RasterBlendMode::copy
                         ? rgba565(request.pixels[(y - request.position.Y) * request.pitch + x - request.position.X])
                         : request.pixels[(y - request.position.Y) * request.pitch + x - request.position.X]);
        source = self.buffer(pixels);
    } else {
        auto& cached = self.fills_[request.color];
        if (!cached) cached = self.buffer({rgba565(request.color)});
        source = cached;
    }
    PixelParameters parameters;
    parameters = {w,        h,         int(right - left), int(bottom - top), int(left),
                  int(top), int(left), int(top),          int(right - left), int(bottom - top)};
    parameters[19] = request.pixels ? 1 : 2;
    if (request.original_line) {
        parameters[19] = 4;
        parameters[13] = 2;
        parameters[14] = std::uint16_t(0x8000u - std::uint32_t(request.position.Y) - std::uint32_t(request.clip.Y) +
                                       std::uint32_t(request.line_z));
    }
    if (request.blend_mode == RasterBlendMode::light_nonzero || request.blend_mode == RasterBlendMode::light_zero) {
        TypeGpuPacket encoded;
        const auto status = prepare_type_light_test_parameters(request, {w, h, 0, 0, 0x8000, true}, encoded);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        parameters = encoded.parameters;
    } else if (request.blend_mode == RasterBlendMode::particle) {
        TypeGpuPacket encoded;
        const auto status = prepare_type_particle_parameters(request, {w, h, 0, 0, 0x8000, true}, encoded);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        parameters = encoded.parameters;
    } else if (request.blend_mode == RasterBlendMode::depth_additive ||
               request.blend_mode == RasterBlendMode::depth_laser_pair) {
        TypeGpuPacket encoded;
        const auto status = prepare_type_laser_parameters(request, {w, h, 0, 0, 0x8000, true}, encoded);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        parameters = encoded.parameters;
    } else if (request.blend_mode == RasterBlendMode::depth_alpha) {
        if (request.line_opacity < 8) return DrawingStatus::skipped;
        parameters[19] = 7;
        parameters[11] = int(request.line_rgb);
        parameters[12] = request.line_opacity;
        parameters[14] = std::uint16_t(0x8000u - std::uint32_t(request.position.Y) - std::uint32_t(request.clip.Y) +
                                       std::uint32_t(request.line_z));
    } else if (request.blend_mode != RasterBlendMode::copy) {
        parameters[19] = request.blend_mode == RasterBlendMode::spotlight ? 5 : 6;
        parameters[11] = int(request.spotlight_flags);
        parameters[12] = request.blend_mode == RasterBlendMode::depth_glow ? request.light_strength : request.color;
        parameters[10] = request.pixels ? 1 : 0;
        parameters[14] = std::uint16_t(0x8000u - std::uint32_t(request.position.Y) - std::uint32_t(request.clip.Y) +
                                       std::uint32_t(request.line_z));
    }
    self.append_direct(parameters, source, dummy, !request.pixels);
    return DrawingStatus::drawn;
}
std::shared_ptr<MapBufferData> FrameCompiler::voxel_source(const VoxelBufferView& view) {
    if (!view.identity || !view.words || !view.count) throw std::invalid_argument("voxel source");
    auto& weak = voxel_sources_[view.identity];
    if (auto cached = weak.lock()) return cached;
    auto cached = buffer(view.words, view.count);
    cached->voxel_model = true;
    allocate_resource(cached);
    weak = cached;
    return cached;
}
std::shared_ptr<MapBufferData> FrameCompiler::voxel_image(const VoxelImageHandle& handle, unsigned depth) {
    if (depth > 32) throw std::invalid_argument("voxel dependency depth");
    const auto identity = voxel_image_identity(handle);
    if (const auto found = voxel_images_.find(identity); found != voxel_images_.end())
        if (auto image = found->second.image.lock()) {
            found->second.used = item_serial_;
            auto& entry = found->second;
            if (!entry.resident) {
                entry.resident = image;
                voxel_cache_words_ += image->word_count();
                voxel_lru_.push_back(identity);
                entry.position = std::prev(voxel_lru_.end());
            } else
                voxel_lru_.splice(voxel_lru_.end(), voxel_lru_, entry.position);
            return image;
        }
    VoxelImageView view;
    if (!read_voxel_image(handle, view)) throw std::invalid_argument("voxel image");
    auto result = std::make_shared<MapBufferData>();
    result->id = next_id_++;
    result->generated_words = std::size_t(view.width) * view.height;
    if (view.reference_count)
        result->voxel_reference.assign(view.reference_pixels, view.reference_pixels + view.reference_count);
    result->voxel = std::make_shared<MapVoxelImage>();
    auto& image = *result->voxel;
    image.kind = static_cast<render::VoxelKind>(view.kind);
    image.width = view.width;
    image.height = view.height;
    image.left = view.left;
    image.top = view.top;
    image.buffered = view.buffered;
    for (unsigned i = 0; i < view.section_count; ++i) {
        std::array<std::int32_t, 16> p;
        std::copy_n(view.sections[i].parameters, 16, p.begin());
        image.sections.push_back(p);
        auto source = voxel_source(view.sections[i].source);
        image.sources.push_back(source);
        result->dependencies.push_back(std::move(source));
    }
    if (view.kind == VoxelImageKind::raster) {
        std::copy_n(view.lighting, 256, image.lighting.begin());
        image.palette = voxel_source(view.palette);
        result->dependencies.push_back(image.palette);
    }
    if (view.kind == VoxelImageKind::shadow) {
        std::copy_n(view.shadow, 10, image.shadow.begin());
        image.columns = voxel_source(view.columns);
        result->dependencies.push_back(image.columns);
    }
    if (view.mask) {
        image.mask = voxel_image(*view.mask, depth + 1);
        result->dependencies.push_back(image.mask);
    }
    for (unsigned i = 0; i < view.layer_count; ++i) {
        const auto& layer = view.layers[i];
        auto child = voxel_image(*layer.image, depth + 1);
        image.layers.push_back({child, layer.position, layer.clip});
        result->dependencies.push_back(std::move(child));
    }
    // The original valid-key cache already owns the immutable result identity.
    // Composite identities are interned from exact immutable child identities.
    allocate_resource(result, view.width, view.height);
    voxel_lru_.push_back(identity);
    voxel_images_[identity] = VoxelEntry{result, result, item_serial_, std::prev(voxel_lru_.end())};
    voxel_cache_words_ += result->word_count();
    return result;
}
DrawingStatus FrameCompiler::indexed(void* pointer, const IndexedDrawingRequest& r) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "indexed", condition, r, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);

    if (!self.frame_ || r.target != reinterpret_cast<DrawingTargetHandle*>(&self) || !r.palette)
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when !self.frame_ || r.target != reinterpret_cast<DrawingTargetHandle*>(&self) || !r.palette",
            __LINE__);
    const auto& palette = *reinterpret_cast<const MapPaletteData*>(r.palette);
    std::shared_ptr<MapBufferData> source;
    TypeGpuPacket prepared;
    const auto result = prepare_type_indexed_parameters(
        r, {self.frame_->width, self.frame_->height, palette.shades, 0, 0x8000, true}, prepared);
    if (result != DrawingStatus::drawn) return diagnose(result, "backend_result", __LINE__);

    // The borrowed pointer only locates a candidate; it is never dereferenced
    // after this call. Reuse requires equal bytes, and a change always gets a
    // new immutable buffer because queued packets may still own the old one.
    if (r.voxel)
        source = self.voxel_image(*r.voxel);
    else {
        const auto count = std::size_t(r.width) * r.height;
        auto& cached = self.indexed_[{r.pixels, r.width, r.height}];
        if (!cached.buffer || std::memcmp(cached.buffer->bytes.get(), r.pixels, count * 4) != 0)
            cached.buffer = self.buffer(r.pixels, count);
        cached.used = self.frame_serial_;
        source = cached.buffer;
    }
    self.append_direct(prepared.parameters, source, palette.buffer);
    return DrawingStatus::drawn;
}
}

namespace game {
DrawingStatus FrameCompiler::screen_effect(void* context, const ScreenEffectDrawingRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "screen_effect", condition, request, __FILE__, line, ordinal);
    };
    auto& resources = *static_cast<FrameCompiler*>(context);
    if (!resources.frame_) return DrawingStatus::unavailable;
    render::ScreenEffect mark;
    mark.capture_id = request.capture_id;
    mark.stage = static_cast<render::ScreenEffectStage>(request.stage);
    mark.operation = static_cast<render::ScreenEffectOperation>(request.operation);
    mark.width = request.width; mark.height = request.height; mark.brightness = request.brightness;
    mark.warp_step = request.warp_step; mark.initial_duration = request.initial_duration;
    mark.center_x = request.center_x; mark.center_y = request.center_y; mark.depth_base = request.depth_base;
    resources.frame_->commands.push_back(
        {MapCommandKind::effect, std::uint32_t(resources.frame_->screen_effect_marks.size())});
    resources.frame_->screen_effect_marks.push_back({mark});
    if (request.operation == ScreenEffectOperation::apply) resources.frame_->screen_effect = mark;
    return DrawingStatus::drawn;
}
}

namespace game {
DrawingStatus FrameCompiler::background_complete(void* context, const BackgroundDrawingRequest& request) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "background_complete", condition, request, __FILE__, line, ordinal);
    };
    auto& resources = *static_cast<FrameCompiler*>(context);
    if (!resources.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&resources))
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when !resources.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&resources)",
            __LINE__);
    auto& frame = *resources.frame_;
    frame.view_origin = request.view_origin;
    return DrawingStatus::drawn;
}
}

namespace game {
namespace {
constexpr std::uint16_t UnlitBound = 0x80;  // a held item's light cannot be derived
int scene_intensity(const BackgroundSceneLight& light, MapLightSource source) {
    switch (source) {
        case MapLightSource::terrain_signed:
            return std::int16_t(light.terrain);
        case MapLightSource::terrain:
            return light.terrain;
        case MapLightSource::normal_signed:
            return std::int16_t(light.normal);
        case MapLightSource::normal:
            return light.normal;
        case MapLightSource::full:
            return 1000;
        default:
            return 0;
    }
}
bool same_tint(const BackgroundSceneTint& a, const BackgroundSceneTint& b) {
    return a.red == b.red && a.green == b.green && a.blue == b.blue && a.shades == b.shades;
}
}
void FrameCompiler::clear_scene_lighting() {
    scene_light_.clear();
    scene_light_use_.clear();
    scene_light_chunks_.clear();
    scene_light_dirty_.clear();
    scene_shroud_chunks_.clear();
    scene_shroud_dirty_.clear();
    scene_alphas_.reset();
    scene_alphas_dirty_ = true;
    scene_converter_slots_.clear();
    scene_converters_.clear();
    scene_converters_dirty_ = true;
}
void FrameCompiler::set_scene_light(const BackgroundSceneLight& light) {
    const auto cell = std::uint32_t(light.cell.Y) * 512 + std::uint32_t(light.cell.X);
    scene_light_[cell] = light;
    const auto chunk = cell / MapLightChunkCells;
    if (scene_light_chunks_.size() <= chunk) {
        scene_light_chunks_.resize(chunk + 1);
        scene_light_dirty_.resize(chunk + 1, 0);
    }
    if (!scene_light_dirty_[chunk]) {
        auto copy = scene_light_chunks_[chunk] ? std::make_shared<MapSceneLightChunk>(*scene_light_chunks_[chunk])
                                               : std::make_shared<MapSceneLightChunk>();
        scene_light_chunks_[chunk] = std::move(copy);
        scene_light_dirty_[chunk] = 1;
    }
    auto* words = scene_light_chunks_[chunk]->words.data() + std::size_t(cell % MapLightChunkCells) * MapLightWords;
    const auto& t0 = light.tint[0];
    const auto& t1 = light.tint[1];
    words[0] = std::int32_t(std::uint32_t(light.normal) | (std::uint32_t(light.terrain) << 16));
    words[1] = std::int32_t((std::uint32_t(t0.shades) & 0xFFFFu) | (std::uint32_t(t1.shades) << 16));
    words[2] = t0.factor_red;
    words[3] = t0.factor_green;
    words[4] = t0.factor_blue;
    words[5] = t1.factor_red;
    words[6] = t1.factor_green;
    words[7] = t1.factor_blue;
}
// Every light-source combination a held item of the cell matched must still
// give one value: then the shader's choice of the first source is exact.
bool FrameCompiler::scene_light_current(std::uint32_t cell, const BackgroundSceneLight& light) const {
    const auto found = scene_light_use_.find(cell);
    if (found == scene_light_use_.end()) return true;
    for (const auto use : found->second) {
        if (use & UnlitBound) return false;
        int value = 0;
        bool first = true;
        for (int i = 0; i < 5; ++i) {
            if (!(use & (1u << i))) continue;
            const int v = scene_intensity(light, MapLightSource(i + 1));
            if (!first && v != value) return false;
            value = v;
            first = false;
        }
        if ((use & 0x60) == 0x60 && !same_tint(light.tint[0], light.tint[1])) return false;
    }
    return true;
}
std::shared_ptr<MapBufferData> FrameCompiler::base_palette(const BytePalette& source) {
    auto& cached = base_palettes_[&source];
    if (cached) return cached;
    // The unlit row of a terrain palette, then the PAL entries the shader tints.
    std::vector<WORD> unlit(256);
    if (!ConvertClass::BuildColorTable(source, unlit.data(), 256, 1, 2))
        throw std::runtime_error("Base palette conversion");
    std::vector<std::uint32_t> words(512);
    for (int i = 0; i < 256; ++i) {
        words[i] = rgba565(unlit[i]);
        const auto& entry = source.Entries[i];
        words[256 + i] = std::uint32_t(entry.R) | (std::uint32_t(entry.G) << 8) | (std::uint32_t(entry.B) << 16);
    }
    cached = buffer(words);
    return cached;
}
// Decide where a recorded packet takes its light from, against the light of
// the item's cell sent before recording. Shadows use neither intensity nor
// palette; unlit packets use only a palette's unlit row, which a cell's tint
// does not change.
void FrameCompiler::append_scene(const PixelParameters& packet, const std::shared_ptr<MapBufferData>& source,
                                const MapPaletteData& palette, int intensity, int gradient, int adjustment, bool rle,
                                std::uint32_t entry, const std::shared_ptr<MapBufferData>& auxiliary) {
    auto a = packet;
    if (entry) a[13] |= ShapePlanesFlag;
    int variant = -1;
    if (a[19] != 0 || a[16] != 0 || a[4] < a[6] || a[5] < a[7] || a[4] + a[2] > a[6] + a[8] ||
        a[5] + a[3] > a[7] + a[9] || !raster_packet(a, a[0], a[1], variant))
        throw std::runtime_error("Background item was clipped or used an unsupported drawing mode");
    const auto& item = scene_request_;
    const bool shadow = ((a[13] >> 8) & 0xFF) == 1, lit = (a[10] & 1) != 0;
    const auto cell = std::uint32_t(item.cell.Y) * 512 + std::uint32_t(item.cell.X);
    auto intensity_source = MapLightSource::recorded;
    auto palette_source = MapPaletteSource::recorded;
    auto colors = palette.buffer;
    int converter = -1;
    std::uint16_t use = 0;
    if (!shadow) {
        const auto found = scene_light_.find(cell);
        if (lit) {
            unsigned mask = 0;
            if (found != scene_light_.end())
                for (int i = 0; i < 5; ++i)
                    if (scene_intensity(found->second, MapLightSource(i + 1)) == intensity) mask |= 1u << i;
            if (mask) {
                intensity_source = MapLightSource(std::countr_zero(mask) + 1);
                use |= std::uint16_t(mask);
            } else
                use |= UnlitBound;
        }
        if (palette.converter && palette.identity) {
            const auto [slot, inserted] =
                scene_converter_slots_.try_emplace(palette.identity, std::int32_t(scene_converters_.size()));
            if (inserted)
                scene_converters_.push_back(colors);
            else
                scene_converters_[std::size_t(slot->second)] = colors;
            scene_converters_dirty_ = true;
            palette_source = MapPaletteSource::converter;
            converter = slot->second;
        } else if (palette.tint_source) {
            unsigned mask = 0;
            if (found != scene_light_.end())
                for (int i = 0; i < 2; ++i) {
                    const auto& t = found->second.tint[i];
                    if (t.red == palette.red && t.green == palette.green && t.blue == palette.blue &&
                        t.shades == palette.shades)
                        mask |= 1u << i;
                }
            if (mask) {
                palette_source = mask & 1 ? MapPaletteSource::cell_tint : MapPaletteSource::sprite_tint;
                colors = base_palette(*palette.tint_source);
                use |= std::uint16_t(mask << 5);
            } else if (lit)
                use |= UnlitBound;
        }
    }
    if (use) {
        auto& uses = scene_light_use_[cell];
        if (std::find(uses.begin(), uses.end(), use) == uses.end()) uses.push_back(use);
    }
    if (!entry) allocate_resource(source, a[2], a[3]);
    allocate_resource(colors);
    const auto metadata = (std::uint32_t(a[10]) & 0xFFFF0000u) | unsigned(a[12] - 1) | unsigned(a[10] & 1) << 8 |
                          unsigned(a[17] != 0) << 9 | unsigned(rle) << 10 | unsigned(gradient + 1) << 11;
    MapSceneRecord record;
    record.words = {
        entry ? int(entry) : int(source->allocation->slot),
        converter >= 0 ? converter : int(colors->allocation->first),
        a[2] | (a[3] << 16),
        a[4] + item.origin.X,
        a[5] + item.origin.Y,
        a[14] - item.origin.Y,
        std::int32_t(std::uint32_t(a[13]) | (std::uint32_t(a[11]) << 24)),
        std::int32_t(metadata),
        adjustment,
        a[15],
        std::int32_t(cell | (std::uint32_t(intensity_source) << 20) | (std::uint32_t(palette_source) << 24)),
        auxiliary ? a[18] : 0};
    record.source = source;
    record.auxiliary = auxiliary;
    record.palette = converter >= 0 ? nullptr : colors;
    record.variant = std::int8_t(variant);
    record.kind = scene_kind_;
    scene_pending_.push_back(std::move(record));
}
void FrameCompiler::clear_scene_slots() {
    scene_slot_instances_.clear();
    scene_instances_dirty_ = true;
    scene_placing_ = false;
    scene_pending_.clear();
}
const MapSceneRecord& FrameCompiler::scene_peek(const BackgroundSlotAddress& at) const {
    static const MapSceneRecord none;
    const auto& instance = scene_slot_instances_.at(at.instance);
    if (at.slot >= instance.slots) throw std::out_of_range("Background slot");
    const auto& chunk = instance.chunks[at.slot / MapSceneChunkSlots];
    return chunk ? chunk->slots[at.slot % MapSceneChunkSlots] : none;
}
MapSceneRecord& FrameCompiler::scene_write(const BackgroundSlotAddress& at) {
    auto& instance = scene_slot_instances_.at(at.instance);
    if (at.slot >= instance.slots) throw std::out_of_range("Background slot");
    const auto index = at.slot / MapSceneChunkSlots;
    auto& chunk = instance.chunks[index];
    // A published chunk stays as the frames saw it; the next write works on a copy.
    if (!chunk)
        chunk = std::make_shared<MapSceneChunk>();
    else if (instance.shared[index]) {
        chunk = std::make_shared<MapSceneChunk>(*chunk);
        instance.shared[index] = 0;
    }
    return chunk->slots[at.slot % MapSceneChunkSlots];
}
DrawingStatus FrameCompiler::background_scene(void* p, const BackgroundSceneRequest& request) noexcept {
    std::uint64_t index = 0, instance_id = 0, first = 0, count = 0, limit = 0, slot = 0;
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        if (status == DrawingStatus::invalid_argument || status == DrawingStatus::backend_failure) {
            char detail[2048];
            describe_drawing_request(request, detail, sizeof(detail));
            const auto used = std::strlen(detail);
            const auto& scene = *static_cast<FrameCompiler*>(p);
            std::snprintf(detail + used, sizeof(detail) - used,
                          "index=%llu instance=%llu first=%llu count=%llu expected_limit=%llu slot=%llu instances=%zu "
                          "recording=%d pending=%zu epoch=%llu",
                          static_cast<unsigned long long>(index), static_cast<unsigned long long>(instance_id),
                          static_cast<unsigned long long>(first), static_cast<unsigned long long>(count),
                          static_cast<unsigned long long>(limit), static_cast<unsigned long long>(slot),
                          scene.scene_slot_instances_.size(), scene.scene_recording_, scene.scene_pending_.size(),
                          static_cast<unsigned long long>(scene.scene_epoch_));
            record_drawing_diagnostic(status, "background_scene", condition, detail, __FILE__, line, ordinal);
        }
        return status;
    };
    auto& s = *static_cast<FrameCompiler*>(p);
    try {
        if (!s.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&s))
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when !s.frame_ || request.target != reinterpret_cast<DrawingTargetHandle*>(&s)",
                            __LINE__);
        switch (request.operation) {
            case BackgroundSceneOperation::begin:
                if (s.scene_recording_)
                    return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
                return diagnose(s.scene_ && s.scene_world_ == request.world &&
                                        s.scene_quality_ == request.lighting_quality
                                    ? DrawingStatus::drawn
                                    : DrawingStatus::unavailable,
                                "backend_result", __LINE__);
            case BackgroundSceneOperation::reset:
                s.clear_scene_slots();
                s.scene_world_ = request.world;
                s.scene_quality_ = request.lighting_quality;
                s.clear_scene_lighting();
                s.scene_reset_ = true;
                ++s.scene_epoch_;
                return DrawingStatus::drawn;
            case BackgroundSceneOperation::item_begin:
                if (s.scene_recording_)
                    return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
                s.scene_recording_ = true;
                s.scene_placing_ = false;
                s.scene_request_ = request;
                s.scene_pending_.clear();
                s.scene_width_ = s.frame_->width;
                s.scene_height_ = s.frame_->height;
                s.frame_->width = s.frame_->height = 8192;
                return DrawingStatus::drawn;
            case BackgroundSceneOperation::item_end: {
                if (!s.scene_recording_)
                    return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
                s.frame_->width = s.scene_width_;
                s.frame_->height = s.scene_height_;
                s.scene_recording_ = false;
                if (!request.info_count || (!request.infos && request.info_capacity))
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when !request.info_count || (!request.infos && request.info_capacity)",
                                    __LINE__);
                *request.info_count = std::uint32_t(s.scene_pending_.size());
                if (s.scene_pending_.size() > request.info_capacity) {
                    s.scene_pending_.clear();
                    return DrawingStatus::unsupported;
                }
                for (std::size_t i = 0; i < s.scene_pending_.size(); ++i) {
                    const auto& record = s.scene_pending_[i];
                    const auto& w = record.words;
                    request.infos[i].kind = record.kind;
                    request.infos[i].state = std::uint32_t(record.variant);
                    request.infos[i].bounds = {w[3], w[4], w[2] & 0xFFFF, int(std::uint32_t(w[2]) >> 16)};
                }
                s.scene_placing_ = true;
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::instance: {
                if (s.scene_recording_ || request.instance != s.scene_slot_instances_.size() ||
                    request.instance_state >= RasterVariants)
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when s.scene_recording_ || request.instance != "
                                    "s.scene_slot_instances_.size() || request.instance_state >= RasterVariants",
                                    __LINE__);
                FrameCompiler::SceneInstance instance;
                instance.slots = request.instance_slots;
                instance.variant = request.instance_state;
                const auto chunks = (std::size_t(instance.slots) + MapSceneChunkSlots - 1) / MapSceneChunkSlots;
                instance.chunks.resize(chunks);
                instance.shared.assign(chunks, 0);
                s.scene_slot_instances_.push_back(std::move(instance));
                s.scene_instances_dirty_ = true;
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::place: {
                if (s.scene_recording_)
                    return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
                if ((request.slot_count && !request.slots) || (request.release_count && !request.releases) ||
                    (request.slot_count && (!s.scene_placing_ || request.slot_count != s.scene_pending_.size())))
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when (request.slot_count && !request.slots) || (request.release_count && "
                                    "!request.releases) || (request.slot_count && (!s.scene_placing_ || "
                                    "request.slot_count != s.scene_pending_.size()))",
                                    __LINE__);
                bool changed = false;
                for (std::uint32_t i = 0; i < request.slot_count; ++i) {
                    if (s.scene_peek(request.slots[i]) == s.scene_pending_[i]) continue;
                    s.scene_write(request.slots[i]) = std::move(s.scene_pending_[i]);
                    changed = true;
                }
                for (std::uint32_t i = 0; i < request.release_count; ++i) {
                    if (s.scene_peek(request.releases[i]).empty()) continue;
                    s.scene_write(request.releases[i]) = {};
                    changed = true;
                }
                s.scene_placing_ = false;
                s.scene_pending_.clear();
                if (request.changed) *request.changed = changed;
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::commit: {
                if (s.scene_recording_)
                    return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
                if ((request.dirty_count && !request.dirty) || (request.order_count && !request.orders))
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when (request.dirty_count && !request.dirty) || (request.order_count && "
                                    "!request.orders)",
                                    __LINE__);
                const bool lights = std::find(s.scene_light_dirty_.begin(), s.scene_light_dirty_.end(), char(1)) !=
                                    s.scene_light_dirty_.end();
                const bool shroud = std::find(s.scene_shroud_dirty_.begin(), s.scene_shroud_dirty_.end(), char(1)) !=
                                    s.scene_shroud_dirty_.end();
                if (!s.scene_ || s.scene_reset_ || request.dirty_count || request.order_count ||
                    s.scene_instances_dirty_ || lights || shroud || s.scene_alphas_dirty_ ||
                    s.scene_converters_dirty_) {
                    auto next = s.scene_reset_ || !s.scene_ ? std::make_shared<MapSceneData>()
                                                            : std::make_shared<MapSceneData>(*s.scene_);
                    next->epoch = s.scene_epoch_;
                    next->revision = ++s.scene_version_;
                    // Published chunks are immutable; the next write to one copies it first.
                    if (lights) {
                        auto chunks = std::make_shared<MapSceneLights>(*next->lights);
                        for (std::size_t i = 0; i < s.scene_light_chunks_.size(); ++i)
                            if (s.scene_light_dirty_[i]) {
                                chunks->set({0, int(i)}, s.scene_light_chunks_[i]);
                                s.scene_light_dirty_[i] = 0;
                            }
                        next->lights = std::move(chunks);
                        next->light_chunks = std::uint32_t(s.scene_light_chunks_.size());
                    }
                    if (shroud) {
                        auto chunks = std::make_shared<MapSceneShroud>(*next->shroud);
                        for (std::size_t i = 0; i < s.scene_shroud_chunks_.size(); ++i)
                            if (s.scene_shroud_dirty_[i]) {
                                chunks->set({0, int(i)}, s.scene_shroud_chunks_[i]);
                                s.scene_shroud_dirty_[i] = 0;
                            }
                        next->shroud = std::move(chunks);
                    }
                    if (s.scene_converters_dirty_) {
                        for (const auto& colors : s.scene_converters_)
                            if (colors) s.allocate_resource(colors);
                        next->converters =
                            std::make_shared<const std::vector<std::shared_ptr<MapBufferData>>>(s.scene_converters_);
                        s.scene_converters_dirty_ = false;
                    }
                    if (s.scene_alphas_dirty_) {
                        if (s.scene_alphas_)
                            for (const auto& alpha : *s.scene_alphas_)
                                s.allocate_resource(alpha.image.source);
                        next->alphas = s.scene_alphas_;
                        s.scene_alphas_dirty_ = false;
                    }
                    if (request.order_count) {
                        // Every chunk an update reaches is replaced by a new one: published chunks are immutable.
                        auto order = std::make_shared<MapSceneOrder>(*next->order);
                        for (std::uint32_t i = 0; i < request.order_count; ++i) {
                            const auto& update = request.orders[i];
                            index = i;
                            instance_id = update.instance;
                            first = update.first;
                            count = update.size;
                            limit = update.instance < s.scene_slot_instances_.size()
                                        ? s.scene_slot_instances_[update.instance].slots
                                        : s.scene_slot_instances_.size();
                            if (update.instance >= s.scene_slot_instances_.size())
                                return diagnose(DrawingStatus::invalid_argument,
                                                "rejected when update.instance >= s.scene_slot_instances_.size()",
                                                __LINE__);
                            auto& instance = s.scene_slot_instances_[update.instance];
                            if (update.first > update.size || update.size > instance.slots ||
                                (update.first < update.size && !update.slots))
                                return diagnose(DrawingStatus::invalid_argument,
                                                "rejected when update.first > update.size || update.size > "
                                                "instance.slots || (update.first < update.size && !update.slots)",
                                                __LINE__);
                            instance.order.resize((std::size_t(instance.slots) + MapSceneOrderChunkEntries - 1) /
                                                  MapSceneOrderChunkEntries);
                            for (std::uint32_t at = update.first; at < update.size;) {
                                const auto c = at / MapSceneOrderChunkEntries;
                                const auto end = std::min(update.size, (c + 1) * MapSceneOrderChunkEntries);
                                auto chunk = instance.order[c]
                                                 ? std::make_shared<MapSceneOrderChunk>(*instance.order[c])
                                                 : std::make_shared<MapSceneOrderChunk>();
                                for (; at < end; ++at) {
                                    slot = update.slots[at - update.first];
                                    if (slot >= instance.slots)
                                        return diagnose(DrawingStatus::invalid_argument,
                                                        "rejected when slot >= instance.slots", __LINE__);
                                    chunk->slots[at % MapSceneOrderChunkEntries] = slot;
                                }
                                instance.order[c] = chunk;
                                order->set({int(update.instance), int(c)}, std::move(chunk));
                            }
                            instance.drawn = update.size;
                        }
                        next->order = std::move(order);
                        s.scene_instances_dirty_ = true;
                    }
                    if (s.scene_instances_dirty_) {
                        auto table = std::make_shared<std::vector<MapSceneInstance>>();
                        table->reserve(s.scene_slot_instances_.size());
                        for (const auto& instance : s.scene_slot_instances_)
                            table->push_back({instance.slots, instance.variant, instance.drawn});
                        next->instances = std::move(table);
                        s.scene_instances_dirty_ = false;
                    }
                    if (request.dirty_count) {
                        // Publish the chunks the core changed; ranges are sorted and may share a boundary chunk.
                        auto slots = std::make_shared<MapSceneSlots>(*next->slots);
                        std::uint32_t last_instance = ~0u, last_chunk = ~0u;
                        for (std::uint32_t i = 0; i < request.dirty_count; ++i) {
                            const auto& range = request.dirty[i];
                            index = i;
                            instance_id = range.instance;
                            first = range.first;
                            count = range.count;
                            limit = range.instance < s.scene_slot_instances_.size()
                                        ? s.scene_slot_instances_[range.instance].slots
                                        : s.scene_slot_instances_.size();
                            if (range.instance >= s.scene_slot_instances_.size() || !range.count)
                                return diagnose(
                                    DrawingStatus::invalid_argument,
                                    "rejected when range.instance >= s.scene_slot_instances_.size() || !range.count",
                                    __LINE__);
                            auto& instance = s.scene_slot_instances_[range.instance];
                            if (std::uint64_t(range.first) + range.count > instance.slots)
                                return diagnose(
                                    DrawingStatus::invalid_argument,
                                    "rejected when std::uint64_t(range.first) + range.count > instance.slots",
                                    __LINE__);
                            for (auto c = range.first / MapSceneChunkSlots;
                                 c <= (range.first + range.count - 1) / MapSceneChunkSlots; ++c) {
                                if (range.instance == last_instance && c == last_chunk) continue;
                                last_instance = range.instance;
                                last_chunk = c;
                                if (!instance.chunks[c]) continue;
                                slots->set({int(range.instance), int(c)}, instance.chunks[c]);
                                instance.shared[c] = 1;
                            }
                        }
                        next->slots = std::move(slots);
                    }
                    s.scene_ = std::move(next);
                    s.scene_reset_ = false;
                }
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::abort:
                if (s.scene_recording_) {
                    s.scene_pending_.clear();
                    s.frame_->width = s.scene_width_;
                    s.frame_->height = s.scene_height_;
                    s.scene_recording_ = false;
                }
                s.scene_.reset();
                s.clear_scene_slots();
                s.clear_scene_lighting();
                return DrawingStatus::drawn;
            case BackgroundSceneOperation::lighting: {
                // Relight held items: a cell whose items cannot take the new light is
                // reported stale; the core records it again in this transaction.
                if (!request.lights && request.light_count)
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when !request.lights && request.light_count", __LINE__);
                std::uint32_t stale = 0;
                for (std::uint32_t i = 0; i < request.light_count; ++i) {
                    const auto& light = request.lights[i];
                    if (light.cell.X < 0 || light.cell.Y < 0 || light.cell.X >= 512 || light.cell.Y >= 512)
                        return diagnose(DrawingStatus::invalid_argument,
                                        "rejected when light.cell.X < 0 || light.cell.Y < 0 || light.cell.X >= 512 || "
                                        "light.cell.Y >= 512",
                                        __LINE__);
                    const auto cell = std::uint32_t(light.cell.Y) * 512 + std::uint32_t(light.cell.X);
                    const auto found = s.scene_light_.find(cell);
                    if (found != s.scene_light_.end() && std::memcmp(&found->second, &light, sizeof(light)) == 0)
                        continue;
                    if (found != s.scene_light_.end() && !s.scene_light_current(cell, light)) {
                        if (stale < request.stale_capacity && request.stale) request.stale[stale] = light.cell;
                        ++stale;
                        s.scene_light_use_.erase(cell);
                    }
                    s.set_scene_light(light);
                }
                if (request.stale_count) *request.stale_count = stale;
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::converter: {
                if (!request.converter)
                    return diagnose(DrawingStatus::invalid_argument, "rejected when !request.converter", __LINE__);
                const auto found = s.scene_converter_slots_.find(request.converter);
                if (found == s.scene_converter_slots_.end()) return DrawingStatus::drawn;
                const DrawingPaletteHandle* handle = nullptr;
                const auto status = converter_palette(&s, *request.converter, request.revision, handle);
                if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
                s.scene_converters_[std::size_t(found->second)] =
                    reinterpret_cast<const MapPaletteData*>(handle)->buffer;
                s.scene_converters_dirty_ = true;
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::shroud:
                if (!request.shroud && request.shroud_count)
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when !request.shroud && request.shroud_count", __LINE__);
                for (std::uint32_t i = 0; i < request.shroud_count; ++i) {
                    const auto& entry = request.shroud[i];
                    if (entry.cell.X < 0 || entry.cell.Y < 0 || entry.cell.X >= 512 || entry.cell.Y >= 512)
                        return diagnose(DrawingStatus::invalid_argument,
                                        "rejected when entry.cell.X < 0 || entry.cell.Y < 0 || entry.cell.X >= 512 || "
                                        "entry.cell.Y >= 512",
                                        __LINE__);
                    const auto cell = std::uint32_t(entry.cell.Y) * 512 + std::uint32_t(entry.cell.X);
                    const auto chunk = cell / MapLightChunkCells;
                    if (s.scene_shroud_chunks_.size() <= chunk) {
                        s.scene_shroud_chunks_.resize(chunk + 1);
                        s.scene_shroud_dirty_.resize(chunk + 1, 0);
                    }
                    // Published chunks are immutable; the first write of a commit copies.
                    if (!s.scene_shroud_dirty_[chunk]) {
                        s.scene_shroud_chunks_[chunk] =
                            s.scene_shroud_chunks_[chunk]
                                ? std::make_shared<MapSceneShroudChunk>(*s.scene_shroud_chunks_[chunk])
                                : std::make_shared<MapSceneShroudChunk>();
                        s.scene_shroud_dirty_[chunk] = 1;
                    }
                    s.scene_shroud_chunks_[chunk]->frames[cell % MapLightChunkCells] = entry.frame;
                }
                return DrawingStatus::drawn;
            case BackgroundSceneOperation::alpha: {
                if (!request.alphas && request.alpha_count)
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when !request.alphas && request.alpha_count", __LINE__);
                auto alphas = std::make_shared<std::vector<MapSceneAlpha>>();
                alphas->reserve(request.alpha_count);
                for (std::uint32_t i = 0; i < request.alpha_count; ++i) {
                    const auto& entry = request.alphas[i];
                    const MapLightingImage* image = nullptr;
                    const auto status = s.lighting_image(entry.image, 0, image);
                    if (status == DrawingStatus::skipped) continue;  // an empty frame draws nothing
                    if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
                    alphas->push_back(
                        {std::uint64_t(reinterpret_cast<std::uintptr_t>(entry.id)), entry.position, *image});
                }
                s.scene_alphas_ = std::move(alphas);
                s.scene_alphas_dirty_ = true;
                return DrawingStatus::drawn;
            }
            case BackgroundSceneOperation::draw: {
                if (!s.scene_) return DrawingStatus::unavailable;
                if (request.draw_count && (!request.visible || !request.window))
                    return diagnose(DrawingStatus::invalid_argument,
                                    "rejected when request.draw_count && (!request.visible || !request.window)",
                                    __LINE__);
                s.frame_->commands.push_back({MapCommandKind::scene, std::uint32_t(s.frame_->scenes.size())});
                s.frame_->scenes.push_back({s.scene_, request.origin, request.clip});
                auto& mark = s.frame_->scenes.back();
                for (std::uint32_t i = 0; i < request.draw_count; ++i) {
                    const auto& v = request.visible[i]; const auto& w = request.window[i];
                    mark.visible.push_back({v.instance, v.first, v.count});
                    mark.window.push_back({w.instance, w.first, w.count});
                }
                return DrawingStatus::drawn;
            }
        }
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when request.draw_count && (!request.visible || !request.window)", __LINE__);
    } catch (const std::exception& error) {
        std::snprintf(s.error_, sizeof(s.error_), "Background scene: %s", error.what());
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    } catch (...) {
        std::snprintf(s.error_, sizeof(s.error_), "Background scene recording failed");
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
}

namespace game {
DrawingStatus FrameCompiler::mask(void* pointer, const MaskDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "mask", condition, r, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);
    if (!self.frame_ || r.target != reinterpret_cast<DrawingTargetHandle*>(&self))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !self.frame_ || r.target != reinterpret_cast<DrawingTargetHandle*>(&self)",
                        __LINE__);

    try {
        TypeGpuPacket packet;
        auto status = prepare_type_mask(r, {self.frame_->width, self.frame_->height, 1}, packet);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        if (std::size_t(r.width) * r.height * 4 > 16 * 1024 * 1024) return DrawingStatus::unsupported;
        const int pitch = (r.width + 7) / 8;
        std::string bits(std::size_t(pitch) * r.height, '\0');
        for (int y = 0; y < r.height; ++y) {
            std::memcpy(bits.data() + std::size_t(y) * pitch, r.bits + std::size_t(y) * r.pitch, pitch);
            if (r.width % 8) bits[std::size_t(y + 1) * pitch - 1] &= char(0xFFu << (8 - r.width % 8));
        }
        const auto key = std::make_tuple(r.width, r.height, std::move(bits));
        auto found = self.masks_.find(key);
        if (found == self.masks_.end()) {
            // Bound memory even when custom fonts or editor text change repeatedly.
            if (self.masks_.size() >= 4096 ||
                self.mask_bytes_ + std::size_t(r.width) * r.height * 4 > 16 * 1024 * 1024) {
                self.masks_.clear();
                self.mask_bytes_ = 0;
            }
            const auto& canonical = std::get<2>(key);
            std::vector<std::uint32_t> pixels(std::size_t(r.width) * r.height);
            bool occupied = false;
            for (int y = 0; y < r.height; ++y)
                for (int x = 0; x < r.width; ++x)
                    if (canonical[std::size_t(y) * pitch + x / 8] & (0x80u >> (x % 8))) {
                        pixels[std::size_t(y) * r.width + x] = 0x10000u;
                        occupied = true;
                    }
            auto source = occupied ? self.buffer(pixels) : std::shared_ptr<MapBufferData>{};
            if (occupied) self.mask_bytes_ += pixels.size() * 4;
            found = self.masks_.emplace(key, std::move(source)).first;
        }
        if (!found->second) return DrawingStatus::skipped;
        auto& ink = self.fills_[r.color];
        if (!ink) ink = self.buffer({rgba565(r.color)});
        self.append_direct(packet.parameters, found->second, ink);
        return DrawingStatus::drawn;
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus FrameCompiler::shape_strip(void* pointer, const ShapeStripDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "shape_strip", condition, r, __FILE__, line, ordinal);
    };
    auto& self = *static_cast<FrameCompiler*>(pointer);
    if (!self.frame_ || r.shape.target != reinterpret_cast<DrawingTargetHandle*>(&self) || !r.shape.palette)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !self.frame_ || r.shape.target != reinterpret_cast<DrawingTargetHandle*>(&self) "
                        "|| !r.shape.palette",
                        __LINE__);

    try {
        if (!r.resource_revision) return DrawingStatus::unsupported;
        const auto key =
            std::make_tuple(r.resource_revision, r.shape.image, r.shape.frame, r.count, r.step.X, r.step.Y);
        auto found = self.strips_.find(key);
        if (found == self.strips_.end()) {
            // A strip is a composite of copies of one frame, so it is not a frame of a sequence; its
            // colour plane is composed from the frame's plane and published as a one-frame table.
            StripImage value;
            value.status = describe_type_shape_strip(r, value.meta);
            if (value.status != DrawingStatus::drawn) return value.status;
            TypeShapeMeta original;
            auto status = describe_type_shape(r.shape, original);
            if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
            if (r.shape.image) r.shape.image->Load();
            auto& sequence = self.sequence(r.shape.image);
            std::uint32_t frame_entry = 0;
            status = self.sequence_entry(sequence, r.shape.image, r.shape.frame, frame_entry, false);
            if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
            const auto& frame_row = sequence.data->table->entries[std::size_t(r.shape.frame)];
            const auto* plane = sequence.data->bytes.get() + frame_row[0];
            const int pitch = frame_row[1], width = value.meta.body.Width, height = value.meta.body.Height;
            std::vector<std::uint8_t> composite(std::size_t(width) * height, 0);
            for (int i = 0; i < r.count; ++i)
                for (int y = 0; y < original.body.Height; ++y)
                    for (int x = 0; x < original.body.Width; ++x) {
                        // Last covered copy wins; transparent pixels never erase prior copies.
                        const auto pixel = plane[std::size_t(y) * pitch + x];
                        if (pixel) composite[std::size_t(y + i * r.step.Y) * width + x + i * r.step.X] = pixel;
                    }
            if (self.strips_.size() >= 128 || self.strip_bytes_ + composite.size() > 32 * 1024 * 1024) {
                self.strips_.clear();
                self.strip_bytes_ = 0;
            }
            auto table = std::make_shared<MapSequenceTable>();
            table->entries.push_back({0, width, height, -1});
            table->allocation = self.sequence_table_pool_.allocate(1);
            value.data = self.byte_buffer(composite);
            self.allocate_resource(value.data);
            value.data->table = table;
            value.entry = MapSequenceFrameBase + table->allocation->first;
            self.strip_bytes_ += composite.size();
            found = self.strips_.emplace(key, std::move(value)).first;
        }
        const auto& palette = *reinterpret_cast<const MapPaletteData*>(r.shape.palette);
        TypeGpuPacket packet;
        const auto status = prepare_type_shape_parameters(
            r.shape, found->second.meta, {self.frame_->width, self.frame_->height, palette.shades, 0, 0x8000, true},
            packet);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        // The composite goes to the renderer with each frame that draws it.
        self.retain_resource(found->second.data);
        self.append_plane(packet.parameters, found->second.entry, palette.buffer);
        return DrawingStatus::drawn;
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
}
