#include "frame_plan.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace render {
int append_lighting_bins(const PixelParameters* packets, int first, int count, int width, int height,
                         std::vector<std::int32_t>& bins) {
    if (!packets || first < 0 || count < 0 || count > INT32_MAX - first || width <= 0 || height <= 0 || width > 8192 ||
        height > 8192)
        throw std::invalid_argument("lighting batch dimensions");
    const int columns = (width + 32 - 1) / 32;
    const int rows = (height + 32 - 1) / 32, tiles = columns * rows;
    const auto start = bins.size();
    if (start + std::size_t(tiles) * 2 > INT32_MAX) throw std::length_error("lighting bins");
    bins.resize(start + tiles * 2, 0);
    const auto visit = [&](int index, auto&& action) {
        const auto& p = packets[index];
        if (p[0] != width || p[1] != height || p[16] != 0) throw std::invalid_argument("lighting batch target");
        const auto l = std::max({std::int64_t(0), std::int64_t(p[4]), std::int64_t(p[6])});
        const auto t = std::max({std::int64_t(0), std::int64_t(p[5]), std::int64_t(p[7])});
        const auto r = std::min({std::int64_t(width), std::int64_t(p[4]) + p[2], std::int64_t(p[6]) + p[8]});
        const auto b = std::min({std::int64_t(height), std::int64_t(p[5]) + p[3], std::int64_t(p[7]) + p[9]});
        if (l >= r || t >= b) return;
        for (int y = int(t) / 32; y <= int(b - 1) / 32; ++y)
            for (int x = int(l) / 32; x <= int(r - 1) / 32; ++x)
                action(y * columns + x);
    };
    for (int n = 0; n < count; ++n)
        visit(first + n, [&](int tile) { ++bins[start + tile * 2 + 1]; });
    std::size_t next = bins.size();
    std::vector<int> cursors(tiles);
    for (int i = 0; i < tiles; ++i) {
        if (next + std::size_t(bins[start + i * 2 + 1]) > INT32_MAX)
            throw std::length_error("lighting packet references");
        cursors[i] = bins[start + i * 2] = int(next);
        next += bins[start + i * 2 + 1];
    }
    bins.resize(next);
    // Ascending source indices preserve every overlapping write, including
    // successive integer AlphaShape multiplications. No parallel pixel races.
    for (int n = 0; n < count; ++n)
        visit(first + n, [&](int tile) { bins[cursors[tile]++] = first + n; });
    return int(start);
}

void FrameCompiler::compile(MapFrameData& frame) {
            std::size_t headers = 0;
            for (const auto& run : frame.commands)
                if (run.kind == MapCommandKind::compute && run.count > 1) ++headers;
            frame.compute_resources.resize((frame.compute.size() + headers) * 2, 0);
            for (auto& run : frame.commands) {
                if (run.kind == MapCommandKind::compute && run.count > 1) {
                    if (run.variant == -3) {
                        PixelParameters header{frame.width, frame.height, int(run.count), 1, int(run.first)};
                        header[19] = 11;
                        run.first = std::uint32_t(frame.compute.size());
                        frame.compute.push_back(header);
                        run.count = 1;
                    } else {
                        const auto batch = append_pixel_run(frame.compute, run.first, run.first + run.count,
                                                            frame.compute_resources, scratch_);
                        run.first = std::uint32_t(batch.header);
                        run.count = 1;
                    }
                } else if (run.kind == MapCommandKind::lighting)
                    run.bin_offset = append_lighting_bins(frame.compute.data(), int(run.first), int(run.count),
                                                          frame.width, frame.height, frame.light_bins);
            }
}
}

namespace render {
Status FrameCompiler::describe(FramePayload& data, Frame& frame) noexcept {
    frame = {};
    try {
        data.resource_views.clear(); data.scene_views.clear(); data.effect_views.clear();
        for (const auto& resource : data.resources) {
            const auto version = resource->allocation ? resource->allocation->version : 0;
            data.resource_views.push_back({resource->id, version, resource->voxel ? 1u : resource->voxel_model ? 2u : 0u,
                std::uint32_t(resource->image_width), std::uint32_t(resource->image_height),
                resource->bytes.get(), std::uint32_t(resource->byte_count), resource.get()});
        }
        for (const auto& mark : data.scenes) {
            if (mark.visible.size() != mark.window.size()) return Status::invalid_argument;
            data.scene_views.push_back({mark.scene.get(), mark.origin, mark.clip, mark.visible.data(), mark.window.data(),
                std::uint32_t(mark.visible.size())});
        }
        for (const auto& mark : data.screen_effect_marks) data.effect_views.push_back(mark.request);
        frame.reference=data.reference.get();
        frame.serial=data.serial; frame.generation=data.generation; frame.width=data.width; frame.height=data.height;
        frame.camera_x=data.view_origin.X; frame.camera_y=data.view_origin.Y;
        frame.resources=data.resource_views.data(); frame.resource_count=std::uint32_t(data.resource_views.size());
        frame.sequences=data.sequences.get(); frame.scene=data.scene_views.data(); frame.scene_count=std::uint32_t(data.scene_views.size());
        frame.instances=data.instances.data(); frame.instance_count=std::uint32_t(data.instances.size());
        frame.compute_words=data.compute.empty()?nullptr:data.compute.front().data(); frame.compute_count=std::uint32_t(data.compute.size());
        frame.compute_resources=data.compute_resources.data(); frame.compute_resource_count=std::uint32_t(data.compute_resources.size());
        frame.light_bins=data.light_bins.data(); frame.light_bin_count=std::uint32_t(data.light_bins.size());
        frame.field_words=data.field_words.data(); frame.field_word_count=std::uint32_t(data.field_words.size());
        frame.light_fields=data.light_fields.data(); frame.light_field_count=std::uint32_t(data.light_fields.size());
        frame.passes=data.commands.data(); frame.command_count=std::uint32_t(data.commands.size());
        frame.screen_effect=data.screen_effect; frame.effects=data.effect_views.data(); frame.effect_count=std::uint32_t(data.effect_views.size());
        for (const auto& effect:data.effect_views) if (effect.operation==ScreenEffectOperation::capture_end) frame.capture=true;
        return Status::ok;
    } catch (...) {return Status::backend_failure;}
}
namespace {
template<class T> void copy_view(std::vector<T>& output,const T* input,std::uint32_t count) {
    if (count && !input) throw std::invalid_argument("null frame array");
    if (count) output.assign(input,input+count); else output.clear();
}
// A view does not transfer ownership; the frame's owner remains alive through return.
// Shared scene chunks copied by the renderer retain their actual original owners.
template<class T> std::shared_ptr<const T> borrow(const T* value) {
    return value?std::shared_ptr<const T>(value,[](const T*){}):std::shared_ptr<const T>{};
}
}
Status FrameCompiler::import(const Frame& frame,MapFrameData& data) noexcept {
    try {
        if (frame.width<=0 || frame.height<=0 || frame.width>8192 || frame.height>8192 ||
            (frame.resource_count && !frame.resources) || (frame.scene_count && !frame.scene) ||
            (frame.instance_count && !frame.instances) || (frame.compute_count && !frame.compute_words) ||
            (frame.effect_count && !frame.effects) || (frame.command_count && !frame.passes) ||
            (frame.compute_resource_count && !frame.compute_resources) || (frame.light_bin_count && !frame.light_bins) ||
            (frame.field_word_count && !frame.field_words) || (frame.light_field_count && !frame.light_fields))
            return Status::invalid_argument;
        data.serial=frame.serial; data.generation=frame.generation; data.width=frame.width; data.height=frame.height;
        data.view_origin={frame.camera_x,frame.camera_y}; data.screen_effect=frame.screen_effect;
        data.resources.clear(); data.scenes.clear(); data.screen_effect_marks.clear();
        for (std::uint32_t i=0;i<frame.resource_count;++i) {
            const auto& resource=frame.resources[i];
            const auto* snapshot=resource.snapshot;
            if (!snapshot || !snapshot->allocation || snapshot->id!=resource.id ||
                snapshot->allocation->version!=resource.version || resource.size!=snapshot->byte_count ||
                resource.data!=snapshot->bytes.get() || resource.width!=std::uint32_t(snapshot->image_width) ||
                resource.height!=std::uint32_t(snapshot->image_height) ||
                resource.kind!=(snapshot->voxel ? 1u : snapshot->voxel_model ? 2u : 0u))
                return Status::invalid_argument;
            data.resources.emplace_back(const_cast<MapBufferData*>(resource.snapshot),[](MapBufferData*){});
        }
        data.sequences=borrow(frame.sequences);
        for (std::uint32_t i=0;i<frame.scene_count;++i) {
            const auto& scene=frame.scene[i];
            if (!scene.snapshot || (scene.range_count && (!scene.visible || !scene.window)))
                return Status::invalid_argument;
            MapSceneMark mark{borrow(scene.snapshot),scene.origin,scene.clip};
            copy_view(mark.visible,scene.visible,scene.range_count); copy_view(mark.window,scene.window,scene.range_count);
            data.scenes.push_back(std::move(mark));
        }
        data.instances.resize(frame.instance_count); data.compute.resize(frame.compute_count);
        static_assert(sizeof(RasterInstance)==16*sizeof(std::int32_t));
        static_assert(sizeof(PixelParameters)==20*sizeof(std::int32_t));
        if (frame.instance_count) std::memcpy(data.instances.data(),frame.instances,data.instances.size()*sizeof(RasterInstance));
        if (frame.compute_count) std::memcpy(data.compute.data(),frame.compute_words,data.compute.size()*sizeof(PixelParameters));
        copy_view(data.compute_resources,frame.compute_resources,frame.compute_resource_count);
        copy_view(data.light_bins,frame.light_bins,frame.light_bin_count);
        copy_view(data.field_words,frame.field_words,frame.field_word_count);
        copy_view(data.light_fields,frame.light_fields,frame.light_field_count);
        copy_view(data.commands,frame.passes,frame.command_count);
        for (std::uint32_t i=0;i<frame.effect_count;++i) data.screen_effect_marks.push_back({frame.effects[i]});
        for (const auto& pass:data.commands) {
            std::uint32_t count=0;
            switch(pass.kind) {
                case PassKind::raster:count=frame.instance_count;break;
                case PassKind::compute:case PassKind::lighting:count=frame.compute_count;break;
                case PassKind::light_field:count=frame.light_field_count;break;
                case PassKind::scene:count=frame.scene_count;break;
                case PassKind::effect:count=frame.effect_count;break;
                default:return Status::invalid_argument;
            }
            if (pass.first>count || pass.count>count-pass.first) return Status::invalid_argument;
        }
        return Status::ok;
    } catch (...) {return Status::backend_failure;}
}
}
