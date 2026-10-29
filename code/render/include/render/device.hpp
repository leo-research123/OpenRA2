#pragma once
#include "render/frame.hpp"
#include "render/shader.hpp"
namespace render {
struct Handle {
    std::uint64_t value = 0;
    constexpr bool is_valid() const noexcept { return value != 0; }
    bool operator==(const Handle&) const = default;
};
struct Color { float r = 0, g = 0, b = 0, a = 1; };
struct Extent3 { std::uint32_t x = 0, y = 0, z = 0; };
enum class Format : std::uint32_t { rgba8, depth32, r32u, rg32u, r8, r8u };
enum TextureUsage : std::uint32_t {
    sampled = 1, storage = 2, color_attachment = 4, depth_attachment = 8,
    copy_source = 16, copy_destination = 32, storage_atomic = 64
};
struct TextureDesc {
    std::uint32_t width = 1, height = 1, layers = 1;
    Format format = Format::rgba8;
    std::uint32_t usage = 0;
    bool array = false;
};
enum class Compare : std::uint32_t { less, less_equal, always };
enum class BlendFactor : std::uint32_t { zero, one, constant_color };
struct DepthDesc { bool test = false, write = false; Compare compare = Compare::always; };
struct BlendAttachment {
    bool enabled = false, red = true, green = true, blue = true, alpha = true;
    BlendFactor source = BlendFactor::one, destination = BlendFactor::zero;
    BlendFactor source_alpha = BlendFactor::one, destination_alpha = BlendFactor::zero;
};
struct PipelineDesc {
    Handle shader{};
    std::uint64_t framebuffer_format = 0;
    DepthDesc depth{};
    const BlendAttachment* attachments = nullptr;
    std::uint32_t attachment_count = 0;
    bool dynamic_blend = false;
};
enum class BindingKind : std::uint32_t { buffer, image, sampled_texture };
struct Binding { BindingKind kind{}; Handle resource{}, texture{}; std::uint32_t index = 0; };
struct DrawListDesc {
    Handle framebuffer{};
    std::uint32_t clear_mask = 0;
    const Color* colors = nullptr;
    std::uint32_t color_count = 0;
    float depth = 1;
};
enum class CommandKind : std::uint32_t {
    end_compute, end_draw, compute_pipeline, draw_pipeline, compute_binding, draw_binding,
    compute_push, draw_push, dispatch, draw, barrier, blend_constant, timestamp
};
struct DeviceCommand {
    CommandKind kind{};
    std::int64_t list = 0;
    Handle resource{};
    std::uint32_t x = 0, y = 0, z = 0;
    const void* data = nullptr;
    std::uint32_t size = 0;
    Color color{};
};
struct Timestamp { std::uint64_t gpu_time = 0; char name[96]{}; };
// The host executes these calls on its render thread. Pointers are borrowed
// through return; callbacks must never propagate exceptions across this boundary.
struct Device {
    std::uint32_t struct_size = sizeof(Device), version = 1;
    void* context = nullptr;
    Status (*create_buffer)(void*, std::uint64_t, const void*, std::uint64_t, Handle&) noexcept = nullptr;
    Status (*update_buffer)(void*, Handle, std::uint64_t, const void*, std::uint64_t) noexcept = nullptr;
    Status (*copy_buffer)(void*, Handle, Handle, std::uint64_t, std::uint64_t, std::uint64_t) noexcept = nullptr;
    Status (*clear_buffer)(void*, Handle, std::uint64_t, std::uint64_t) noexcept = nullptr;
    Status (*create_texture)(void*, const TextureDesc&, Handle&) noexcept = nullptr;
    bool (*supports_texture)(void*, Format, std::uint32_t) noexcept = nullptr;
    Status (*texture_slice)(void*, Handle, std::uint32_t, Handle&) noexcept = nullptr;
    Status (*copy_texture)(void*, Handle, Handle, Extent3, Extent3, Extent3, std::uint32_t, std::uint32_t) noexcept = nullptr;
    Status (*read_texture)(void*, Handle, std::uint32_t, void*, std::uint64_t, std::uint64_t&) noexcept = nullptr;
    std::uint32_t (*max_texture_layers)(void*) noexcept = nullptr;
    Status (*create_framebuffer)(void*, const Handle*, std::uint32_t, Handle&) noexcept = nullptr;
    std::uint64_t (*framebuffer_format)(void*, Handle) noexcept = nullptr;
    Status (*create_shader)(void*, const ShaderProgram&, Handle&) noexcept = nullptr;
    Status (*create_pipeline)(void*, const PipelineDesc&, Handle&) noexcept = nullptr;
    Status (*create_compute_pipeline)(void*, Handle, Handle&) noexcept = nullptr;
    Status (*create_sampler)(void*, Handle&) noexcept = nullptr;
    Status (*create_bindings)(void*, Handle, const Binding*, std::uint32_t, Handle&) noexcept = nullptr;
    bool (*bindings_valid)(void*, Handle) noexcept = nullptr;
    bool (*texture_valid)(void*, Handle) noexcept = nullptr;
    bool (*framebuffer_valid)(void*, Handle) noexcept = nullptr;
    Status (*begin_compute)(void*, std::int64_t&) noexcept = nullptr;
    Status (*begin_draw)(void*, const DrawListDesc&, std::int64_t&) noexcept = nullptr;
    Status (*record)(void*, const DeviceCommand&) noexcept = nullptr;
    void (*release)(void*, Handle) noexcept = nullptr;
    std::uint64_t (*timestamp_frame)(void*) noexcept = nullptr;
    std::uint32_t (*timestamp_count)(void*) noexcept = nullptr;
    Status (*timestamp)(void*, std::uint32_t, Timestamp&) noexcept = nullptr;
};
}
