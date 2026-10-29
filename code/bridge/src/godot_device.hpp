#pragma once
#include "render/device.hpp"
#include <godot_cpp/classes/rendering_device.hpp>
#include <unordered_map>
namespace game {
// Private host adapter. Every method is confined to the RenderingServer thread.
class GodotDevice {
public:
    render::Device interface() noexcept;
    godot::RID rid(render::Handle) const;
private:
    godot::RenderingDevice* rd_ = nullptr;
    enum class ResourceKind { ordinary, texture, framebuffer, bindings };
    struct Resource { godot::RID rid; ResourceKind kind; };
    std::unordered_map<std::uint64_t, Resource> resources_;
    render::Status retain(godot::RID, render::Handle&, ResourceKind = ResourceKind::ordinary);
};
}
