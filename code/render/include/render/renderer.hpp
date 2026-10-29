#pragma once
#include "render/device.hpp"
namespace render {
struct RendererHandle;
// Device context outlives the renderer. Calls are serialized on the render thread.
// A submitted frame is borrowed through return; persistent snapshots are retained
// internally. No exception may cross these entry points.
Status create(const Device&, RendererHandle*&) noexcept;
Status submit(RendererHandle&, const Frame&, Handle& texture) noexcept;
void destroy(RendererHandle*) noexcept;
const char* error(const RendererHandle&) noexcept;
const char* failure_stage(const RendererHandle&) noexcept;
}
