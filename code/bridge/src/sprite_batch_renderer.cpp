#include "bridge/sprite_batch_renderer.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <algorithm>
#include <limits>

bool SpriteBatchRenderer::initialize(const godot::RID& parent, const godot::RID& texture, int capacity,
    const godot::RID& palette) {
    if (!parent.is_valid() || !texture.is_valid() || capacity < 1 ||
        capacity > std::numeric_limits<int>::max() / 16) return false;
    shutdown();
    auto* server = godot::RenderingServer::get_singleton();
    quad_.instantiate();
    quad_->set_size(godot::Vector2(1, 1));
    canvas_ = server->canvas_item_create();
    multimesh_ = server->multimesh_create();
    shader_ = server->shader_create();
    material_ = server->material_create();
    server->shader_set_code(shader_, palette.is_valid() ? R"(
shader_type canvas_item;
render_mode unshaded;
uniform sampler2D palette_texture : filter_nearest, repeat_disable;
varying vec4 instance_color;
void vertex() {
    // QuadMesh UVs use +Y up; CanvasItem and image rows use +Y down.
    UV = INSTANCE_CUSTOM.xy + vec2(UV.x, 1.0 - UV.y) * INSTANCE_CUSTOM.zw;
    instance_color = COLOR;
}
void fragment() {
    int index = int(round(texture(TEXTURE, UV).r * 255.0));
    COLOR = index == 0 ? vec4(0.0) : texelFetch(palette_texture, ivec2(index, 0), 0) * instance_color;
}
)" : R"(
shader_type canvas_item;
render_mode unshaded;
void vertex() {
    UV = INSTANCE_CUSTOM.xy + vec2(UV.x, 1.0 - UV.y) * INSTANCE_CUSTOM.zw;
}
)");
    server->material_set_shader(material_, shader_);
    if (palette.is_valid()) server->material_set_param(material_, "palette_texture", palette);
    server->canvas_item_set_material(canvas_, material_);
    server->canvas_item_set_parent(canvas_, parent);
    server->canvas_item_set_default_texture_filter(canvas_, palette.is_valid()
        ? godot::RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST
        : godot::RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);
    server->multimesh_set_mesh(multimesh_, quad_->get_rid());
    allocate(capacity);
    // Persistent drawing command. Only instance values change during normal frames.
    server->canvas_item_add_multimesh(canvas_, multimesh_, texture);
    return true;
}

void SpriteBatchRenderer::allocate(int capacity) {
    auto* server = godot::RenderingServer::get_singleton();
    capacity_ = capacity;
    buffer_.resize(static_cast<int64_t>(capacity) * 16);
    server->multimesh_allocate_data(multimesh_, capacity, godot::RenderingServer::MULTIMESH_TRANSFORM_2D, true, true);
    server->multimesh_set_visible_instances(multimesh_, 0);
    visible_ = 0;
}

bool SpriteBatchRenderer::submit(ra2::experiments::SpriteDrawBatch batch) {
    if (!multimesh_.is_valid() || batch.instances.size() > std::numeric_limits<int>::max() / 32) return false;
    const int count = static_cast<int>(batch.instances.size());
    if (count > capacity_) allocate(std::max(count, capacity_ * 2));
    auto* server = godot::RenderingServer::get_singleton();
    if (count > 0) {
        float* out = buffer_.ptrw();
        float left = std::numeric_limits<float>::max(), top = left;
        float right = std::numeric_limits<float>::lowest(), bottom = right;
        for (const auto& sprite : batch.instances) {
            // Godot's padded Transform2D rows, followed by color and custom UV rect.
            const float values[16] = {
                sprite.width, 0, 0, sprite.x + sprite.width * 0.5f,
                0, sprite.height, 0, sprite.y + sprite.height * 0.5f,
                sprite.red, sprite.green, sprite.blue, sprite.alpha,
                sprite.u, sprite.v, sprite.uv_width, sprite.uv_height,
            };
            std::copy_n(values, 16, out);
            out += 16;
            left = std::min(left, sprite.x); top = std::min(top, sprite.y);
            right = std::max(right, sprite.x + sprite.width);
            bottom = std::max(bottom, sprite.y + sprite.height);
        }
        server->multimesh_set_custom_aabb(multimesh_, godot::AABB(
            godot::Vector3(left, top, 0), godot::Vector3(right - left, bottom - top, 1)));
        // CanvasItem also caches its 2D command bounds. Update them explicitly
        // so a moved batch can re-enter a resized viewport after being culled.
        server->canvas_item_set_custom_rect(canvas_, true,
            godot::Rect2(left, top, right - left, bottom - top));
        server->multimesh_set_buffer(multimesh_, buffer_);
    }
    if (visible_ != count) {
        server->multimesh_set_visible_instances(multimesh_, count);
        visible_ = count;
    }
    return true;
}

void SpriteBatchRenderer::shutdown() {
    if (auto* server = godot::RenderingServer::get_singleton()) {
        for (auto rid : {canvas_, multimesh_, material_, shader_})
            if (rid.is_valid()) server->free_rid(rid);
    }
    canvas_ = multimesh_ = material_ = shader_ = godot::RID();
    quad_.unref();
    buffer_ = godot::PackedFloat32Array();
    capacity_ = visible_ = 0;
}
