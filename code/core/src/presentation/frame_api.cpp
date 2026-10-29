#include "api/presentation.hpp"
#include "presentation/frame_compiler.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <thread>
#include <functional>
namespace game {
struct PresentationFrameHandle;
struct PresentationCompilerState {
    explicit PresentationCompilerState(bool reference) : compiler(reference) {}
    FrameCompiler compiler;
    PresentationFrameHandle* active = nullptr;
};
struct PresentationHandle {
    explicit PresentationHandle(bool reference) : state(std::make_shared<PresentationCompilerState>(reference)) {}
    std::shared_ptr<PresentationCompilerState> state;
};
struct PresentationFrameHandle {
    std::shared_ptr<PresentationCompilerState> state;
    std::shared_ptr<MapFrameData> data;
    MapDrawingContext drawing;
    bool sealed = false;
};
bool create_presentation(PresentationHandle*& output, bool software_reference) noexcept {
    if (output) return false;
    try { output = new PresentationHandle(software_reference); return true; } catch (...) { return false; }
}
void destroy_presentation(PresentationHandle* handle) noexcept { delete handle; }
void clear_presentation(PresentationHandle& handle) noexcept {
    handle.state->active = nullptr;
    try { handle.state->compiler.clear(); } catch (...) {}
}
bool begin_presentation_frame(PresentationHandle& handle, std::uint64_t generation, int width, int height,
                               PresentationFrameHandle*& output) noexcept {
    if (output || handle.state->active || width <= 0 || height <= 0 || width > 8192 || height > 8192) return false;
    try {
        auto frame = std::make_unique<PresentationFrameHandle>();
        frame->state = handle.state;
        frame->data = handle.state->compiler.acquire_frame();
        frame->data->generation = generation;
        frame->data->width = width;
        frame->data->height = height;
        frame->drawing = handle.state->compiler.begin_frame(*frame->data);
        handle.state->active = frame.get();
        output = frame.release();
        return true;
    } catch (...) { return false; }
}
DrawingStatus advance_presentation_frame(MapViewHandle& view, PresentationFrameHandle& frame, double seconds,
                                          MapDrawStatistics& stats, bool& rendered) noexcept {
    stats={}; rendered=false;
    if (frame.sealed || frame.state->active != &frame) return DrawingStatus::invalid_argument;
    if (!set_game_view_size(view, frame.data->width, frame.data->height)) return DrawingStatus::invalid_argument;
    return advance_game_view_batch(view, seconds, frame.drawing, stats, rendered);
}
bool end_presentation_frame(PresentationHandle& handle, PresentationFrameHandle& frame, render::Frame& output) noexcept {
    output = {};
    if (frame.sealed || frame.state != handle.state || handle.state->active != &frame) return false;
    frame.sealed = true;
    handle.state->active = nullptr;
    if (!handle.state->compiler.end_frame()) return false;
    return render::FrameCompiler::describe(*frame.data, output) == render::Status::ok;
}
void release_presentation_frame(PresentationFrameHandle* frame) noexcept {
    if (!frame) return;
    if (!frame->sealed && frame->state->active == frame) {
        frame->state->active = nullptr;
        try { frame->state->compiler.clear(); } catch (...) {}
    }
    if (frame->sealed) {
        static const bool diagnostics = [] {
            const auto* value = std::getenv("RA2_RESOURCE_STATS");
            return value && value[0] && std::strcmp(value, "0") != 0;
        }();
        const auto started = diagnostics ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        const auto retired = frame->data->retired_sequences.size();
        const auto resources = frame->data->resources.size();
        frame->data->resources.clear();
        frame->data->sequence_resources.clear();
        frame->data->scenes.clear();
        frame->data->sequences.reset();
        frame->data->retired_sequences.clear();
        if (diagnostics && (retired || frame->data->serial % 256 == 0))
            std::fprintf(stderr, "RESOURCE_RELEASE frame=%llu thread=%zu resources=%zu retired_sets=%zu elapsed_us=%.3f\n",
                (unsigned long long)frame->data->serial, std::hash<std::thread::id>{}(std::this_thread::get_id()),
                resources, retired, std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - started).count());
    }
    delete frame;
}
const char* presentation_error(const PresentationHandle& handle) noexcept { return handle.state->compiler.error(); }
bool presentation_statistics(const PresentationHandle& h, PresentationStatistics& output) noexcept {
    const auto& c=h.state->compiler;
    output = {c.decoded_tiles(), c.prepared_palettes(), c.decoded_lighting(), c.shape_sequences()};
    return true;
}
}
