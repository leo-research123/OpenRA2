#pragma once
#include "frame_storage.hpp"
namespace render {
int append_lighting_bins(const PixelParameters*, int, int, int, int, std::vector<std::int32_t>&);
class FrameCompiler {
public:
    void compile(MapFrameData&);
    static Status describe(FramePayload&, Frame&) noexcept;
    static Status import(const Frame&, MapFrameData&) noexcept;
private:
    PixelRunScratch scratch_;
};
}
