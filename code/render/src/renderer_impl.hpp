#pragma once
#include "device_operations.hpp"
#include "frame_storage.hpp"
#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace render {
class RendererImpl {
public:
    explicit RendererImpl(const Device& device):operations_(device) {}
    bool render(const MapFrameData&);
    void clear();
    void verify(const MapFrameData& frame) {
        if (!scene_pages_checked_ && !frame.scenes.empty() && std::getenv("RA2_VERIFY_SCENE_PAGES")) {
            scene_pages_checked_=true; verify_scene_pages(frame);
        }
    }
    Handle output() const { return colors_; }
    const char* error() const { return error_.c_str(); }
    const char* failure_stage() const { return failure_stage_; }
private:
    std::string error_;
    const char* failure_stage_ = "gpu_prepare";
    void verify_scene_pages(const MapFrameData&);
    bool bypass_pages_ = false, scene_pages_checked_ = false;
    bool create_pipeline(const ShaderProgram&, Handle&, Handle&);
    bool create_render_shader(const ShaderProgram&, Handle&);
    bool create_raster_pipelines();
    bool prepare_direct_resources(const MapFrameData&);
    bool create_direct_uniforms();
    Handle resource_pages_, resource_metadata_, direct_instances_, direct_compute_, direct_schedule_;
    Handle direct_shader_, direct_pipelines_[25], direct_uniform_;
    Handle direct_compute_shader_, direct_compute_pipeline_, direct_compute_uniform_;
    Handle direct_lighting_shader_, direct_lighting_pipeline_, direct_lighting_uniform_, direct_light_bins_;
    std::size_t direct_light_bin_capacity_ = 0;
    std::size_t resource_metadata_capacity_ = 0, direct_instance_capacity_ = 0, direct_compute_capacity_ = 0,
                direct_schedule_capacity_ = 0;
    Handle resource_upload_shader_, resource_upload_pipeline_, resource_upload_buffer_, resource_upload_uniform_;
    std::size_t resource_upload_capacity_ = 0;
    std::vector<const MapBufferData*> resource_uploads_;
    std::vector<std::uint32_t> resource_upload_words_;
    std::vector<std::uint64_t> resource_versions_;
    std::vector<std::array<std::int32_t, 4>> resource_metadata_words_;
    std::uint32_t resource_layers_ = 0;
    std::uint64_t sequence_revision_ = 0;  // last shape sequence set prepared (MapSequenceSet::revision)
    bool prepare_device(const MapFrameData&);
    void free_states();
    void free_uniforms();
    bool upload(Handle& buffer, std::size_t& capacity, const void* data, std::size_t size);
    bool create_uniforms();
    bool generate_voxels();
    std::vector<const MapBufferData*> voxel_jobs_;
    Handle voxel_models_;
    std::size_t voxel_model_capacity_ = 0;
    std::vector<std::uint64_t> voxel_model_versions_;
    std::vector<const MapBufferData*> voxel_model_uploads_;
    Handle voxel_shader_, voxel_pipeline_, voxel_uniform_, voxel_words_, voxel_projection_, voxel_depth_;
    std::size_t voxel_word_capacity_ = 0, voxel_projection_capacity_ = 0, voxel_depth_capacity_ = 0;
    bool prepare_scene(const MapSceneData&, std::size_t&);
    void free_scene();
    // Resident background: every instance of the core's TerrainSpriteLayer keeps its slots in one record
    // buffer (48-byte layout of map_raster.vert/.frag's resident path). An instance starts at `base` (slot 0 is
    // reserved: record id 0 means no pixel on the material pages), so the global slot is also the record id the
    // pages name. A slot nothing was written to is zero and draws nothing.
    struct SceneInstanceState {
        std::uint32_t base = 0, slots = 0, variant = 0;
        // Records of the instance whose pixels depend on the camera (scene_page_cacheable).
        std::uint32_t uncacheable = 0;
        // Lit, non-shadow records that take their light from the first visible row (metadata 0x200): the lit
        // color of their pixels follows the camera, so the lit background ring cannot hold them.
        std::uint32_t first_row = 0;
    };
    std::vector<SceneInstanceState> scene_instance_state_;
    std::shared_ptr<const MapSceneSlots> scene_slot_snapshot_;
    // Draw orders (MapSceneOrder) as record ids, at the same base as the records: entry base + i is the record id
    // of the instance's i-th occupied slot. Draws read their records through it (map_raster.vert/.frag, map_page.vert/.frag).
    std::shared_ptr<const MapSceneOrder> scene_order_snapshot_;
    Handle scene_order_;
    std::size_t scene_order_capacity_ = 0;
    std::vector<std::shared_ptr<MapBufferData>> scene_resource_uploads_;
    Handle scene_records_;
    std::size_t scene_record_capacity_ = 0;
    std::uint32_t scene_slot_total_ = 1;
    int scene_cache_height_ = -1;  // viewport height scene_page_cacheable assumed for the counts
    // Instances whose records the material pages hold: the leading instances of the drawing order that have no
    // camera dependent record. Pages are invalid whenever this set changes.
    std::vector<std::uint32_t> scene_page_prefix_;
    // World rectangles whose resident pixels changed since the pages were made.
    struct WorldRect {
        int x = 0, y = 0, width = 0, height = 0;
    };
    std::vector<WorldRect> scene_dirty_;
    // Map_raster.glsl resident draws (a scene the pages do not support): every
    // visible row, merged into ranges per variant.
    Handle resident_uniform_;
    // Material pages (map_page.vert/.frag) and their resolve (map_resolve.vert/.frag).
    static constexpr int PageSize = 256;
    struct PageSlot {
        int x = 0, y = 0;
        bool valid = false;
        std::uint64_t used = 0;
        Handle material, shadow, depth, framebuffer;
    };
    std::vector<PageSlot> pages_;
    Handle page_materials_, page_shadows_, page_depths_, page_table_;
    std::size_t page_table_capacity_ = 0;
    Handle page_shader_, page_pipelines_[10], resolve_shader_, resolve_pipeline_, page_uniform_, resolve_uniform_;
    std::uint64_t page_frame_ = 0;
    std::size_t pages_generated_ = 0;
    bool create_pages();
    void free_pages();
    // Draws the page at page coordinates (x, y) into slot; no list is open.
    bool generate_page(PageSlot& slot, int x, int y, const MapSceneMark&,
                       const std::vector<std::uint8_t>& prefix);
    // Lit background ring (map_resolve.vert/.frag compiled with RING_TILE, map_ring.vert/.frag): the lit color and world Z of
    // the paged background in a world-anchored toroidal RG32UI texture of PageSize x PageSize tiles, one tile per
    // material page. A tile is made when it enters the view without a valid copy and stays valid until a change
    // reaches one of its lit pixels: a record under it, a cell light, a converter palette or a light ring tile
    // (all pushed as world rectangles; see invalidate_lit). A frame then copies the ring to the frame color and
    // depth instead of resolving every pixel from the pages.
    struct LitTile {
        int x = 0, y = 0;  // world tile held by this ring slot
        bool valid = false;
    };
    std::vector<LitTile> lit_tiles_;
    int lit_columns_ = 0, lit_rows_ = 0;
    Handle lit_ring_, lit_framebuffer_, lit_tile_shader_, lit_tile_pipeline_, lit_copy_shader_, lit_copy_pipeline_,
        lit_tile_uniform_, lit_copy_uniform_;
    bool lit_enabled_ = true;
    // RA2_MAP_PROFILE: this frame's path (0 pages resolved directly, 1 ring), tiles made and visible tiles, and why
    // the direct path ran.
    int lit_path_ = 0;
    const char* lit_reason_ = "none";
    std::size_t lit_generated_ = 0, lit_visible_ = 0;
    // Tiles invalidated so far, and the count when the last paged frame planned its ring tiles.
    std::uint64_t lit_invalidations_ = 0, lit_invalidations_seen_ = 0;
    std::size_t lit_light_cells_ = 0;  // cells with records whose light changed since the last profile line
    bool create_lit_ring();
    void invalidate_lit(int x, int y, int width, int height);
    void invalidate_lit_all();
    // World bounds of every record that reads the light of a cell, and of every record that uses a converter slot
    // (only growing until the scene is reset): the rectangles a cell light or converter change invalidates.
    std::unordered_map<std::uint32_t, std::array<std::int32_t, 4>> cell_bounds_;
    std::vector<std::array<std::int32_t, 4>> converter_bounds_;
    std::vector<std::uint64_t> converter_versions_;
    Handle frame_depth_;
    // Cell lights (MapSceneLightChunk words) and converter slots (arena word
    // offsets) of the resident scene; both bound to every row.
    Handle scene_lights_, scene_slots_;
    std::size_t scene_light_capacity_ = 0, scene_slot_capacity_ = 0;
    std::shared_ptr<const MapSceneLights> scene_light_sources_;
    // SHROUD.SHP frame of every cell index (512 x 512, -1 none) for world
    // light fields; device-sized, rewritten per changed chunk.
    Handle scene_shroud_;
    std::shared_ptr<const MapSceneShroud> scene_shroud_sources_;
    bool scene_shroud_clear_ = true;
    // Light ring (map_light_page.comp): the world shroud light field in a world-anchored toroidal R8 texture,
    // one byte per pixel at (world & mask). The ring is made of PageSize x PageSize tiles (a power of two of
    // them in each direction, enough for the view plus a tile of margin on each side); a tile is generated when
    // it enters the view without a valid copy and stays valid until a shroud cell or AlphaShape overlapping it
    // changes. World fields expand the ring under the view into the ABuffer instead of evaluating every cell.
    struct RingTile {
        int x = 0, y = 0;  // world tile held by this ring slot
        bool valid = false;
    };
    std::vector<RingTile> light_tiles_;
    int light_ring_columns_ = 0, light_ring_rows_ = 0;
    Handle light_page_shader_, light_page_pipeline_, light_page_uniform_, light_ring_, light_tables_;
    std::size_t light_table_capacity_ = 0, light_pages_generated_ = 0;
    // Frames last uploaded per cell index, and the cells changed since the
    // ring was checked; light_all_dirty_ invalidates every tile.
    std::vector<std::int32_t> shroud_mirror_;
    std::vector<std::uint32_t> light_dirty_cells_;
    std::vector<WorldRect> light_dirty_;
    bool light_all_dirty_ = true;
    // Footprint origin (world) and view x the ring tiles were made with.
    int light_origin_x_ = 0, light_origin_y_ = 0, light_view_x_ = 0;
    std::uint64_t light_image_version_ = 0;
    // The scene AlphaShapes the ring holds, their scene arena references and
    // table words (arena offset, world x, y, width, height).
    std::shared_ptr<const std::vector<MapSceneAlpha>> light_alphas_;
    std::vector<std::int32_t> light_alpha_words_;
    bool light_alpha_words_stale_ = true;
    // Generation and expansion push constants of each world field this frame.
    std::vector<std::array<std::int32_t, 16>> light_pushes_;
    std::vector<int> light_generated_;
    void note_alphas(const MapSceneData&);
    bool plan_light_ring(const MapLightField&, std::int32_t field_base, std::vector<std::int32_t>& tables);
    bool create_light_ring();
    std::shared_ptr<const std::vector<std::shared_ptr<MapBufferData>>> scene_slot_sources_;
    std::uint64_t scene_epoch_ = 0, scene_revision_ = 0;
    // Device state is confined to the host render thread.
    DeviceOperations operations_;
    DeviceOperations* rd_ = &operations_;
    // map_draw, map_abuffer_batch, map_light_field and map_present.
    Handle light_field_shader_, light_field_pipeline_;
    Handle present_shader_, present_pipeline_;
    // map_raster (one pipeline per raster_batch.hpp variant).
    // Nearest sampler through which compute paths read scene Z.
    Handle depth_sampler_;
    // Color is RGBA8 and the published texture; D32F is the sole scene Z.
    // Compute paths only read Z.
    Handle colors_, raster_depth_, lights_, screen_snapshot_;
    // Color + D32F scene Z for map_raster.
    Handle raster_framebuffer_;
    // The Composite kept across the Chrono new-world capture pass.
    Handle kept_composite_;
    // Tables of this frame's light fields (map_light_field.comp).
    Handle light_field_words_;
    std::size_t light_field_capacity_ = 0;
    Handle light_field_uniform_, present_uniform_;
    std::uint64_t screen_capture_id_ = 0;
    std::uint64_t generation_ = 0;
    int width_ = 0, height_ = 0;
    // RA2_MAP_PROFILE diagnostics: GPU timestamps named "ra2:<segment>", each
    // closing the segment recorded since the previous one.
    bool profiling_ = false;
    unsigned timestamps_ = 0;
    std::uint64_t timing_frame_ = 0;
    void timestamp(const char* segment);
    void report_gpu_timing();

};
}
