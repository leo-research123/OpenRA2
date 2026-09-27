#pragma once
#include "yrpp/FileSystem.h"
#include "yrpp/BitFont.h"
#include <array>
#include <memory>

namespace game {
// Resource ownership for the original UI helpers (72FA10/6A5840/6D02B0).
// No layout, input, player or production state is stored here.
enum class UiImage : unsigned {
    credits, top, radar, side1, side2, side2b, side3, addon,
    spacer, leftcap, button_background, rightcap, repair, sell, down, up,
    briefing, options, tab0, tab1, tab2, tab3, power, clock, button0, count=button0+25
};
class UiResources {
public:
    ~UiResources() { clear(); }
    bool load(int side) noexcept;
    void clear() noexcept;
    SHPStruct* image(UiImage id) const noexcept { return images_[static_cast<unsigned>(id)].get(); }
    BitFont* font() const noexcept { return font_.get(); }
    const BytePalette& palette() const noexcept { return palette_; }
    const BytePalette& cameo_palette() const noexcept { return cameo_palette_; }
    const char* error() const noexcept { return error_; }
private:
    std::array<std::unique_ptr<SHPReference>,static_cast<unsigned>(UiImage::count)> images_{};
    BytePalette palette_{};
    BytePalette cameo_palette_{};
    std::unique_ptr<BitFont> font_;
    int side_=-1;
    char error_[128]{};
};
}
