#pragma once
#include "../../../experiments/sprite_draw_batch.hpp"
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/rid.hpp>

// Godot-only adapter. It knows rectangles, atlas UVs and colors, not ball motion.
class SpriteBatchRenderer {
public:
    SpriteBatchRenderer() = default;
    SpriteBatchRenderer(const SpriteBatchRenderer&) = delete;
    SpriteBatchRenderer& operator=(const SpriteBatchRenderer&) = delete;
    // The texture owner must keep it alive until shutdown().
    bool initialize(const godot::RID& parent, const godot::RID& texture, int capacity,
        const godot::RID& palette = {});
    bool submit(ra2::experiments::SpriteDrawBatch batch);
    void shutdown();
    ~SpriteBatchRenderer() { shutdown(); }

private:
    void allocate(int capacity);
    godot::RID canvas_, multimesh_, shader_, material_;
    godot::Ref<godot::QuadMesh> quad_;
    godot::PackedFloat32Array buffer_;
    int capacity_ = 0;
    int visible_ = 0;
};
