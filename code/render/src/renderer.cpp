#include "renderer_impl.hpp"
#include "frame_storage.hpp"
#include "scene_page_cache.hpp"
#include "map_shader_resources.hpp"
#include <functional>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
namespace render {
namespace {
struct DeviceWork { std::size_t buffer_updates=0, update_bytes=0, creations=0, uniform_sets=0, draws=0, dispatches=0; } work;
void count_update(std::size_t size) { ++work.buffer_updates; work.update_bytes+=size; }
// Device callbacks consume upload/push data before returning. Keep the view
// borrowed here; the host performs any copy its native API requires.
ByteView bytes(const void* data,std::size_t size) {
    return {static_cast<const std::uint8_t*>(data), size};
}
Handle storage_set(DeviceOperations* rd,Handle shader,std::initializer_list<Handle> buffers) {
    std::vector<Binding> bindings; std::uint32_t index=0;
    for(Handle h:buffers) bindings.push_back({BindingKind::buffer,h,{},index++});
    ++work.uniform_sets; return rd->binding_set(shader,bindings);
}
struct BindingInput { BindingKind type; Handle rid; Handle texture{}; int at=-1; };
Handle binding_set(DeviceOperations* rd,Handle shader,std::initializer_list<BindingInput> values) {
    std::vector<Binding> bindings; std::uint32_t index=0;
    for(const auto& b:values) {
        if(b.at>=0) index=b.at;
        bindings.push_back({b.type,b.rid,b.texture,index++});
    }
    ++work.uniform_sets; return rd->binding_set(shader,bindings);
}
constexpr auto StorageBuffer=BindingKind::buffer, ImageUniform=BindingKind::image, SampledTexture=BindingKind::sampled_texture;
constexpr float FarDepth=65535.0f/65536.0f;
Color raster_blend_constant(int variant) {
    switch(variant%RasterBlends) {
        case 1: case 3: return {0.5f,0.5f,0.5f,1};
        case 2: return {0.25f,0.25f,0.25f,1};
        case 4: return {0.6796875f,0.71484375f,0.6796875f,1};
        default: return {1,1,1,1};
    }
}
constexpr int PresentClear=1, PresentSnapshot=2, PresentEffect=3, PresentClearDepth=4, PresentKeepComposite=5,
    PresentRestoreComposite=6, PresentIonBlast=7;
}
void RendererImpl::free_uniforms() {
    if (!rd_) return;
    for (Handle* rid : {&light_field_uniform_, &present_uniform_, &voxel_uniform_, &resident_uniform_, &page_uniform_,
                     &resolve_uniform_, &light_page_uniform_, &direct_uniform_, &direct_compute_uniform_,
                     &resource_upload_uniform_, &direct_lighting_uniform_, &lit_tile_uniform_, &lit_copy_uniform_}) {
        if (rid->is_valid()) rd_->free_rid(*rid);
        *rid = {};
    }
}
void RendererImpl::free_states() {
    if (!rd_) return;
    free_uniforms();
    free_scene();
    free_pages();
    // Framebuffers before the textures they name.
    for (Handle* rid : {&lit_framebuffer_,    &lit_ring_,         &raster_framebuffer_, &colors_,
                     &raster_depth_,       &lights_,
                     &screen_snapshot_,    &kept_composite_,   &frame_depth_,      &scene_shroud_,
                     &light_field_words_,  &voxel_words_,      &voxel_projection_, &voxel_depth_,
                     &light_ring_,         &light_tables_,     &resource_pages_,   &resource_metadata_,
                     &direct_instances_,   &direct_compute_,   &direct_schedule_,  &resource_upload_buffer_,
                     &voxel_models_,       &direct_light_bins_}) {
        if (rid->is_valid()) rd_->free_rid(*rid);
        *rid = {};
    }
    light_tiles_.clear();
    light_ring_columns_ = light_ring_rows_ = 0;
    lit_tiles_.clear();
    lit_columns_ = lit_rows_ = 0;
    {
        const char* ring = std::getenv("RA2_MAP_LIT_RING");
        lit_enabled_ = !(ring && ring[0] == '0');
    }
    light_table_capacity_ = 0;
    light_all_dirty_ = true;
    width_ = height_ = 0;
    light_field_capacity_ = 0;
    voxel_word_capacity_ = voxel_projection_capacity_ = voxel_depth_capacity_ = 0;
    voxel_jobs_.clear();
    voxel_model_capacity_ = 0;
    voxel_model_versions_.clear();
    voxel_model_uploads_.clear();
    screen_capture_id_ = 0;
    resource_uploads_.clear();
    resource_upload_words_.clear();
    resource_upload_capacity_ = 0;
    scene_resource_uploads_.clear();
    resource_versions_.clear();
    resource_metadata_words_.clear();
    resource_layers_ = 0;
    sequence_revision_ = 0;
    resource_metadata_capacity_ = direct_instance_capacity_ = direct_compute_capacity_ = direct_schedule_capacity_ =
        direct_light_bin_capacity_ = 0;
}
void RendererImpl::clear() {
    free_states();
    if (rd_) {
        for (Handle& pipeline : direct_pipelines_) {
            if (pipeline.is_valid()) rd_->free_rid(pipeline);
            pipeline = {};
        }
        for (Handle& pipeline : page_pipelines_) {
            if (pipeline.is_valid()) rd_->free_rid(pipeline);
            pipeline = {};
        }
        for (Handle* rid : {&resolve_pipeline_, &resolve_shader_, &page_shader_, &voxel_pipeline_, &voxel_shader_,
                         &lit_tile_pipeline_, &lit_tile_shader_, &lit_copy_pipeline_, &lit_copy_shader_,
                         &light_field_pipeline_, &light_field_shader_, &light_page_pipeline_, &light_page_shader_,
                         &present_pipeline_, &present_shader_, &depth_sampler_, &direct_shader_,
                         &direct_compute_pipeline_, &direct_compute_shader_, &resource_upload_pipeline_,
                         &resource_upload_shader_, &direct_lighting_pipeline_, &direct_lighting_shader_}) {
            if (rid->is_valid()) rd_->free_rid(*rid);
            *rid = {};
        }
    }
    rd_ = nullptr;
}
bool RendererImpl::create_pipeline(const ShaderProgram& program, Handle& shader, Handle& pipeline) {
    shader = rd_->shader_create_from_spirv(program);
    if (shader.is_valid()) pipeline = rd_->compute_pipeline_create(shader);
    return pipeline.is_valid();
}
bool RendererImpl::create_render_shader(const ShaderProgram& program, Handle& shader) {
    shader = rd_->shader_create_from_spirv(program);
    return shader.is_valid();
}
bool RendererImpl::create_raster_pipelines() {
    if (!direct_shader_.is_valid() && !create_render_shader(map_sprite_program, direct_shader_)) return false;
    const auto format = rd_->framebuffer_get_format(raster_framebuffer_);
    static_assert(sizeof(direct_pipelines_) / sizeof(direct_pipelines_[0]) == RasterVariants);
    for (int variant = 0; variant < RasterVariants; ++variant) {
        if (direct_pipelines_[variant].is_valid()) continue;
        const auto depth_state = static_cast<RasterDepth>(variant / RasterBlends);
        const int blend_mode = variant % RasterBlends;
        DepthSpec depth;
        const bool write =
            depth_state == RasterDepth::less_write || depth_state == RasterDepth::less_equal_write;
        depth->set_enable_depth_test(depth_state != RasterDepth::none);
        depth->set_enable_depth_write(write);
        depth->set_depth_compare_operator(depth_state == RasterDepth::less_write ||
                                                  depth_state == RasterDepth::less
                                              ? DeviceOperations::COMPARE_OP_LESS
                                              : DeviceOperations::COMPARE_OP_LESS_OR_EQUAL);
        // color = source + constant * destination: the shader outputs zero for
        // a shadow and the original masked source term for translucency;
        // raster_blend_constant() supplies the constant; alpha is kept.
        AttachmentSpec color;
        if (blend_mode != 0) {
            color->set_enable_blend(true);
            color->set_src_color_blend_factor(DeviceOperations::BLEND_FACTOR_ONE);
            color->set_dst_color_blend_factor(DeviceOperations::BLEND_FACTOR_CONSTANT_COLOR);
            color->set_color_blend_op(DeviceOperations::BLEND_OP_ADD);
            color->set_src_alpha_blend_factor(DeviceOperations::BLEND_FACTOR_ZERO);
            color->set_dst_alpha_blend_factor(DeviceOperations::BLEND_FACTOR_ONE);
            color->set_alpha_blend_op(DeviceOperations::BLEND_OP_ADD);
        }
        BlendSpec blend;
        std::vector<AttachmentSpec> attachments;
        attachments.push_back(color);
        blend->set_attachments(attachments);
        if (!direct_pipelines_[variant].is_valid())
            direct_pipelines_[variant] = rd_->render_pipeline_create(
                direct_shader_, format, depth, blend, DeviceOperations::DYNAMIC_STATE_BLEND_CONSTANTS);
        if (!direct_pipelines_[variant].is_valid()) return false;
    }
    return true;
}
bool RendererImpl::prepare_device(const MapFrameData& frame) {
    if (!rd_) {
            error_ = "DeviceOperations unavailable; select Forward+ or Mobile";
        return false;
    }
    if (!light_field_pipeline_.is_valid() &&
        !create_pipeline(map_light_field_program, light_field_shader_, light_field_pipeline_))
        return false;
    if (!light_page_pipeline_.is_valid() &&
        !create_pipeline(map_light_page_program, light_page_shader_, light_page_pipeline_))
        return false;
    if (!present_pipeline_.is_valid() &&
        !create_pipeline(map_present_program, present_shader_, present_pipeline_))
        return false;
    if (generation_ == frame.generation && width_ == frame.width && height_ == frame.height) return true;
    free_states();
    const auto size = std::uint32_t(frame.width) * frame.height * 4;
    for (Handle* rid : {&lights_, &screen_snapshot_, &kept_composite_}) {
        *rid = rd_->storage_buffer_create(size);
        if (!rid->is_valid()) return false;
    }
    frame_depth_ = rd_->storage_buffer_create(64);
    if (!frame_depth_.is_valid()) return false;
    scene_shroud_ = rd_->storage_buffer_create(512 * 512 * 4);
    ++work.creations;
    if (!scene_shroud_.is_valid()) return false;
    scene_shroud_sources_.reset();
    scene_shroud_clear_ = true;
    const auto texture = [&](DeviceOperations::DataFormat data, std::uint64_t usage) {
        TextureSpec format;
        format->set_width(frame.width);
        format->set_height(frame.height);
        format->set_format(data);
        format->set_usage_bits(usage);
        return rd_->texture_create(format);
    };
    using RD = DeviceOperations;
    // Color is a storage image for compute paths and the raster attachment;
    // scene Z is a D32F attachment that compute paths sample.
    // Color is also the published texture (sampled by its display).
    const std::uint64_t color_usage = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
                                      RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
    const std::uint64_t depth_usage = RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT |
                                      RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
    if (!rd_->texture_is_format_supported_for_usage(RD::DATA_FORMAT_R8G8B8A8_UNORM, color_usage) ||
        !rd_->texture_is_format_supported_for_usage(RD::DATA_FORMAT_D32_SFLOAT, depth_usage)) {
            error_ = "DeviceOperations lacks RGBA8 storage/color or sampled D32F depth attachments";
        return false;
    }
    colors_ = texture(RD::DATA_FORMAT_R8G8B8A8_UNORM, color_usage);
    raster_depth_ = texture(RD::DATA_FORMAT_D32_SFLOAT, depth_usage);
    if (!colors_.is_valid() || !raster_depth_.is_valid()) return false;
    std::vector<Handle> attachments;
    attachments.push_back(colors_);
    attachments.push_back(raster_depth_);
    raster_framebuffer_ = rd_->framebuffer_create(attachments);
    if (!raster_framebuffer_.is_valid() || !create_raster_pipelines()) return false;
    if (!depth_sampler_.is_valid()) {
        depth_sampler_ = rd_->sampler_create();
        if (!depth_sampler_.is_valid()) return false;
    }
    generation_ = frame.generation;
    width_ = frame.width;
    height_ = frame.height;
    return create_light_ring();
}
// Grows the buffer when needed (uniform sets naming it are rebuilt) and
// uploads size bytes; an empty upload still leaves a bindable buffer.
bool RendererImpl::upload(Handle& buffer, std::size_t& capacity, const void* data, std::size_t size) {
    const auto needed = std::max<std::size_t>(size, 16);
    if (!buffer.is_valid() || capacity < needed) {
        free_uniforms();
        if (buffer.is_valid()) rd_->free_rid(buffer);
        capacity = std::max(needed, capacity * 2);
        if (capacity > std::size_t(INT32_MAX)) throw std::length_error("map renderer buffer");
        buffer = rd_->storage_buffer_create(static_cast<std::uint32_t>(capacity));
        ++work.creations;
        if (!buffer.is_valid()) return false;
    }
    if (size && data) count_update(size);
    return !size || !data || rd_->buffer_update(buffer, 0, static_cast<std::uint32_t>(size), bytes(data, size)) == OK;
}
bool RendererImpl::generate_voxels() {
    if (voxel_jobs_.empty()) return true;
    if (!voxel_pipeline_.is_valid() && !create_pipeline(map_voxel_program, voxel_shader_, voxel_pipeline_))
        return false;
    std::vector<std::int32_t> words;
    std::vector<std::uint32_t> bases;
    // Scratch is per image so independent images share a dispatch. An image
    // reading another image generated this frame (mask or composite layer)
    // runs at a later dependency level; jobs are in reference order.
    std::size_t projection_words = 4, depth_words = 4;
    std::unordered_map<const MapBufferData*, unsigned> levels;
    std::vector<unsigned> job_levels;
    unsigned level_count = 0;
    for (const auto* output : voxel_jobs_) {
        const auto& image = *output->voxel;
        const auto base = std::uint32_t(words.size());
        bases.push_back(base);
        words.resize(words.size() + 32);
        unsigned level = 0;
        const auto after = [&](const MapBufferData* input) {
            if (const auto found = levels.find(input); found != levels.end())
                level = std::max(level, found->second + 1);
        };
        if (image.mask) after(image.mask.get());
        for (const auto& layer : image.layers)
            after(layer.image.get());
        levels[output] = level;
        job_levels.push_back(level);
        level_count = std::max(level_count, level + 1);
        words[base] = int(image.kind);
        words[base + 1] = int(output->allocation->first);
        words[base + 2] = image.width;
        words[base + 3] = image.height;
        words[base + 4] = image.left;
        words[base + 5] = image.top;
        words[base + 6] = image.buffered;
        words[base + 7] = int(image.sections.size());
        words[base + 12] = image.mask ? int(image.mask->allocation->first) : -1;
        if (image.mask) {
            words[base + 13] = image.mask->voxel->width;
            words[base + 14] = image.mask->voxel->height;
        }
        words[base + 15] = int(image.layers.size());
        std::copy(image.shadow.begin(), image.shadow.end(), words.begin() + base + 16);
        words[base + 26] = int(words.size());
        unsigned samples = 0, columns = 0;
        for (std::size_t i = 0; i < image.sections.size(); ++i) {
            const auto& source = *image.sources[i];
            const auto* data = reinterpret_cast<const std::uint32_t*>(source.bytes.get());
            const auto first = words.size();
            words.resize(first + 24);
            std::copy(image.sections[i].begin(), image.sections[i].end(), words.begin() + first);
            words[first + 16] = int(source.allocation->first);
            words[first + 17] = int(samples);
            words[first + 18] = int(columns);
            words[first + 19] = int(data[0]);
            words[first + 20] = int(data[1]);
            samples += data[1];
            columns += data[0];
        }
        if (image.kind == VoxelKind::raster) {
            words[base + 10] = int(image.palette->allocation->first);
            words[base + 11] = int(image.palette->word_count() / 256);
            words[base + 27] = int(words.size());
            words.insert(words.end(), image.lighting.begin(), image.lighting.end());
        }
        if (image.kind == VoxelKind::shadow) {
            columns = unsigned(image.columns->word_count());
            samples = columns * 2;
            words[base + 29] = int(image.columns->allocation->first);
        }
        words[base + 8] = int(samples);
        words[base + 9] = int(columns);
        words[base + 28] = int(words.size());
        for (const auto& layer : image.layers) {
            const auto& child = *layer.image->voxel;
            const std::int32_t data[12]{int(layer.image->allocation->first),
                                        child.width,
                                        child.height,
                                        layer.position.X,
                                        layer.position.Y,
                                        layer.clip.X,
                                        layer.clip.Y,
                                        layer.clip.Width,
                                        layer.clip.Height,
                                        0,
                                        0,
                                        0};
            words.insert(words.end(), std::begin(data), std::end(data));
        }
        words[base + 30] = int(projection_words);
        projection_words += std::size_t(samples) * 2;
        words[base + 31] = int(depth_words);
        if (image.buffered) depth_words += std::size_t(image.height) * 256;
    }
    // Stages per level: 0 clears (composites run whole here), 1 projects,
    // 2 resolves. Each list is the bases of its images; the dispatch is as
    // wide as its largest image.
    struct Stage {
        std::uint32_t list = 0, count = 0, groups = 1;
    };
    std::vector<Stage> stages(std::size_t(level_count) * 3);
    for (unsigned level = 0; level < level_count; ++level)
        for (unsigned mode = 0; mode < 3; ++mode) {
            auto& stage = stages[level * 3 + mode];
            stage.list = std::uint32_t(words.size());
            for (std::size_t j = 0; j < bases.size(); ++j) {
                if (job_levels[j] != level) continue;
                const auto base = bases[j];
                const auto kind = words[base], width = words[base + 2], height = words[base + 3];
                const bool buffered = words[base + 6] != 0;
                if (kind == 2 && mode != 0) continue;
                const unsigned count = kind == 2   ? unsigned(width * height)
                                       : mode == 0 ? unsigned(buffered ? height * 256 : width * height)
                                       : mode == 1 ? unsigned(words[base + 9])
                                                   : unsigned(buffered ? height : width * height);
                words.push_back(int(base));
                ++stage.count;
                stage.groups = std::max(stage.groups, std::max(1u, (count + 63) / 64));
            }
        }
    if (!upload(voxel_words_, voxel_word_capacity_, words.data(), words.size() * 4) ||
        !upload(voxel_projection_, voxel_projection_capacity_, nullptr, projection_words * 4) ||
        !upload(voxel_depth_, voxel_depth_capacity_, nullptr, depth_words * 4))
        return false;
    if (!voxel_uniform_.is_valid())
        voxel_uniform_ = binding_set(rd_, voxel_shader_,
                                     {{StorageBuffer, voxel_words_},
                                      {StorageBuffer, voxel_models_},
                                      {StorageBuffer, voxel_projection_},
                                      {StorageBuffer, voxel_depth_},
                                      {ImageUniform, resource_pages_}});
    if (!voxel_uniform_.is_valid()) return false;
    const auto list = rd_->compute_list_begin();
    rd_->compute_list_bind_compute_pipeline(list, voxel_pipeline_);
    rd_->compute_list_bind_uniform_set(list, voxel_uniform_, 0);
    std::size_t dispatches = 0;
    for (std::size_t i = 0; i < stages.size(); ++i) {
        const auto& stage = stages[i];
        if (!stage.count) continue;
        const std::uint32_t push[4]{stage.list, std::uint32_t(i % 3), 0, 0};
        rd_->compute_list_set_push_constant(list, bytes(push, sizeof(push)), sizeof(push));
        rd_->compute_list_dispatch(list, stage.groups, stage.count, 1);
        rd_->compute_list_add_barrier(list);
        ++dispatches;
        ++work.dispatches;
    }
    rd_->compute_list_end();
    if (std::getenv("RA2_VERIFY_VOXELS")) {
        std::unordered_map<std::uint32_t, Bytes> pages;
        std::size_t pixels = 0, images = 0;
        for (const auto* output : voxel_jobs_)
            if (!output->voxel_reference.empty()) {
                const auto& a = *output->allocation;
                if (output->voxel_reference.size() != a.count)
                    throw std::runtime_error("Voxel reference extent mismatch");
                for (std::uint32_t index = 0; index < a.count; ++index) {
                    const auto address = a.first + index, layer = address / DrawResourcePool::PageWords;
                    auto [entry, inserted] = pages.try_emplace(layer);
                    if (inserted) entry->second = rd_->texture_get_data(resource_pages_, layer);
                    const auto* words = reinterpret_cast<const std::uint32_t*>(entry->second.data());
                    const auto actual = words[address % DrawResourcePool::PageWords],
                               expected = output->voxel_reference[index];
                    if (actual != expected) {
                        char message[192];
                        std::snprintf(message, sizeof(message),
                                      "Voxel GPU mismatch kind=%u id=%llu pixel=%u actual=0x%08X expected=0x%08X",
                                      unsigned(output->voxel->kind), static_cast<unsigned long long>(output->id), index,
                                      actual, expected);
                        throw std::runtime_error(message);
                    }
                }
                pixels += a.count;
                ++images;
            }
        std::fprintf(stderr, "VOXEL_REFERENCE images=%zu pixels=%zu failures=0\n", images, pixels);
    }
    if (std::getenv("RA2_MAP_PROFILE"))
        std::fprintf(
            stderr,
            "VOXEL_GPU images=%zu dispatches=%zu levels=%u parameter_bytes=%zu projection_bytes=%zu depth_bytes=%zu\n",
            voxel_jobs_.size(), dispatches, level_count, words.size() * 4, projection_words * 4, depth_words * 4);
    return true;
}
bool RendererImpl::prepare_direct_resources(const MapFrameData& frame) {
    using Pool = DrawResourcePool;
    using RD = DeviceOperations;
    resource_uploads_.clear();
    voxel_jobs_.clear();
    voxel_model_uploads_.clear();
    std::uint32_t required_layers = resource_layers_;
    std::size_t model_bytes = voxel_model_capacity_;
    bool metadata_dirty = false;
    const auto prepare = [&](auto&& self, const std::shared_ptr<MapBufferData>& data) -> void {
        if (!data) return;
        const auto& allocation = *data->allocation;
        if (data->voxel_model) {
            if (voxel_model_versions_.size() <= allocation.slot)
                voxel_model_versions_.resize(std::size_t(allocation.slot) + 1);
            if (voxel_model_versions_[allocation.slot] == allocation.version) return;
            voxel_model_versions_[allocation.slot] = allocation.version;
            model_bytes = std::max(model_bytes, std::size_t(allocation.first + allocation.count) * 4);
            voxel_model_uploads_.push_back(data.get());
            return;
        }
        if (resource_versions_.size() <= allocation.slot) resource_versions_.resize(std::size_t(allocation.slot) + 1);
        if (resource_metadata_words_.size() < resource_versions_.size())
            resource_metadata_words_.resize(resource_versions_.size());
        if (resource_versions_[allocation.slot] == allocation.version) return;
        required_layers =
            std::max(required_layers, (allocation.first + allocation.count + Pool::PageWords - 1) / Pool::PageWords);
        resource_metadata_words_[allocation.slot] = {int(allocation.first), data->image_width, data->image_height,
                                                     int(allocation.count)};
        resource_versions_[allocation.slot] = allocation.version;
        metadata_dirty = true;
        if (data->table) {
            // A sequence also publishes its frame table after the slot metadata.
            const auto& table = *data->table;
            const std::size_t base = MapSequenceFrameBase + table.allocation->first;
            if (resource_metadata_words_.size() < base + table.entries.size())
                resource_metadata_words_.resize(base + table.entries.size());
            // Planes are byte addressed: the entry holds absolute byte addresses (prefix plane or -1).
            const int origin = int(allocation.first) * 4;
            for (std::size_t i = 0; i < table.entries.size(); ++i) {
                const auto& entry = table.entries[i];
                resource_metadata_words_[base + i] = {origin + entry[0], entry[1], entry[2],
                                                      entry[3] < 0 ? -1 : origin + entry[3]};
            }
        }
        // Dependencies were compiled once by the producer. Only a cold
        // immutable group/image registers its dependencies on this device.
        for (const auto& input : data->dependencies)
            self(self, input);
        if (data->voxel)
            voxel_jobs_.push_back(data.get());
        else
            resource_uploads_.push_back(data.get());
    };
    for (const auto& data : scene_resource_uploads_)
        prepare(prepare, data);
    // The live shape sequences are read only when the set changed; a frame that
    // follows dropped ones still carries every sequence.
    if (frame.sequences && frame.sequences->revision != sequence_revision_) {
        for (const auto& data : frame.sequences->data)
            prepare(prepare, data);
        sequence_revision_ = frame.sequences->revision;
    }
    for (const auto& data : frame.resources)
        prepare(prepare, data);
    if (!voxel_jobs_.empty()) model_bytes = std::max<std::size_t>(model_bytes, 16);
    if (model_bytes > voxel_model_capacity_) {
        free_uniforms();
        const auto capacity =
            std::min(std::size_t(Pool::DefaultWords) * 4,
                     std::max(model_bytes, std::max<std::size_t>(1048576, voxel_model_capacity_ * 2)));
        const auto replacement = rd_->storage_buffer_create(std::uint32_t(capacity));
        ++work.creations;
        if (!replacement.is_valid()) return false;
        if (voxel_models_.is_valid()) {
            if (rd_->buffer_copy(voxel_models_, replacement, 0, 0, std::uint32_t(voxel_model_capacity_)) != OK) {
                rd_->free_rid(replacement);
                return false;
            }
            rd_->free_rid(voxel_models_);
        }
        voxel_models_ = replacement;
        voxel_model_capacity_ = capacity;
    }
    for (const auto* data : voxel_model_uploads_) {
        const auto& allocation = *data->allocation;
        if (rd_->buffer_update(voxel_models_, allocation.first * 4, allocation.count * 4, bytes(data->bytes.get(), data->byte_count)) != OK)
            return false;
        count_update(allocation.count * 4);
    }
    if (required_layers > resource_layers_) {
        const auto limit = std::min(std::uint32_t(Pool::DefaultWords / Pool::PageWords),
                                    std::uint32_t(rd_->limit_get(RD::LIMIT_MAX_TEXTURE_ARRAY_LAYERS)));
        if (required_layers > limit) throw std::length_error("drawing resource array exceeds device layers");
        const auto wanted = std::min(limit, std::max(required_layers, std::max(4u, resource_layers_ * 2)));
        free_uniforms();
        TextureSpec format;
        format->set_texture_type(RD::TEXTURE_TYPE_2D_ARRAY);
        format->set_width(Pool::PageSide);
        format->set_height(Pool::PageSide);
        format->set_array_layers(wanted);
        format->set_format(RD::DATA_FORMAT_R32_UINT);
        format->set_usage_bits(RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT |
                               RD::TEXTURE_USAGE_STORAGE_ATOMIC_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
                               RD::TEXTURE_USAGE_CAN_COPY_TO_BIT);
        const auto replacement = rd_->texture_create(format);
        ++work.creations;
        if (!replacement.is_valid()) return false;
        for (std::uint32_t layer = 0; layer < resource_layers_; ++layer)
            if (rd_->texture_copy(resource_pages_, replacement, {}, {}, {Pool::PageSide, Pool::PageSide, 1}, 0, 0,
                                  layer, layer) != OK) {
                rd_->free_rid(replacement);
                return false;
            }
        if (resource_pages_.is_valid()) rd_->free_rid(resource_pages_);
        resource_pages_ = replacement;
        resource_layers_ = wanted;
    }
    if (!resource_uploads_.empty()) {
        // Only changed ranges cross the CPU/GPU boundary. Growing the array
        // copies existing layers on the GPU, including non-visible resources.
        auto& words = resource_upload_words_;
        words.clear();
        words.resize(resource_uploads_.size() * 3);
        std::uint32_t largest = 0;
        for (std::size_t index = 0; index < resource_uploads_.size(); ++index) {
            const auto& data = *resource_uploads_[index];
            const auto& allocation = *data.allocation;
            words[index * 3] = allocation.first;
            words[index * 3 + 1] = std::uint32_t(words.size());
            words[index * 3 + 2] = allocation.count;
            const auto offset = words.size();
            words.resize(offset + allocation.count);
            std::memcpy(words.data() + offset, data.bytes.get(), allocation.count * 4);
            largest = std::max(largest, allocation.count);
        }
        if (!upload(resource_upload_buffer_, resource_upload_capacity_, words.data(), words.size() * 4)) return false;
        if (!resource_upload_pipeline_.is_valid() &&
            !create_pipeline(map_resource_upload_program, resource_upload_shader_, resource_upload_pipeline_))
            return false;
        if (!resource_upload_uniform_.is_valid())
            resource_upload_uniform_ =
                binding_set(rd_, resource_upload_shader_,
                            {{StorageBuffer, resource_upload_buffer_}, {ImageUniform, resource_pages_}});
        if (!resource_upload_uniform_.is_valid()) return false;
        const auto list = rd_->compute_list_begin();
        rd_->compute_list_bind_compute_pipeline(list, resource_upload_pipeline_);
        rd_->compute_list_bind_uniform_set(list, resource_upload_uniform_, 0);
        rd_->compute_list_dispatch(list, (largest + 63) / 64, std::uint32_t(resource_uploads_.size()), 1);
        ++work.dispatches;
        rd_->compute_list_end();
    }
    if (metadata_dirty && !upload(resource_metadata_, resource_metadata_capacity_, resource_metadata_words_.data(),
                                  resource_metadata_words_.size() * sizeof(resource_metadata_words_[0])))
        return false;
    if (!upload(direct_instances_, direct_instance_capacity_, frame.instances.data(),
                frame.instances.size() * sizeof(RasterInstance)))
        return false;
    if (!upload(direct_compute_, direct_compute_capacity_, frame.compute.data(),
                frame.compute.size() * sizeof(PixelParameters)))
        return false;
    if (!upload(direct_light_bins_, direct_light_bin_capacity_, frame.light_bins.data(), frame.light_bins.size() * 4))
        return false;
    if (!upload(direct_schedule_, direct_schedule_capacity_, frame.compute_resources.data(),
                frame.compute_resources.size() * sizeof(std::int32_t)))
        return false;
    return true;
}
bool RendererImpl::create_direct_uniforms() {
    if (!direct_uniform_.is_valid())
        direct_uniform_ = binding_set(rd_, direct_shader_,
                                      {{StorageBuffer, direct_instances_},
                                       {StorageBuffer, frame_depth_},
                                       {StorageBuffer, lights_},
                                       {StorageBuffer, direct_schedule_},
                                       {StorageBuffer, frame_depth_},
                                       {StorageBuffer, frame_depth_},
                                       {StorageBuffer, frame_depth_},
                                       {SampledTexture, depth_sampler_, light_ring_},
                                       {SampledTexture, depth_sampler_, resource_pages_, 9},
                                       {StorageBuffer, resource_metadata_}});
    if (!direct_compute_pipeline_.is_valid() &&
        !create_pipeline(map_sprite_compute_program, direct_compute_shader_, direct_compute_pipeline_))
        return false;
    if (!direct_compute_uniform_.is_valid())
        direct_compute_uniform_ = binding_set(rd_, direct_compute_shader_,
                                              {{StorageBuffer, direct_compute_},
                                               {StorageBuffer, frame_depth_},
                                               {ImageUniform, colors_},
                                               {SampledTexture, depth_sampler_, raster_depth_},
                                               {StorageBuffer, lights_},
                                               {StorageBuffer, frame_depth_},
                                               {StorageBuffer, direct_schedule_},
                                               {StorageBuffer, frame_depth_},
                                               {SampledTexture, depth_sampler_, resource_pages_},
                                               {StorageBuffer, resource_metadata_}});
    if (!direct_lighting_pipeline_.is_valid() &&
        !create_pipeline(map_sprite_lighting_program, direct_lighting_shader_, direct_lighting_pipeline_))
        return false;
    if (!direct_lighting_uniform_.is_valid())
        direct_lighting_uniform_ = binding_set(rd_, direct_lighting_shader_,
                                               {{StorageBuffer, direct_compute_},
                                                {SampledTexture, depth_sampler_, resource_pages_},
                                                {StorageBuffer, lights_},
                                                {StorageBuffer, direct_light_bins_}});
    return direct_uniform_.is_valid() && direct_compute_uniform_.is_valid() && direct_lighting_uniform_.is_valid();
}
bool RendererImpl::create_uniforms() {
    if (resource_pages_.is_valid() && !light_field_uniform_.is_valid())
        light_field_uniform_ = binding_set(rd_, light_field_shader_,
                                           {{StorageBuffer, light_field_words_},
                                            {SampledTexture, depth_sampler_, resource_pages_},
                                            {StorageBuffer, lights_}});
    if (!present_uniform_.is_valid())
        present_uniform_ = binding_set(rd_, present_shader_,
                                       {{ImageUniform, colors_},
                                        {StorageBuffer, lights_},
                                        {StorageBuffer, screen_snapshot_},
                                        {StorageBuffer, kept_composite_},
                                        {SampledTexture, depth_sampler_, raster_depth_}});
    return present_uniform_.is_valid() && (!resource_pages_.is_valid() || light_field_uniform_.is_valid());
}
bool RendererImpl::render(const MapFrameData& frame) {
    failure_stage_ = "gpu_prepare";
    const auto profile_start = std::chrono::steady_clock::now();
    if (!prepare_device(frame)) return false;
    profiling_ = std::getenv("RA2_MAP_PROFILE") != nullptr;
    work = {};
    timestamps_ = 0;
    pages_generated_ = 0;
    if (profiling_) report_gpu_timing();
    timestamp("ra2:begin");
    const bool world_depth = !frame.scenes.empty();
    const float far_depth = world_depth ? 1.0f : FarDepth;
    scene_resource_uploads_.clear();
    std::size_t scene_upload_bytes = 0;
    for (const auto& mark : frame.scenes)
        if (!prepare_scene(*mark.scene, scene_upload_bytes)) return false;
    if (!prepare_direct_resources(frame)) return false;
    timestamp("ra2:upload");
    failure_stage_ = "gpu_submit";
    if (!generate_voxels()) return false;
    timestamp("ra2:voxel");
    if (!upload(light_field_words_, light_field_capacity_, frame.field_words.data(), frame.field_words.size() * 4))
        return false;
    light_pushes_.clear();
    light_generated_.clear();
    light_pages_generated_ = 0;
    std::vector<std::int32_t> tables;
    for (const auto& field : frame.light_fields)
        if (field.world && !plan_light_ring(field, field.program, tables)) return false;
    if (!light_pushes_.empty()) {
        if (!upload(light_tables_, light_table_capacity_, tables.data(), tables.size() * 4)) return false;
        if (!light_page_uniform_.is_valid())
            light_page_uniform_ = binding_set(rd_, light_page_shader_,
                                              {{StorageBuffer, light_field_words_},
                                               {SampledTexture, depth_sampler_, resource_pages_},
                                               {StorageBuffer, scene_shroud_},
                                               {StorageBuffer, light_tables_},
                                               {ImageUniform, light_ring_},
                                               {StorageBuffer, lights_}});
        if (!light_page_uniform_.is_valid()) return false;
    }
    const bool effect_frame =
        frame.screen_effect.stage != ScreenEffectStage::inactive ||
        std::any_of(frame.screen_effect_marks.begin(), frame.screen_effect_marks.end(),
                    [](const auto& mark) { return (mark.request.operation == ScreenEffectOperation::capture_begin ||
                                 mark.request.operation == ScreenEffectOperation::capture_end); });
    bool page_light = light_pushes_.size() == 2 && frame.light_fields.size() == 1 && !effect_frame &&
                      std::none_of(frame.commands.begin(), frame.commands.end(),
                                   [](const auto& command) { return command.kind == MapCommandKind::lighting; });
    for (const auto& command : frame.commands) {
        if (!page_light || command.kind == MapCommandKind::light_field) break;
        if (command.kind == MapCommandKind::scene) {
            page_light = false;
            break;
        }
        if (command.kind == MapCommandKind::raster)
            for (std::uint32_t i = 0; i < command.count; ++i)
                if (frame.instances[command.first + i][10] & 1) {
                    page_light = false;
                    break;
                }
        if (command.kind == MapCommandKind::compute) page_light = (frame.compute[command.first][10] & 1) == 0;
    }
    const bool expand_lights = !page_light || !frame.compute.empty();
    std::int32_t depth_frame[16]{world_depth ? 1 : 0, frame.view_origin.Y, frame.width, frame.height};
    if (page_light) {
        const auto& e = light_pushes_[1];
        const std::int32_t light[12]{1, e[7], e[8], e[3], e[4], e[5], e[6], e[9], e[10], 0, 0, 0};
        std::copy(std::begin(light), std::end(light), depth_frame + 4);
    }
    if (rd_->buffer_update(frame_depth_, 0, sizeof(depth_frame), bytes(depth_frame, sizeof(depth_frame))) != OK)
        return false;
    count_update(sizeof(depth_frame));
    if (!create_uniforms() || ((!frame.instances.empty() || !frame.compute.empty()) && !create_direct_uniforms()))
        return false;
    const auto profile_prepared = std::chrono::steady_clock::now();
    timestamp("ra2:prepare");
    auto commands = rd_->compute_list_begin();
    if (commands < 0) return false;
    std::size_t dispatches = 0, depth_clears = 0;
    // A frame clear of scene Z waits for the next draw list, which clears it
    // on load, unless a compute path reads Z first.
    bool clear_depth = false;
    const auto flush_depth_clear = [&]() {
        rd_->compute_list_end();
        const auto list =
            rd_->draw_list_begin(raster_framebuffer_, DeviceOperations::DRAW_CLEAR_DEPTH, Colors(), far_depth);
        if (list < 0) return false;
        rd_->draw_list_end();
        timestamp("ra2:depth_clear");
        clear_depth = false;
        ++depth_clears;
        commands = rd_->compute_list_begin();
        return commands >= 0;
    };
    bool commands_ok = true;
    // One draw list stays open across the background and raster runs until a
    // compute pass needs the targets. Frame clears wait for the first writer:
    // a draw list clears color (and Z) on load, a world light field fills the
    // ABuffer, and only a compute writer still clears them in a pass.
    bool list_open = false, pending_color = false, pending_lights = false;
    std::int64_t list = -1;
    const auto close_list = [&](const char* segment = "ra2:raster") {
        if (!list_open) return true;
        rd_->draw_list_end();
        list_open = false;
        timestamp(segment);
        commands = rd_->compute_list_begin();
        return commands >= 0;
    };
    std::function<void()> flush_clears;
    const auto dispatch = [&](Handle pipeline, Handle set, const void* push, std::size_t push_size, int x, int y, int z = 1) {
        if (!commands_ok) return;
        if (!close_list()) {
            commands_ok = false;
            return;
        }
        if (pipeline != present_pipeline_ && pipeline != light_page_pipeline_) flush_clears();
        if (clear_depth && pipeline == direct_compute_pipeline_ && !flush_depth_clear()) {
            commands_ok = false;
            return;
        }
        rd_->compute_list_bind_compute_pipeline(commands, pipeline);
        rd_->compute_list_bind_uniform_set(commands, set, 0);
        rd_->compute_list_set_push_constant(commands, bytes(push, push_size), static_cast<std::uint32_t>(push_size));
        rd_->compute_list_dispatch(commands, x, y, z);
        ++work.dispatches;
        rd_->compute_list_add_barrier(commands);
        ++dispatches;
    };
    // Profiling closes the compute list so a timestamp can end the segment.
    const auto split = [&](const char* segment) {
        if (!profiling_ || !commands_ok) return;
        rd_->compute_list_end();
        timestamp(segment);
        commands = rd_->compute_list_begin();
        if (commands < 0) commands_ok = false;
    };
    // Opens the shared draw list (the compute list is open otherwise).
    const auto open_list = [&]() {
        if (list_open) return true;
        // Raster runs read the ABuffer.
        if (pending_lights) flush_clears();
        if (!commands_ok) return false;
        rd_->compute_list_end();
        std::uint32_t flags = 0;
        Colors clear_colors;
        if (clear_depth) {
            flags |= DeviceOperations::DRAW_CLEAR_DEPTH;
            clear_depth = false;
            ++depth_clears;
        }
        if (pending_color) {
            flags |= DeviceOperations::DRAW_CLEAR_COLOR_0;
            clear_colors.push_back(Color(0, 0, 0, 1));
            pending_color = false;
        }
        list = rd_->draw_list_begin(raster_framebuffer_, DeviceOperations::DrawFlags(flags), clear_colors, far_depth);
        list_open = list >= 0;
        return list_open;
    };
    const int groups_x = (width_ + 7) / 8, groups_y = (height_ + 7) / 8;
    auto effect = frame.screen_effect;
    std::function<void(int)> present = [&](int operation) {
        // Effects read the frame drawn so far.
        if (operation != PresentClear && operation != PresentClearDepth) flush_clears();
        const std::int32_t push[]{width_,
                                  height_,
                                  operation,
                                  operation == PresentIonBlast ? int(world_depth) : int(effect.stage),
                                  effect.warp_step,
                                  effect.initial_duration,
                                  effect.brightness,
                                  effect.depth_base + (world_depth ? 0x100000 - frame.view_origin.Y : 0),
                                  effect.width,
                                  effect.height,
                                  effect.center_x,
                                  effect.center_y};
        dispatch(present_pipeline_, present_uniform_, push, sizeof(push), groups_x, groups_y);
        split(operation == PresentClear || operation == PresentClearDepth ? "ra2:clear" : "ra2:effect");
        if (operation == PresentClear || operation == PresentClearDepth) clear_depth = true;
    };
    flush_clears = [&]() {
        if (!pending_color && !pending_lights) return;
        const int operation = pending_color ? PresentClear : PresentClearDepth;
        pending_color = pending_lights = false;
        present(operation);
    };
    if (effect.stage == ScreenEffectStage::inactive) {
        screen_capture_id_ = 0;
    } else if (screen_capture_id_ != effect.capture_id) {
        // Preserve the previously presented world before clearing this frame.
        present(PresentSnapshot);
        screen_capture_id_ = effect.capture_id;
    }
    // Effect frames keep the previous color; others clear it with the first writer.
    if (effect_frame)
        present(PresentClearDepth);
    else {
        pending_color = pending_lights = true;
        clear_depth = true;
    }

    std::size_t scene_draws = 0, scene_instances = 0;
    const bool pages_enabled = true;
    bool paged_frame = false;
    const auto draw_scene = [&](const MapSceneMark& mark) {
        if (!close_list()) return false;
        rd_->compute_list_end();
        if (!prepare_scene(*mark.scene, scene_upload_bytes)) return false;
        const int view_x = mark.origin.X + mark.clip.X, view_y = mark.origin.Y + mark.clip.Y;
        const auto floor_page = [](int v) { return v >= 0 ? v / PageSize : -((-v + PageSize - 1) / PageSize); };
        // The pages hold the leading instances of the drawing order that depend on no camera: the set is a
        // property of the scene, so a change of it makes every page stale.
        std::vector<std::uint32_t> prefix_ids;
        for (const auto& range : mark.window) {
            if (range.instance >= scene_instance_state_.size() || scene_instance_state_[range.instance].uncacheable)
                break;
            prefix_ids.push_back(range.instance);
        }
        if (prefix_ids != scene_page_prefix_) {
            scene_page_prefix_ = prefix_ids;
            for (auto& page : pages_)
                page.valid = false;
            invalidate_lit_all();
        }
        std::vector<std::uint8_t> in_prefix(scene_instance_state_.size(), 0);
        bool prefix_drawn = false;
        for (const auto& range : mark.window)
            if (std::find(prefix_ids.begin(), prefix_ids.end(), range.instance) != prefix_ids.end()) {
                in_prefix[range.instance] = 1;
                prefix_drawn = prefix_drawn || range.count != 0;
            }
        int page_x = 0, page_y = 0, columns = 0, rows = 0;
        bool paged = pages_enabled && !bypass_pages_ && prefix_drawn && mark.clip.Width > 0 && mark.clip.Height > 0 &&
                     create_pages();
        if (paged) {
            page_x = floor_page(view_x);
            page_y = floor_page(view_y);
            columns = floor_page(view_x + mark.clip.Width - 1) - page_x + 1;
            rows = floor_page(view_y + mark.clip.Height - 1) - page_y + 1;
            paged = std::size_t(columns) * std::size_t(rows) <= pages_.size();
        }
        // The lit background ring serves the paged background while the light is the light ring, no lit pixel
        // follows the camera and not too many of the view's tiles must be made again; otherwise the pages are
        // resolved directly.
        struct Tile {
            int x, y;
        };
        std::vector<Tile> lit_missing;
        bool ringed = false;
        lit_path_ = 0;
        lit_reason_ = "none";
        lit_generated_ = lit_visible_ = 0;
        if (paged) {
            const char* reason = nullptr;
            if (!lit_enabled_)
                reason = "disabled";
            else if (!page_light)
                reason = "abuffer_light";
            else {
                const auto& e = light_pushes_[1];
                if (e[7] != mark.origin.X || e[8] != mark.origin.Y)
                    reason = "light_camera";
                else if (mark.clip.X < e[3] || mark.clip.Y < e[4] || mark.clip.X + mark.clip.Width > e[3] + e[5] ||
                         mark.clip.Y + mark.clip.Height > e[4] + e[6])
                    reason = "light_view";
            }
            if (!reason) {
                std::uint32_t first_row = 0;
                for (const auto instance : prefix_ids)
                    first_row += scene_instance_state_[instance].first_row;
                if (first_row) reason = "first_row_light";
            }
            if (!reason && !lit_ring_.is_valid() && !create_lit_ring()) {
                lit_enabled_ = false;
                reason = "ring_unavailable";
            }
            if (!reason) {
                // Tiles new to the ring are made as they scroll in; a tile a change made stale is made again too,
                // unless most of the view is stale and the changes are still arriving (global lighting): then
                // this frame resolves the pages directly and the tiles are made once the changes stop.
                const bool changing = lit_invalidations_ != lit_invalidations_seen_;
                lit_invalidations_seen_ = lit_invalidations_;
                std::size_t stale = 0;
                for (int y = page_y; y < page_y + rows; ++y)
                    for (int x = page_x; x < page_x + columns; ++x) {
                        const auto& tile = lit_tiles_[std::size_t(y & (lit_rows_ - 1)) * std::size_t(lit_columns_) +
                                                      std::size_t(x & (lit_columns_ - 1))];
                        if (tile.valid && tile.x == x && tile.y == y) continue;
                        lit_missing.push_back({x, y});
                        if (tile.x == x && tile.y == y) ++stale;
                    }
                lit_visible_ = std::size_t(columns) * std::size_t(rows);
                if (columns > lit_columns_ || rows > lit_rows_)
                    reason = "view_larger_than_ring";
                else if (stale * 2 > lit_visible_ && changing)
                    reason = "too_many_stale_tiles";
            }
            ringed = reason == nullptr;
            lit_path_ = ringed ? 1 : 0;
            lit_reason_ = reason ? reason : "ring";
        }
        const auto page_for = [&](int x, int y, int& layer) {
            // The page of a world tile, made when no valid copy exists.
            const auto found = std::find_if(pages_.begin(), pages_.end(),
                                            [&](const PageSlot& p) { return p.valid && p.x == x && p.y == y; });
            if (found != pages_.end()) {
                found->used = page_frame_;
                layer = int(found - pages_.begin());
                return true;
            }
            // Reuse an invalid slot first, then the least recently used.
            PageSlot* slot = nullptr;
            for (auto& p : pages_) {
                if (p.used == page_frame_) continue;
                if (!slot || (slot->valid && !p.valid) || (slot->valid == p.valid && p.used < slot->used)) slot = &p;
            }
            if (!slot || !generate_page(*slot, x, y, mark, in_prefix)) return false;
            slot->used = page_frame_;
            layer = int(slot - pages_.data());
            return true;
        };
        const auto touch = [&](int x, int y) {
            // Pages the frame uses must not be taken for another tile while the others are made.
            for (auto& p : pages_)
                if (p.valid && p.x == x && p.y == y) p.used = page_frame_;
        };
        if (paged && ringed) {
            ++page_frame_;
            // Only the tiles to make need their pages.
            for (const auto& tile : lit_missing)
                touch(tile.x, tile.y);
            std::vector<int> layers(lit_missing.size());
            for (std::size_t i = 0; i < lit_missing.size(); ++i)
                if (!page_for(lit_missing[i].x, lit_missing[i].y, layers[i])) return false;
            timestamp("ra2:pages");
            if (!lit_missing.empty()) {
                if (!lit_tile_uniform_.is_valid())
                    lit_tile_uniform_ = binding_set(rd_, lit_tile_shader_,
                                                    {{StorageBuffer, scene_records_},
                                                     {SampledTexture, depth_sampler_, resource_pages_},
                                                     {StorageBuffer, scene_lights_, {}, 4},
                                                     {StorageBuffer, scene_slots_},
                                                     {SampledTexture, depth_sampler_, page_materials_},
                                                     {SampledTexture, depth_sampler_, page_shadows_},
                                                     {SampledTexture, depth_sampler_, page_depths_},
                                                     {SampledTexture, depth_sampler_, light_ring_}});
                if (!lit_tile_uniform_.is_valid()) return false;
                const auto tile_list = rd_->draw_list_begin(lit_framebuffer_, DeviceOperations::DrawFlags(0),
                                                            Colors(), 1.0f);
                if (tile_list < 0) return false;
                rd_->draw_list_bind_render_pipeline(tile_list, lit_tile_pipeline_);
                rd_->draw_list_bind_uniform_set(tile_list, lit_tile_uniform_, 0);
                for (std::size_t i = 0; i < lit_missing.size(); ++i) {
                    const auto [x, y] = lit_missing[i];
                    const std::int32_t push[12]{x * PageSize,
                                                y * PageSize,
                                                (x & (lit_columns_ - 1)) * PageSize,
                                                (y & (lit_rows_ - 1)) * PageSize,
                                                layers[i],
                                                light_ring_columns_ * PageSize - 1,
                                                light_ring_rows_ * PageSize - 1,
                                                lit_columns_ * PageSize,
                                                lit_rows_ * PageSize,
                                                0,
                                                0,
                                                0};
                    rd_->draw_list_set_push_constant(tile_list, bytes(push, sizeof(push)), sizeof(push));
                    rd_->draw_list_draw(tile_list, false, 1, 6);
                    ++work.draws;
                    lit_tiles_[std::size_t(y & (lit_rows_ - 1)) * std::size_t(lit_columns_) +
                               std::size_t(x & (lit_columns_ - 1))] = {x, y, true};
                }
                rd_->draw_list_end();
                lit_generated_ = lit_missing.size();
            }
            if (!lit_copy_uniform_.is_valid())
                lit_copy_uniform_ = binding_set(rd_, lit_copy_shader_, {{SampledTexture, depth_sampler_, lit_ring_}});
            if (!lit_copy_uniform_.is_valid()) return false;
            timestamp("ra2:ring");
        } else if (paged) {
            ++page_frame_;
            std::vector<std::int32_t> table(std::size_t(columns) * rows, -1);
            for (int y = 0; y < rows; ++y)
                for (int x = 0; x < columns; ++x)
                    touch(page_x + x, page_y + y);
            for (int y = 0; y < rows; ++y)
                for (int x = 0; x < columns; ++x) {
                    int layer = 0;
                    if (!page_for(page_x + x, page_y + y, layer)) return false;
                    table[std::size_t(y) * columns + x] = layer;
                }
            if (rd_->buffer_update(page_table_, 0, std::uint32_t(table.size() * 4),
                                   bytes(table.data(), table.size() * 4)) != OK)
                return false;
            count_update(table.size() * 4);
            if (!resolve_uniform_.is_valid())
                resolve_uniform_ =
                    binding_set(rd_, resolve_shader_,
                                {{StorageBuffer, scene_records_},
                                 {SampledTexture, depth_sampler_, resource_pages_},
                                 {StorageBuffer, lights_},
                                 {StorageBuffer, page_table_},
                                 {StorageBuffer, scene_lights_},
                                 {StorageBuffer, scene_slots_},
                                 {SampledTexture, depth_sampler_, page_materials_},
                                 {SampledTexture, depth_sampler_, page_shadows_},
                                 {SampledTexture, depth_sampler_, page_depths_},
                                 {SampledTexture, depth_sampler_, light_ring_},
                                 {StorageBuffer, frame_depth_}});
            if (!resolve_uniform_.is_valid()) return false;
            timestamp("ra2:pages");
        }
        if (!resident_uniform_.is_valid()) {
            // Parameters are the records; instance i of a draw is the record the draw order names at first + i
            // (binding 3).
            resident_uniform_ =
                binding_set(rd_, direct_shader_,
                            {{StorageBuffer, scene_records_},
                             {StorageBuffer, frame_depth_},
                             {StorageBuffer, lights_},
                             {StorageBuffer, scene_order_},
                             {StorageBuffer, frame_depth_},
                             {StorageBuffer, scene_lights_},
                             {StorageBuffer, scene_slots_},
                             {SampledTexture, depth_sampler_, light_ring_},
                             {SampledTexture, depth_sampler_, resource_pages_, 9},
                             {StorageBuffer, resource_metadata_}});
            if (!resident_uniform_.is_valid()) return false;
        }
        paged_frame = paged;
        commands = rd_->compute_list_begin();
        if (commands < 0 || !open_list()) return false;
        if (paged && ringed) {
            rd_->draw_list_bind_render_pipeline(list, lit_copy_pipeline_);
            rd_->draw_list_bind_uniform_set(list, lit_copy_uniform_, 0);
            const std::int32_t push[12]{mark.origin.X,
                                        mark.origin.Y,
                                        mark.clip.X,
                                        mark.clip.Y,
                                        mark.clip.Width,
                                        mark.clip.Height,
                                        lit_columns_ * PageSize - 1,
                                        lit_rows_ * PageSize - 1,
                                        width_,
                                        height_,
                                        0,
                                        0};
            rd_->draw_list_set_push_constant(list, bytes(push, sizeof(push)), sizeof(push));
            rd_->draw_list_draw(list, false, 1, 6);
            ++scene_draws;
            ++work.draws;
        } else if (paged) {
            rd_->draw_list_bind_render_pipeline(list, resolve_pipeline_);
            rd_->draw_list_bind_uniform_set(list, resolve_uniform_, 0);
            const std::int32_t push[12]{mark.origin.X,    mark.origin.Y, mark.clip.X, mark.clip.Y, mark.clip.Width,
                                        mark.clip.Height, page_x,        page_y,      columns,     rows,
                                        width_,           height_};
            rd_->draw_list_set_push_constant(list, bytes(push, sizeof(push)), sizeof(push));
            rd_->draw_list_draw(list, false, 1, 6);
            ++scene_draws;
            ++work.draws;
        }
        {
            // The ranges the core computed for the viewport, as ranges of each instance's draw order (its occupied
            // slots); GPU clipping trims what the ranges overshoot.
            int bound = -1;
            for (const auto& range : mark.visible) {
                if (!range.count || range.instance >= scene_instance_state_.size()) continue;
                if (paged && in_prefix[range.instance]) continue;
                const auto& state = scene_instance_state_[range.instance];
                if (int(state.variant) != bound) {
                    bound = int(state.variant);
                    rd_->draw_list_bind_render_pipeline(list, direct_pipelines_[bound]);
                    rd_->draw_list_bind_uniform_set(list, resident_uniform_, 0);
                    rd_->draw_list_set_blend_constants(list, raster_blend_constant(bound));
                }
                const std::int32_t push[8]{int(state.base + range.first),
                                           mark.origin.X,
                                           mark.origin.Y,
                                           1,
                                           mark.clip.X,
                                           mark.clip.Y,
                                           mark.clip.Width,
                                           mark.clip.Height};
                rd_->draw_list_set_push_constant(list, bytes(push, sizeof(push)), sizeof(push));
                rd_->draw_list_draw(list, false, range.count, 6);
                ++scene_draws;
                ++work.draws;
                scene_instances += range.count;
            }
        }
        // Profiling ends the draw list after the background, so its GPU time is a segment of its own and the
        // foreground raster runs that follow load the background's color and depth.
        if (profiling_ && !close_list("ra2:background")) return false;
        return true;
    };
    std::size_t world_field = 0;
    for (const auto& command : frame.commands) {
        switch (command.kind) {
            case MapCommandKind::raster: {
                if (!open_list()) return false;
                rd_->draw_list_bind_render_pipeline(list, direct_pipelines_[command.variant]);
                rd_->draw_list_bind_uniform_set(list, direct_uniform_, 0);
                rd_->draw_list_set_blend_constants(list, raster_blend_constant(command.variant));
                const std::int32_t push[8]{int(command.first), 0, 0, 2, 0, 0, 0, 0};
                rd_->draw_list_set_push_constant(list, bytes(push, sizeof(push)), sizeof(push));
                rd_->draw_list_draw(list, false, command.count, 6);
                ++work.draws;
                break;
            }
            case MapCommandKind::compute: {
                const auto& words = frame.compute[command.first];
                const auto first = std::int32_t(command.first);
                const int scale = words[19] == 13 ? PixelBinSize : 1;
                if (words[2] > 0 && words[3] > 0)
                    dispatch(direct_compute_pipeline_, direct_compute_uniform_, &first, sizeof(first),
                             words[19] == 11 ? (words[2] + 63) / 64 : (words[2] * scale + 7) / 8,
                             words[19] == 11 ? 1 : (words[3] * scale + 7) / 8);
                split("ra2:compute");
                break;
            }
            case MapCommandKind::lighting: {
                const std::int32_t push[8]{
                    width_, height_, (width_ + 32 - 1) / 32, command.bin_offset};
                dispatch(direct_lighting_pipeline_, direct_lighting_uniform_, push, sizeof(push), groups_x, groups_y);
                split("ra2:lighting");
                break;
            }
            case MapCommandKind::scene:
                if (!draw_scene(frame.scenes[command.first])) return false;
                break;
            case MapCommandKind::light_field: {
                const auto& field = frame.light_fields[command.first];
                if (field.world) {
                    const auto& pushes = light_pushes_[world_field * 2];
                    if (light_generated_[world_field] > 0)
                        dispatch(light_page_pipeline_, light_page_uniform_, pushes.data(), sizeof(pushes),
                                 PageSize / 8, PageSize / 8, light_generated_[world_field]);
                    if (expand_lights) {
                        auto expand = light_pushes_[world_field * 2 + 1];
                        if (pending_lights) {
                            expand[13] = 1;
                            pending_lights = false;
                        }
                        dispatch(light_page_pipeline_, light_page_uniform_, expand.data(), sizeof(expand), groups_x,
                                 groups_y);
                    } else
                        pending_lights = false;
                    ++world_field;
                } else {
                    const std::int32_t push[]{width_, height_, field.program, 0};
                    dispatch(light_field_pipeline_, light_field_uniform_, push, sizeof(push), groups_x, groups_y);
                }
                split("ra2:light_field");
                break;
            }
            case MapCommandKind::effect: {
                const auto& mark = frame.screen_effect_marks[command.first].request;
                effect = mark;
                switch (mark.operation) {
                    case ScreenEffectOperation::capture_begin:
                        present(PresentKeepComposite);
                        present(PresentClear);
                        break;
                    case ScreenEffectOperation::capture_end:
                        present(PresentSnapshot);
                        present(PresentRestoreComposite);
                        break;
                    case ScreenEffectOperation::apply:
                        if (mark.stage != ScreenEffectStage::inactive) present(PresentEffect);
                        break;
                    case ScreenEffectOperation::ion_blast:
                        present(PresentKeepComposite);
                        present(PresentIonBlast);
                        break;
                }
                break;
            }
        }
    }
    if (!close_list()) return false;
    flush_clears();
    rd_->compute_list_end();
    if (!commands_ok) return false;
    if (profiling_) {
        std::fprintf(
            stderr, "MAP_PROFILE records=%zu commands=%zu resources=%zu prepare_us=%lld dispatch_us=%lld\n",
            frame.instances.size() + frame.compute.size(), frame.commands.size(), frame.resources.size(),
            static_cast<long long>(
                std::chrono::duration_cast<std::chrono::microseconds>(profile_prepared - profile_start).count()),
            static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::steady_clock::now() - profile_prepared)
                                       .count()));
        std::fprintf(stderr,
                     "BACKGROUND_GPU layers=%zu draws=%zu instances=%zu upload_bytes=%zu atlas_bytes=%zu paged=%d "
                     "pages_generated=%zu prefix=%zu slots=%zu\n",
                     scene_instance_state_.size(), scene_draws, scene_instances, scene_upload_bytes,
                     std::size_t(resource_layers_) * DrawResourcePool::PageWords * 4, int(paged_frame),
                     pages_generated_, scene_page_prefix_.size(), std::size_t(scene_slot_total_));
        std::fprintf(stderr,
                     "LIT_RING path=%d reason=%s tiles_made=%zu tiles_visible=%zu columns=%d rows=%d light_cells=%zu\n",
                     lit_path_, lit_reason_, lit_generated_, lit_visible_, lit_columns_, lit_rows_, lit_light_cells_);
        lit_light_cells_ = 0;
        std::fprintf(stderr, "LIGHT_RING page_light=%d tiles_generated=%zu columns=%d rows=%d\n", int(page_light),
                     light_pages_generated_, light_ring_columns_, light_ring_rows_);
        std::fprintf(stderr,
                     "MAP_DEVICE draws=%zu dispatches=%zu buffer_updates=%zu update_bytes=%zu creations=%zu "
                     "uniform_sets=%zu timestamps=%u\n",
                     work.draws, work.dispatches, work.buffer_updates, work.update_bytes, work.creations,
                     work.uniform_sets, timestamps_);
    }
    return true;
}

// Diagnostic replay consumes the already recorded original requests. It never
// invokes Draw twice, and runs only when explicitly requested by the test host.
void RendererImpl::verify_scene_pages(const MapFrameData& frame) {
    auto cached = std::make_unique<RendererImpl>(rd_->device());
    auto reference = std::make_unique<RendererImpl>(rd_->device());
    reference->bypass_pages_ = true;
    try {
        MapFrameData sample;
        sample.generation = frame.generation;
        sample.scenes.push_back(frame.scenes.front());
        auto& mark = sample.scenes.front();
        // The replay moves the camera: draw every instance whole, in the order the core listed them.
        std::vector<SlotRange> everything;
        for (const auto& range : mark.window)
            everything.push_back({range.instance, 0, (*mark.scene->instances)[range.instance].drawn});
        mark.visible = mark.window = everything;
        sample.commands.push_back({MapCommandKind::scene, 0});
        const auto origin = mark.origin;
        std::size_t positions = 0;
        int cases = 0;
        for (const auto size : {Point{512, 320}, Point{1024, 768}, Point{1920, 1080}}) {
            sample.width = size.X;
            sample.height = size.Y;
            for (const auto shift : {Point{-257, -257}, Point{-1, -1}, Point{0, 0}, Point{1, 1}, Point{2, 2},
                                     Point{255, 256}}) {
                mark.origin = {origin.X + shift.X, origin.Y + shift.Y};
                sample.view_origin = mark.origin;
                mark.clip = {7, 11, size.X - 21, size.Y - 29};
                if (!cached->render(sample) || !reference->render(sample))
                    throw std::runtime_error("Scene replay render failed");
                const auto color = cached->rd_->texture_get_data(cached->colors_, 0);
                const auto depth = cached->rd_->texture_get_data(cached->raster_depth_, 0);
                const auto expected_color = reference->rd_->texture_get_data(reference->colors_, 0);
                const auto expected_depth = reference->rd_->texture_get_data(reference->raster_depth_, 0);
                if (color != expected_color || depth != expected_depth) {
                    std::size_t colors = 0, depths = 0;
                    for (std::int64_t i = 0; i < std::min(color.size(), expected_color.size()); i += 4)
                        colors += std::memcmp(color.data() + i, expected_color.data() + i, 4) != 0;
                    for (std::int64_t i = 0; i < std::min(depth.size(), expected_depth.size()); i += 4)
                        depths += std::memcmp(depth.data() + i, expected_depth.data() + i, 4) != 0;
                    char message[256];
                    std::snprintf(message, sizeof(message),
                                  "Scene page mismatch: %dx%d camera %d,%d colors=%zu depth=%zu prefix=%zu", size.X,
                                  size.Y, mark.origin.X, mark.origin.Y, colors, depths,
                                  cached->scene_page_prefix_.size());
                    throw std::runtime_error(message);
                }
                positions += std::size_t(size.X) * size.Y;
                ++cases;
            }
        }
        std::fprintf(stderr,
                     "SCENE_PAGE_REFERENCE cases=%d color_depth_positions=%zu failures=0 prefix=%zu records=%zu\n",
                     cases, positions, cached->scene_page_prefix_.size(), std::size_t(cached->scene_slot_total_));
    } catch (...) {
        cached->clear();
        reference->clear();
        throw;
    }
    cached->clear();
    reference->clear();
}

void RendererImpl::free_scene() {
    for (Handle* rid : {&resident_uniform_, &page_uniform_, &resolve_uniform_, &light_page_uniform_, &lit_tile_uniform_,
                     &lit_copy_uniform_})
        if (rid->is_valid()) {
            rd_->free_rid(*rid);
            *rid = {};
        }
    scene_slot_snapshot_.reset();
    scene_order_snapshot_.reset();
    scene_instance_state_.clear();
    scene_slot_total_ = 1;
    scene_cache_height_ = -1;
    for (Handle* rid : {&scene_lights_, &scene_slots_, &scene_records_, &scene_order_})
        if (rid->is_valid()) {
            rd_->free_rid(*rid);
            *rid = {};
        }
    scene_light_capacity_ = scene_slot_capacity_ = scene_order_capacity_ = 0;
    scene_epoch_ = scene_revision_ = 0;
    scene_record_capacity_ = 0;
    scene_light_sources_.reset();
    scene_slot_sources_.reset();
    scene_dirty_.clear();
    scene_page_prefix_.clear();
    for (auto& page : pages_)
        page.valid = false;
    invalidate_lit_all();
    cell_bounds_.clear();
    converter_bounds_.clear();
    converter_versions_.clear();
    scene_shroud_sources_.reset();
    scene_shroud_clear_ = true;
    light_alphas_.reset();
    light_alpha_words_.clear();
    light_alpha_words_stale_ = true;
    light_dirty_cells_.clear();
    light_dirty_.clear();
    light_all_dirty_ = true;
}
bool RendererImpl::prepare_scene(const MapSceneData& scene, std::size_t& uploaded) {
    if (scene_epoch_ == scene.epoch && scene_revision_ == scene.revision) return true;
    if (scene_epoch_ != scene.epoch) {
        free_scene();
        scene_epoch_ = scene.epoch;
    }
    const auto free_scene_uniforms = [&] {
        for (Handle* rid : {&resident_uniform_, &page_uniform_, &resolve_uniform_, &light_page_uniform_, &lit_tile_uniform_,
                         &lit_copy_uniform_})
            if (rid->is_valid()) {
                rd_->free_rid(*rid);
                *rid = {};
            }
    };
    const auto record_bounds = [&](const std::array<std::int32_t, 12>& w) {
        scene_dirty_.push_back({w[3], w[4], w[2] & 0xFFFF, int(std::uint32_t(w[2]) >> 16)});
    };
    const bool slots_changed = scene.converters != scene_slot_sources_;
    if (slots_changed) {
        scene_slot_sources_ = scene.converters;
        for (const auto& colors : *scene_slot_sources_)
            if (colors) scene_resource_uploads_.push_back(colors);
        // A converter palette that was replaced changes the lit color of every record that uses its slot.
        if (converter_versions_.size() < scene_slot_sources_->size()) converter_versions_.resize(scene_slot_sources_->size(), 0);
        for (std::size_t i = 0; i < scene_slot_sources_->size(); ++i) {
            const auto& colors = (*scene_slot_sources_)[i];
            const std::uint64_t version = colors && colors->allocation ? colors->allocation->version : 0;
            if (version == converter_versions_[i]) continue;
            converter_versions_[i] = version;
            if (i < converter_bounds_.size()) {
                const auto& b = converter_bounds_[i];
                invalidate_lit(b[0], b[1], b[2] - b[0], b[3] - b[1]);
            }
        }
    }
    note_alphas(scene);
    if (light_alpha_words_stale_) {
        light_alpha_words_.clear();
        if (light_alphas_)
            for (const auto& alpha : *light_alphas_) {
                const auto& b = alpha.image.bounds;
                light_alpha_words_.insert(light_alpha_words_.end(),
                                          {int(alpha.image.source->allocation->first), alpha.position.X + b.X,
                                           alpha.position.Y + b.Y, b.Width, b.Height});
            }
        light_alpha_words_stale_ = false;
    }
    // A grown buffer is rebound by every scene uniform set.
    const auto ensure = [&](Handle& buffer, std::size_t& capacity, std::size_t size) {
        size = std::max<std::size_t>(size, 16);
        if (buffer.is_valid() && capacity >= size) return true;
        free_scene_uniforms();
        if (buffer.is_valid()) rd_->free_rid(buffer);
        capacity = std::max(size, capacity * 2);
        if (capacity > std::size_t(INT32_MAX)) throw std::length_error("background scene buffer");
        buffer = rd_->storage_buffer_create(std::uint32_t(capacity));
        ++work.creations;
        return buffer.is_valid();
    };
    constexpr std::size_t ChunkBytes = sizeof(MapSceneLightChunk);
    if (!scene_lights_.is_valid() || scene_light_capacity_ < std::size_t(scene.light_chunks) * ChunkBytes) {
        if (!ensure(scene_lights_, scene_light_capacity_, std::size_t(scene.light_chunks) * ChunkBytes)) return false;
        scene_light_sources_.reset();
    }
    bool uploaded_ok = true;
    std::size_t light_cells_changed = 0;
    scene.lights->changes(scene_light_sources_.get(), [&](const auto& key, const auto& before, const auto& chunk) {
        if (!chunk) return;
        if (rd_->buffer_update(scene_lights_, std::uint32_t(key.second * ChunkBytes), std::uint32_t(ChunkBytes),
                               bytes(chunk->words.data(), ChunkBytes)) != OK) {
            uploaded_ok = false;
            return;
        }
        count_update(ChunkBytes);
        uploaded += ChunkBytes;
        // The cells whose light changed invalidate the lit pixels of the records that read them. A change of
        // many cells (global lighting) takes the whole ring instead of a lookup per cell.
        static const std::array<std::int32_t, MapLightWords> unlit{};
        for (std::uint32_t cell = 0; cell < MapLightChunkCells && light_cells_changed <= 4096; ++cell) {
            const auto* now = chunk->words.data() + std::size_t(cell) * MapLightWords;
            const auto* was = before ? before->words.data() + std::size_t(cell) * MapLightWords : unlit.data();
            if (std::memcmp(now, was, MapLightWords * sizeof(std::int32_t)) == 0) continue;
            if (++light_cells_changed > 4096) break;
            const auto found = cell_bounds_.find(std::uint32_t(key.second) * MapLightChunkCells + cell);
            if (found == cell_bounds_.end()) continue;
            ++lit_light_cells_;
            const auto& b = found->second;
            invalidate_lit(b[0], b[1], b[2] - b[0], b[3] - b[1]);
        }
        if (light_cells_changed > 4096) invalidate_lit_all();
    });
    if (!uploaded_ok) return false;
    scene_light_sources_ = scene.lights;
    if (scene_shroud_clear_) {
        shroud_mirror_.assign(512 * 512, -1);
        light_dirty_cells_.clear();
        light_all_dirty_ = true;
        std::vector<std::int32_t> none(512 * 512, -1);
        if (rd_->buffer_update(scene_shroud_, 0, std::uint32_t(none.size() * 4), bytes(none.data(), none.size() * 4)) !=
            OK)
            return false;
        count_update(none.size() * 4);
        uploaded += none.size() * 4;
        scene_shroud_clear_ = false;
    }
    {
        constexpr std::size_t ShroudChunkBytes = sizeof(MapSceneShroudChunk::frames);
        scene.shroud->changes(scene_shroud_sources_.get(), [&](const auto& key, const auto&, const auto& chunk) {
            if (!chunk) return;
            const auto i = std::size_t(key.second);
            for (std::size_t cell = 0; cell < chunk->frames.size(); ++cell) {
                auto& known = shroud_mirror_[i * MapLightChunkCells + cell];
                if (known == chunk->frames[cell]) continue;
                known = chunk->frames[cell];
                if (!light_all_dirty_) light_dirty_cells_.push_back(std::uint32_t(i * MapLightChunkCells + cell));
            }
            if (light_dirty_cells_.size() > 4096) {
                light_dirty_cells_.clear();
                light_all_dirty_ = true;
            }
            if (rd_->buffer_update(scene_shroud_, std::uint32_t(i * ShroudChunkBytes), std::uint32_t(ShroudChunkBytes),
                                   bytes(chunk->frames.data(), ShroudChunkBytes)) != OK) {
                uploaded_ok = false;
                return;
            }
            count_update(ShroudChunkBytes);
            uploaded += ShroudChunkBytes;
        });
        if (!uploaded_ok) return false;
        scene_shroud_sources_ = scene.shroud;
    }
    if (!scene_slots_.is_valid() || slots_changed) {
        if (!ensure(scene_slots_, scene_slot_capacity_, scene_slot_sources_->size() * 4)) return false;
        std::vector<std::int32_t> offsets(std::max<std::size_t>(scene_slot_sources_->size(), 1), 0);
        for (std::size_t i = 0; i < scene_slot_sources_->size(); ++i)
            if (const auto& buffer = (*scene_slot_sources_)[i]) offsets[i] = int(buffer->allocation->first);
        const auto size = offsets.size() * 4;
        if (rd_->buffer_update(scene_slots_, 0, std::uint32_t(size), bytes(offsets.data(), size)) != OK) return false;
        count_update(size);
        uploaded += size;
    }
    // Instances: bases follow ids, in creation order; slot 0 of the record buffer is reserved.
    for (std::size_t id = scene_instance_state_.size(); id < scene.instances->size(); ++id) {
        const auto& source = (*scene.instances)[id];
        if (std::uint64_t(scene_slot_total_) + source.slots >= (1u << 24))
            throw std::length_error("background record ids");
        scene_instance_state_.push_back({scene_slot_total_, source.slots, source.variant, 0});
        scene_slot_total_ += source.slots;
    }
    constexpr std::size_t RecordBytes = 12 * sizeof(std::int32_t);
    if (!scene_records_.is_valid() || scene_record_capacity_ < std::size_t(scene_slot_total_) * RecordBytes) {
        // A grown buffer keeps its slots (copied) and starts zeroed, so unwritten slots draw nothing.
        const auto capacity = std::max<std::size_t>(std::size_t(scene_slot_total_) * RecordBytes,
                                                    scene_record_capacity_ + scene_record_capacity_ / 2);
        if (capacity > std::size_t(INT32_MAX)) throw std::length_error("background scene buffer");
        free_scene_uniforms();
        const auto fresh = rd_->storage_buffer_create(std::uint32_t(capacity));
        ++work.creations;
        if (!fresh.is_valid()) return false;
        if (rd_->buffer_clear(fresh, 0, std::uint32_t(capacity)) != OK) return false;
        if (scene_records_.is_valid()) {
            if (rd_->buffer_copy(scene_records_, fresh, 0, 0, std::uint32_t(scene_record_capacity_)) != OK)
                return false;
            rd_->free_rid(scene_records_);
        }
        scene_records_ = fresh;
        scene_record_capacity_ = capacity;
    }
    if (!scene_order_.is_valid() || scene_order_capacity_ < std::size_t(scene_slot_total_) * 4) {
        // Grown like the records: the entries written so far are copied.
        const auto capacity = std::max<std::size_t>(std::size_t(scene_slot_total_) * 4,
                                                    scene_order_capacity_ + scene_order_capacity_ / 2);
        free_scene_uniforms();
        const auto fresh = rd_->storage_buffer_create(std::uint32_t(capacity));
        ++work.creations;
        if (!fresh.is_valid()) return false;
        if (scene_order_.is_valid()) {
            if (rd_->buffer_copy(scene_order_, fresh, 0, 0, std::uint32_t(scene_order_capacity_)) != OK) return false;
            rd_->free_rid(scene_order_);
        }
        scene_order_ = fresh;
        scene_order_capacity_ = capacity;
    }
    {
        // Draw order chunks that changed, as record ids.
        std::vector<std::uint32_t> ids;
        bool ok = true;
        scene.order->changes(scene_order_snapshot_.get(), [&](const auto& key, const auto&, const auto& chunk) {
            if (!ok || !chunk) return;
            if (std::size_t(key.first) >= scene_instance_state_.size()) {
                ok = false;
                return;
            }
            const auto& state = scene_instance_state_[std::size_t(key.first)];
            const std::uint32_t first = std::uint32_t(key.second) * MapSceneOrderChunkEntries;
            if (first >= state.slots) return;
            const auto count = std::min<std::uint32_t>(MapSceneOrderChunkEntries, state.slots - first);
            ids.resize(count);
            for (std::uint32_t i = 0; i < count; ++i)
                ids[i] = state.base + chunk->slots[i];
            if (rd_->buffer_update(scene_order_, (state.base + first) * 4, count * 4, bytes(ids.data(), count * 4)) !=
                OK) {
                ok = false;
                return;
            }
            count_update(count * 4);
            uploaded += count * 4;
        });
        if (!ok) return false;
        scene_order_snapshot_ = scene.order;
    }
    // A lit pixel that takes its light from the first visible row follows the camera (see first_row).
    const auto first_row_light = [](const std::array<std::int32_t, 12>& w) {
        const auto metadata = std::uint32_t(w[7]);
        return (metadata & 0x300u) == 0x300u && ((std::uint32_t(w[6]) >> 8) & 0xFFu) != 1u;
    };
    const auto grow_bounds = [](std::array<std::int32_t, 4>& b, const std::array<std::int32_t, 4>& r) {
        if (b[2] <= b[0] || b[3] <= b[1]) {
            b = r;
            return;
        }
        b = {std::min(b[0], r[0]), std::min(b[1], r[1]), std::max(b[2], r[2]), std::max(b[3], r[3])};
    };
    // Which records depend on the camera (see scene_page_cacheable) depends on the viewport height.
    const auto cacheable = [&](const MapSceneRecord& record) {
        return scene_page_cacheable(record.words, height_);
    };
    if (scene_cache_height_ != height_) {
        for (auto& state : scene_instance_state_)
            state.uncacheable = 0;
        if (scene_slot_snapshot_)
            scene_slot_snapshot_->visit([&](const auto& key, const auto& chunk) {
                if (std::size_t(key.first) >= scene_instance_state_.size()) return;
                for (const auto& record : chunk->slots)
                    if (!record.empty() && !cacheable(record))
                        ++scene_instance_state_[std::size_t(key.first)].uncacheable;
            });
        scene_cache_height_ = height_;
        for (auto& page : pages_)
            page.valid = false;
        invalidate_lit_all();
    }
    // Chunks that changed since the last scene this renderer prepared: their records are written to the
    // buffer, the pages under the changed records are stale, and their sources are made resident.
    std::vector<std::int32_t> words;
    std::uint32_t run_first = 0, run_slots = 0;
    bool written = true;
    const auto flush = [&] {
        if (!run_slots) return;
        const auto size = words.size() * sizeof(std::int32_t);
        if (rd_->buffer_update(scene_records_, std::uint32_t(run_first * RecordBytes), std::uint32_t(size),
                               bytes(words.data(), size)) != OK)
            written = false;
        count_update(size);
        uploaded += size;
        words.clear();
        run_slots = 0;
    };
    static const MapSceneRecord none;
    scene.slots->changes(scene_slot_snapshot_.get(), [&](const auto& key, const auto& before, const auto& chunk) {
        if (!written) return;
        if (std::size_t(key.first) >= scene_instance_state_.size()) {
            written = false;
            return;
        }
        auto& state = scene_instance_state_[std::size_t(key.first)];
        const std::uint32_t first = std::uint32_t(key.second) * MapSceneChunkSlots;
        const std::uint32_t count = std::min<std::uint32_t>(MapSceneChunkSlots, state.slots - first);
        const std::uint32_t global = state.base + first;
        if (run_slots && global != run_first + run_slots) flush();
        if (!run_slots) run_first = global;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto& was = before ? before->slots[i] : none;
            const auto& now = chunk ? chunk->slots[i] : none;
            if (!was.empty() && !cacheable(was)) --state.uncacheable;
            if (!now.empty() && !cacheable(now)) ++state.uncacheable;
            if (!was.empty() && first_row_light(was.words)) --state.first_row;
            if (!now.empty() && first_row_light(now.words)) ++state.first_row;
            if (!now.empty()) {
                const auto& w = now.words;
                const std::array<std::int32_t, 4> rect{w[3], w[4], w[3] + (w[2] & 0xFFFF), w[4] + int(std::uint32_t(w[2]) >> 16)};
                grow_bounds(cell_bounds_[std::uint32_t(w[10]) & 0x3FFFFu], rect);
                if (((std::uint32_t(w[10]) >> 24) & 3u) == 3u && w[1] >= 0) {
                    if (converter_bounds_.size() <= std::size_t(w[1])) converter_bounds_.resize(std::size_t(w[1]) + 1, {});
                    grow_bounds(converter_bounds_[std::size_t(w[1])], rect);
                }
            }
            if (!(was == now)) {
                // Reordering overlapping equal-Z records can change the winning pixel as well as replacing one.
                if (!was.empty()) record_bounds(was.words);
                if (!now.empty()) record_bounds(now.words);
            }
            if (now.empty()) {
                words.insert(words.end(), 12, 0);
                continue;
            }
            scene_resource_uploads_.push_back(now.source);
            if (now.auxiliary) scene_resource_uploads_.push_back(now.auxiliary);
            if (now.palette) scene_resource_uploads_.push_back(now.palette);
            words.insert(words.end(), now.words.begin(), now.words.end());
        }
        run_slots += count;
    });
    flush();
    if (!written) return false;
    // Invalidate the pages the changed records touch.
    for (const auto& rect : scene_dirty_)
        for (auto& page : pages_) {
            if (!page.valid) continue;
            const int x = page.x * PageSize, y = page.y * PageSize;
            if (rect.x < x + PageSize && rect.y < y + PageSize && rect.x + rect.width > x && rect.y + rect.height > y)
                page.valid = false;
        }
    for (const auto& rect : scene_dirty_)
        invalidate_lit(rect.x, rect.y, rect.width, rect.height);
    scene_dirty_.clear();
    scene_slot_snapshot_ = scene.slots;
    scene_revision_ = scene.revision;
    return true;
}
void RendererImpl::timestamp(const char* segment) {
    // Godot keeps a bounded number of timestamps per frame for every user.
    // Its Metal driver reports zero GPU times; measure with --rendering-driver vulkan.
    if (!profiling_ || timestamps_ >= 48) return;
    rd_->capture_timestamp(segment);
    ++timestamps_;
}
void RendererImpl::report_gpu_timing() {
    // Timestamps resolve a few frames later; each is reported once. A frame
    // can hold several render() calls (screen capture passes), all summed.
    const auto frame = rd_->get_captured_timestamps_frame();
    if (frame == timing_frame_) return;
    timing_frame_ = frame;
    static constexpr const char* names[]{"upload",     "voxel",   "prepare",    "clear",   "depth_clear", "pages",
                                         "ring",       "background", "raster", "light_field", "abuffer", "compute",
                                         "effect",     "present"};
    constexpr std::size_t count = sizeof(names) / sizeof(names[0]);
    std::uint64_t totals[count]{};
    std::uint64_t previous = 0, first = 0, last = 0;
    bool open = false;
    for (std::uint32_t i = 0; i < rd_->get_captured_timestamps_count(); ++i) {
        const auto name = rd_->get_captured_timestamp_name(i);
        if (!name.starts_with("ra2:")) continue;
        const auto time = rd_->get_captured_timestamp_gpu_time(i);
        const auto segment = name.substr(4);
        if (segment == "begin") {
            if (!first) first = time;
            previous = time;
            open = true;
            continue;
        }
        if (!open) continue;
        for (std::size_t n = 0; n < count; ++n)
            if (segment == names[n]) {
                totals[n] += time > previous ? time - previous : 0;
                break;
            }
        previous = last = time;
    }
    if (!first || !last) return;
    char line[512];
    int used = std::snprintf(line, sizeof(line), "MAP_GPU frame=%llu", static_cast<unsigned long long>(frame));
    for (std::size_t n = 0; n < count && used > 0 && std::size_t(used) < sizeof(line); ++n)
        used += std::snprintf(line + used, sizeof(line) - std::size_t(used), " %s_us=%.1f", names[n],
                              double(totals[n]) / 1000.0);
    std::fprintf(stderr, "%s span_us=%.1f\n", line, double(last - first) / 1000.0);
}
bool RendererImpl::create_pages() {
    using RD = DeviceOperations;
    // Enough layers for every page a view of this size can touch, plus one
    // row and column of slack so scrolling back reuses pages.
    const int columns = (width_ + PageSize - 2) / PageSize + 2, rows = (height_ + PageSize - 2) / PageSize + 2;
    const auto layers = std::size_t(columns + 1) * std::size_t(rows + 1);
    if (pages_.size() == layers) return true;
    free_pages();
    if (!page_shader_.is_valid() && !create_render_shader(map_page_program, page_shader_)) return false;
    if (!resolve_shader_.is_valid() && !create_render_shader(map_resolve_program, resolve_shader_)) return false;
    const auto array = [&](RD::DataFormat data, std::uint64_t usage) {
        TextureSpec format;
        format->set_texture_type(RD::TEXTURE_TYPE_2D_ARRAY);
        format->set_width(PageSize);
        format->set_height(PageSize);
        format->set_array_layers(std::uint32_t(layers));
        format->set_format(data);
        format->set_usage_bits(usage);
        ++work.creations;
        return rd_->texture_create(format);
    };
    page_materials_ =
        array(RD::DATA_FORMAT_R32_UINT, RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT);
    page_shadows_ =
        array(RD::DATA_FORMAT_R8_UNORM, RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT);
    page_depths_ = array(RD::DATA_FORMAT_D32_SFLOAT,
                         RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT);
    if (!page_materials_.is_valid() || !page_shadows_.is_valid() || !page_depths_.is_valid()) return false;
    pages_.resize(layers);
    for (std::size_t layer = 0; layer < layers; ++layer) {
        auto& page = pages_[layer];
        page.material = rd_->texture_create_shared_from_slice(page_materials_, std::uint32_t(layer));
        page.shadow = rd_->texture_create_shared_from_slice(page_shadows_, std::uint32_t(layer));
        page.depth =
            rd_->texture_create_shared_from_slice(page_depths_, std::uint32_t(layer));
        std::vector<Handle> attachments;
        attachments.push_back(page.material);
        attachments.push_back(page.shadow);
        attachments.push_back(page.depth);
        page.framebuffer = rd_->framebuffer_create(attachments);
        if (!page.framebuffer.is_valid()) return false;
    }
    page_table_capacity_ = layers * 4;
    page_table_ = rd_->storage_buffer_create(std::uint32_t(page_table_capacity_));
    ++work.creations;
    if (!page_table_.is_valid()) return false;
    // Pipelines: per raster depth state, an opaque variant (material and a
    // zero shadow count) and a shadow variant (count + 1, material kept).
    const auto format = rd_->framebuffer_get_format(pages_[0].framebuffer);
    for (int variant = 0; variant < 10; ++variant) {
        if (page_pipelines_[variant].is_valid()) continue;
        const auto depth_state = static_cast<RasterDepth>(variant / 2);
        const bool shadow = variant % 2 != 0;
        DepthSpec depth;
        const bool write =
            depth_state == RasterDepth::less_write || depth_state == RasterDepth::less_equal_write;
        depth->set_enable_depth_test(depth_state != RasterDepth::none);
        depth->set_enable_depth_write(write);
        depth->set_depth_compare_operator(depth_state == RasterDepth::less_write ||
                                                  depth_state == RasterDepth::less
                                              ? RD::COMPARE_OP_LESS
                                              : RD::COMPARE_OP_LESS_OR_EQUAL);
        AttachmentSpec material;
        AttachmentSpec count;
        if (shadow) {
            material->set_write_r(false);
            material->set_write_g(false);
            material->set_write_b(false);
            material->set_write_a(false);
            count->set_enable_blend(true);
            count->set_src_color_blend_factor(RD::BLEND_FACTOR_ONE);
            count->set_dst_color_blend_factor(RD::BLEND_FACTOR_ONE);
            count->set_color_blend_op(RD::BLEND_OP_ADD);
            count->set_src_alpha_blend_factor(RD::BLEND_FACTOR_ONE);
            count->set_dst_alpha_blend_factor(RD::BLEND_FACTOR_ONE);
            count->set_alpha_blend_op(RD::BLEND_OP_ADD);
        }
        BlendSpec blend;
        std::vector<AttachmentSpec> attachments;
        attachments.push_back(material);
        attachments.push_back(count);
        blend->set_attachments(attachments);
        page_pipelines_[variant] =
            rd_->render_pipeline_create(page_shader_, format, depth, blend);
        if (!page_pipelines_[variant].is_valid()) return false;
    }
    if (!resolve_pipeline_.is_valid()) {
        DepthSpec depth;
        depth->set_enable_depth_test(true);
        depth->set_enable_depth_write(true);
        depth->set_depth_compare_operator(RD::COMPARE_OP_ALWAYS);
        AttachmentSpec color;
        BlendSpec blend;
        std::vector<AttachmentSpec> attachments;
        attachments.push_back(color);
        blend->set_attachments(attachments);
        resolve_pipeline_ = rd_->render_pipeline_create(
            resolve_shader_, rd_->framebuffer_get_format(raster_framebuffer_), depth, blend);
        if (!resolve_pipeline_.is_valid()) return false;
    }
    return true;
}
void RendererImpl::free_pages() {
    if (!rd_) return;
    for (Handle* rid : {&page_uniform_, &resolve_uniform_, &lit_tile_uniform_})
        if (rid->is_valid()) {
            rd_->free_rid(*rid);
            *rid = {};
        }
    // Framebuffers before the slices they name, slices before their arrays.
    for (auto& page : pages_)
        if (page.framebuffer.is_valid()) rd_->free_rid(page.framebuffer);
    for (auto& page : pages_)
        for (Handle* rid : {&page.material, &page.shadow, &page.depth})
            if (rid->is_valid()) rd_->free_rid(*rid);
    pages_.clear();
    for (Handle* rid : {&page_materials_, &page_shadows_, &page_depths_, &page_table_})
        if (rid->is_valid()) {
            rd_->free_rid(*rid);
            *rid = {};
        }
    page_table_capacity_ = 0;
}
bool RendererImpl::generate_page(PageSlot& slot, int x, int y, const MapSceneMark& mark,
                                   const std::vector<std::uint8_t>& prefix) {
    const int origin_x = x * PageSize, origin_y = y * PageSize;
    if (!page_uniform_.is_valid())
        page_uniform_ = binding_set(rd_, page_shader_,
                                    {{StorageBuffer, scene_records_},
                                     {SampledTexture, depth_sampler_, resource_pages_},
                                     {StorageBuffer, resource_metadata_},
                                     {StorageBuffer, scene_order_}});
    if (!page_uniform_.is_valid()) return false;
    Colors clears;
    clears.push_back(Color(0, 0, 0, 0));
    clears.push_back(Color(0, 0, 0, 0));
    const auto list = rd_->draw_list_begin(
        slot.framebuffer, DeviceOperations::DRAW_CLEAR_COLOR_ALL | DeviceOperations::DRAW_CLEAR_DEPTH, clears, 1.0f);
    if (list < 0) return false;
    int bound = -1;
    // Every instance of the page set, from the window around the viewport the core measured (it covers the
    // page: a page lies within BackgroundWindowMargin of the view). GPU clipping trims it to the page.
    for (const auto& range : mark.window) {
        if (!range.count || range.instance >= prefix.size() || !prefix[range.instance]) continue;
        const auto& state = scene_instance_state_[range.instance];
        const int variant =
            int(state.variant) / RasterBlends * 2 + (state.variant % RasterBlends == 1 ? 1 : 0);
        if (variant != bound) {
            bound = variant;
            rd_->draw_list_bind_render_pipeline(list, page_pipelines_[variant]);
            rd_->draw_list_bind_uniform_set(list, page_uniform_, 0);
        }
        const std::int32_t push[4]{int(state.base + range.first), origin_x, origin_y, PageSize};
        rd_->draw_list_set_push_constant(list, bytes(push, sizeof(push)), sizeof(push));
        rd_->draw_list_draw(list, false, range.count, 6);
        ++work.draws;
    }
    rd_->draw_list_end();
    slot.x = x;
    slot.y = y;
    slot.valid = true;
    ++pages_generated_;
    return true;
}
void RendererImpl::note_alphas(const MapSceneData& scene) {
    if (scene.alphas == light_alphas_) return;
    // Pages under every added, removed or changed AlphaShape are stale.
    const auto rect = [](const MapSceneAlpha& a) {
        const auto& b = a.image.bounds;
        return WorldRect{a.position.X + b.X, a.position.Y + b.Y, b.Width, b.Height};
    };
    std::unordered_map<std::uint64_t, const MapSceneAlpha*> previous;
    if (light_alphas_)
        for (const auto& a : *light_alphas_)
            previous.emplace(a.id, &a);
    std::unordered_set<std::uint64_t> kept;
    if (scene.alphas)
        for (const auto& a : *scene.alphas) {
            const auto found = previous.find(a.id);
            if (found != previous.end() && found->second->position.X == a.position.X &&
                found->second->position.Y == a.position.Y && found->second->image.source == a.image.source) {
                kept.insert(a.id);
                continue;
            }
            light_dirty_.push_back(rect(a));
        }
    if (light_alphas_)
        for (const auto& a : *light_alphas_)
            if (!kept.count(a.id)) light_dirty_.push_back(rect(a));
    // Immutable alpha versions share the same atlas and lifetime as sprites.
    if (scene.alphas)
        for (const auto& a : *scene.alphas)
            scene_resource_uploads_.push_back(a.image.source);
    light_alphas_ = scene.alphas;
    light_alpha_words_stale_ = true;
}
void RendererImpl::invalidate_lit(int x, int y, int width, int height) {
    if (lit_tiles_.empty() || width <= 0 || height <= 0) return;
    const auto floor_tile = [](int v) { return v >= 0 ? v / PageSize : -((-v + PageSize - 1) / PageSize); };
    const int last_x = floor_tile(x + width - 1), last_y = floor_tile(y + height - 1);
    for (int ty = floor_tile(y); ty <= last_y; ++ty)
        for (int tx = floor_tile(x); tx <= last_x; ++tx) {
            auto& tile = lit_tiles_[std::size_t(ty & (lit_rows_ - 1)) * std::size_t(lit_columns_) +
                                    std::size_t(tx & (lit_columns_ - 1))];
            if (tile.valid && tile.x == tx && tile.y == ty) {
                tile.valid = false;
                ++lit_invalidations_;
            }
        }
}
void RendererImpl::invalidate_lit_all() {
    for (auto& tile : lit_tiles_)
        if (tile.valid) {
            tile.valid = false;
            ++lit_invalidations_;
        }
}
bool RendererImpl::create_lit_ring() {
    if (lit_ring_.is_valid()) return true;
    using RD = DeviceOperations;
    // The tiles a window of the target size can touch, rounded up to a power of two so the ring address is
    // world & mask. A tile outside the view is only kept, never made ahead.
    const auto tiles = [](int pixels) {
        const int needed = (std::max(pixels, 1) + PageSize - 2) / PageSize + 1;
        int count = 1;
        while (count < needed) count *= 2;
        return count;
    };
    lit_columns_ = tiles(width_);
    lit_rows_ = tiles(height_);
    const std::uint64_t usage = RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
    if (!rd_->texture_is_format_supported_for_usage(RD::DATA_FORMAT_R32G32_UINT, usage)) {
        lit_columns_ = lit_rows_ = 0;
        return false;
    }
    if (!lit_tile_shader_.is_valid() && !create_render_shader(map_ring_tile_program, lit_tile_shader_)) return false;
    if (!lit_copy_shader_.is_valid() && !create_render_shader(map_ring_program, lit_copy_shader_)) return false;
    TextureSpec format;
    format->set_width(std::uint32_t(lit_columns_ * PageSize));
    format->set_height(std::uint32_t(lit_rows_ * PageSize));
    format->set_format(RD::DATA_FORMAT_R32G32_UINT);
    format->set_usage_bits(usage);
    lit_ring_ = rd_->texture_create(format);
    ++work.creations;
    if (!lit_ring_.is_valid()) return false;
    std::vector<Handle> attachments;
    attachments.push_back(lit_ring_);
    lit_framebuffer_ = rd_->framebuffer_create(attachments);
    if (!lit_framebuffer_.is_valid()) return false;
    if (!lit_tile_pipeline_.is_valid()) {
        // Integer color: no blending, no depth.
        DepthSpec depth;
        AttachmentSpec color;
        BlendSpec blend;
        std::vector<AttachmentSpec> blends;
        blends.push_back(color);
        blend->set_attachments(blends);
        lit_tile_pipeline_ = rd_->render_pipeline_create(lit_tile_shader_, rd_->framebuffer_get_format(lit_framebuffer_),
                                                         depth, blend);
        if (!lit_tile_pipeline_.is_valid()) return false;
    }
    if (!lit_copy_pipeline_.is_valid()) {
        // The frame's background write: depth always passes and is written.
        DepthSpec depth;
        depth->set_enable_depth_test(true);
        depth->set_enable_depth_write(true);
        depth->set_depth_compare_operator(RD::COMPARE_OP_ALWAYS);
        AttachmentSpec color;
        BlendSpec blend;
        std::vector<AttachmentSpec> blends;
        blends.push_back(color);
        blend->set_attachments(blends);
        lit_copy_pipeline_ = rd_->render_pipeline_create(lit_copy_shader_, rd_->framebuffer_get_format(raster_framebuffer_),
                                                         depth, blend);
        if (!lit_copy_pipeline_.is_valid()) return false;
    }
    lit_tiles_.assign(std::size_t(lit_columns_) * std::size_t(lit_rows_), {});
    return true;
}
bool RendererImpl::create_light_ring() {
    if (light_ring_.is_valid()) return true;
    using RD = DeviceOperations;
    // Tiles one window of the target size can touch (a tile more than the pixel count suggests, for any offset),
    // plus a tile of margin on each side, rounded up to a power of two so that the ring address is world & mask.
    const auto tiles = [](int pixels) {
        int needed = (std::max(pixels, 1) + PageSize - 2) / PageSize + 1 + 2, count = 1;
        while (count < needed) count *= 2;
        return count;
    };
    light_ring_columns_ = tiles(width_);
    light_ring_rows_ = tiles(height_);
    const std::uint64_t usage = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT;
    if (!rd_->texture_is_format_supported_for_usage(RD::DATA_FORMAT_R8_UINT, usage)) {
            error_ = "DeviceOperations lacks sampled R8 storage images";
        return false;
    }
    TextureSpec format;
    format->set_width(std::uint32_t(light_ring_columns_ * PageSize));
    format->set_height(std::uint32_t(light_ring_rows_ * PageSize));
    format->set_format(RD::DATA_FORMAT_R8_UINT);
    format->set_usage_bits(usage);
    light_ring_ = rd_->texture_create(format);
    ++work.creations;
    if (!light_ring_.is_valid()) return false;
    light_tiles_.assign(std::size_t(light_ring_columns_) * std::size_t(light_ring_rows_), {});
    light_all_dirty_ = true;
    return true;
}
bool RendererImpl::plan_light_ring(const MapLightField& field, std::int32_t field_base,
                                     std::vector<std::int32_t>& tables) {
    if (!resource_pages_.is_valid() || !create_light_ring()) return false;
    const int origin_x = field.origin.X + field.camera.X, origin_y = field.origin.Y + field.camera.Y;
    if (origin_x != light_origin_x_ || origin_y != light_origin_y_ || field.view.X != light_view_x_ ||
        field.image_version != light_image_version_) {
        light_origin_x_ = origin_x;
        light_origin_y_ = origin_y;
        light_view_x_ = field.view.X;
        light_image_version_ = field.image_version;
        light_all_dirty_ = true;
    }
    // A cell's frame is clipped to its 60x30 footprint.
    if (light_dirty_cells_.size() > 4096) light_all_dirty_ = true;
    if (!light_all_dirty_)
        for (const auto cell : light_dirty_cells_) {
            const int x = int(cell % 512), y = int(cell / 512);
            light_dirty_.push_back({origin_x + 30 * (x - y), origin_y + 15 * (x + y), 60, 30});
        }
    light_dirty_cells_.clear();
    const auto floor_page = [](int v) { return v >= 0 ? v / PageSize : -((-v + PageSize - 1) / PageSize); };
    const auto tile_slot = [&](int x, int y) -> RingTile& {
        return light_tiles_[std::size_t(y & (light_ring_rows_ - 1)) * std::size_t(light_ring_columns_) +
                            std::size_t(x & (light_ring_columns_ - 1))];
    };
    // The lit background ring holds lit colors made with this light: the same changes reach it.
    if (light_all_dirty_) {
        for (auto& tile : light_tiles_)
            tile.valid = false;
        invalidate_lit_all();
    } else
        for (const auto& r : light_dirty_) {
            if (r.width <= 0 || r.height <= 0) continue;
            invalidate_lit(r.x, r.y, r.width, r.height);
            const int last_x = floor_page(r.x + r.width - 1), last_y = floor_page(r.y + r.height - 1);
            for (int y = floor_page(r.y); y <= last_y; ++y)
                for (int x = floor_page(r.x); x <= last_x; ++x) {
                    auto& tile = tile_slot(x, y);
                    if (tile.valid && tile.x == x && tile.y == y) tile.valid = false;
                }
        }
    light_all_dirty_ = false;
    light_dirty_.clear();
    const int view_x = field.view.X + field.camera.X, view_y = field.view.Y + field.camera.Y;
    const int first_x = floor_page(view_x), first_y = floor_page(view_y);
    const int columns = std::max(field.view.Width, 1), rows = std::max(field.view.Height, 1);
    const int last_x = floor_page(view_x + columns - 1), last_y = floor_page(view_y + rows - 1);
    if (last_x - first_x + 1 > light_ring_columns_ || last_y - first_y + 1 > light_ring_rows_) return false;
    // Tables: generation entries, AlphaShape indexes, AlphaShapes.
    std::vector<std::int32_t> entries, indexes;
    for (int y = first_y; y <= last_y; ++y)
        for (int x = first_x; x <= last_x; ++x) {
            auto& tile = tile_slot(x, y);
            if (tile.valid && tile.x == x && tile.y == y) continue;
            tile = {x, y, true};
            const int left = x * PageSize, top = y * PageSize, first = int(indexes.size());
            for (std::size_t a = 0; a * 5 < light_alpha_words_.size(); ++a) {
                const auto* w = &light_alpha_words_[a * 5];
                if (w[1] < left + PageSize && w[2] < top + PageSize && w[1] + w[3] > left && w[2] + w[4] > top)
                    indexes.push_back(int(a));
            }
            entries.insert(entries.end(), {left, top, (x & (light_ring_columns_ - 1)) * PageSize,
                                           (y & (light_ring_rows_ - 1)) * PageSize, first,
                                           int(indexes.size()) - first});
        }
    const auto base = std::int32_t(tables.size());
    const auto entry_offset = base, index_offset = entry_offset + std::int32_t(entries.size());
    const auto alpha_offset = index_offset + std::int32_t(indexes.size());
    tables.insert(tables.end(), entries.begin(), entries.end());
    tables.insert(tables.end(), indexes.begin(), indexes.end());
    tables.insert(tables.end(), light_alpha_words_.begin(), light_alpha_words_.end());
    const int mask_x = light_ring_columns_ * PageSize - 1, mask_y = light_ring_rows_ * PageSize - 1;
    light_pushes_.push_back({0, field_base, origin_x, origin_y, field.view.X, entry_offset, index_offset, alpha_offset,
                             0, 0, 0, 0, 0, 0, 0, 0});
    light_pushes_.push_back({1, width_, height_, field.view.X, field.view.Y, field.view.Width, field.view.Height,
                             field.camera.X, field.camera.Y, mask_x, mask_y, 0, 0, 0, 0, 0});
    light_generated_.push_back(int(entries.size() / 6));
    light_pages_generated_ += entries.size() / 6;
    return true;
}

}
