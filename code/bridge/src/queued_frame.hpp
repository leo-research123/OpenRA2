#pragma once
#include "api/presentation.hpp"
#include "api/runtime_diagnostics.hpp"
namespace game {
struct QueuedFrame {
    PresentationFrameHandle* owner = nullptr;
    render::Frame input{};
    MapDrawStatistics stats{};
    RenderFailureContext diagnostic{};
    ~QueuedFrame() { release_presentation_frame(owner); }
    QueuedFrame() = default;
    QueuedFrame(const QueuedFrame&) = delete;
    QueuedFrame& operator=(const QueuedFrame&) = delete;
};
}
