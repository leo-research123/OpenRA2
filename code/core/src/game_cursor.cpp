#include "map_view.hpp"
#include "api/images.hpp"
#include "yrpp/WWMouseClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/FileSystem.h"
#include "yrpp/FileFormats/SHP.h"
#include <vector>
#include <string>

namespace game {
bool read_game_cursor(MapViewHandle& view, GameCursorFrame& output, std::uint8_t* rgba,
                      std::uint32_t capacity) noexcept {
    output = {};
    if ((!rgba && capacity) || view.state != MapViewState::ready) return false;
    struct Query {
        MapViewHandle& view;
        GameCursorFrame& out;
        std::uint8_t* pixels;
        std::uint32_t capacity;
        bool ok = false;
    } q{view, output, rgba, capacity};
    const bool scoped = with_map_view(
        view,
        [](void* data) {
            auto& q = *static_cast<Query*>(data);
            auto* driver = WWMouseClass::Instance;
            if (!driver || !driver->Image || !FileSystem::MOUSE_PAL || !FileSystem::MOUSE_PAL->PaletteData) return;
            auto* shape = driver->Image;
            const int frame = driver->ImageFrameIndex;
            if (frame < 0 || frame >= shape->Frames) return;
            const auto bounds = shape->GetFrameBounds(frame);
            if (bounds.Width <= 0 || bounds.Height <= 0 || bounds.Width > 4096 || bounds.Height > 4096) return;
            auto& out = q.out;
            out.visible =
                q.view.pointer_inside && Game::IsFocused && driver->IsCaptured() && !driver->IsRefCountNegative();
            out.x = q.view.pointer.X;
            out.y = q.view.pointer.Y;
            out.width = bounds.Width;
            out.height = bounds.Height;
            out.hotspot_x = driver->XY2.X - bounds.X;
            out.hotspot_y = driver->XY2.Y - bounds.Y;
            out.index = int(MouseClass::Instance.MouseCursorIndex);
            out.frame = frame;
            auto* palette = static_cast<const WORD*>(FileSystem::MOUSE_PAL->PaletteData);
            std::uint64_t hash = 14695981039346656037ull;
            const auto append = [&](unsigned value) { hash = (hash ^ value) * 1099511628211ull; };
            append(unsigned(frame));
            append(unsigned(out.width));
            append(unsigned(out.height));
            for (int i = 0; i < 256; ++i)
                append(palette[i]);
            out.revision = hash;
            if (!q.pixels) {
                q.ok = true;
                return;
            }
            if (std::uint64_t(out.width) * out.height * 4 > q.capacity) return;
            std::vector<std::uint8_t> indices;
            std::string error;
            if (!decode_shp_pixels(shape->GetPixels(frame), out.width, out.height, shape->HasCompression(frame),
                                   indices, error))
                return;
            const int left[]{Drawing::RedShiftLeft, Drawing::GreenShiftLeft, Drawing::BlueShiftLeft};
            const int right[]{Drawing::RedShiftRight, Drawing::GreenShiftRight, Drawing::BlueShiftRight};
            for (std::size_t i = 0; i < indices.size(); ++i) {
                const auto index = indices[i];
                const auto color = palette[index];
                for (int channel = 0; channel < 3; ++channel) {
                    const unsigned maximum = (1u << (8 - right[channel])) - 1u;
                    const unsigned value = (color >> left[channel]) & maximum;
                    q.pixels[4 * i + channel] = std::uint8_t((value * 255u + maximum / 2u) / maximum);
                }
                q.pixels[4 * i + 3] = index ? 255 : 0;
            }
            q.ok = true;
        },
        &q);
    return scoped && q.ok;
}
}
