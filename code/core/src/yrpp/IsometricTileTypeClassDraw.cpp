// Diamond traversal derived from XCC 70358b46858973426c1ecf204485cb2a88716217
// misc/tmp_ts_file.cpp (Olaf van der Spek, GPL-3.0-or-later). YR-specific
// clipping, variants, Z/Alpha and flags calibrated to 547CF0. One scalar
// implementation is the optional software backend, not a core prerequisite.
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include "type_drawing_software.hpp"
#include <algorithm>
#include <cstdint>

int Drawing::TileDrawOffsetY = 0;
bool Drawing::TileDrawTranslucency = true;

namespace {
struct Clip { int left, top, right, bottom; };
bool intersect(std::int64_t x, std::int64_t y, int width, int height, const RectangleStruct& clip, Clip& out) {
    const auto left = std::max(x, std::int64_t(clip.X)), top = std::max(y, std::int64_t(clip.Y));
    const auto right = std::min(x + width, std::int64_t(clip.X) + clip.Width);
    const auto bottom = std::min(y + height, std::int64_t(clip.Y) + clip.Height);
    if (left >= right || top >= bottom) return false;
    out = {int(left), int(top), int(right), int(bottom)};
    return true;
}
struct RemapScope {
    AlphaLightingRemapClass* value;
    ~RemapScope() { AlphaLightingRemapClass::Release(value); }
};
struct LockScope {
    Surface* surface;
    ~LockScope() { surface->Unlock(); }
};
// Work in byte offsets so advancing across a circular allocation never forms
// an out-of-bounds C++ pointer. GetBuffer retains the original lock/offset ABI.
struct Ring {
    std::uintptr_t head;
    std::int64_t start;
    int size, width;
    template<class T> Ring(T* buffer, int x, int y) :
        head(reinterpret_cast<std::uintptr_t>(buffer->BufferTail) - buffer->BufferSize),
        start(std::int64_t(reinterpret_cast<std::uintptr_t>(buffer->GetBuffer(x, y))) - std::int64_t(head)),
        size(buffer->BufferSize), width(buffer->Width) {}
    WORD& at(int x, int y) const {
        auto offset = (start + (std::int64_t(y) * width + x) * 2) % size;
        if (offset < 0) offset += size;
        return *reinterpret_cast<WORD*>(head + offset);
    }
};
}

game::DrawingStatus game::draw_tmp_software(const TileDrawingRequest& request) {
    auto* surface = reinterpret_cast<Surface*>(request.target);
    const auto* convert = reinterpret_cast<const ConvertClass*>(request.palette);
    const auto* tmp = request.resource;
    const auto* image = request.image;
    if (!surface || !convert || !tmp || !image || !convert->FullColorData || convert->ShadeCount < 1)
        return DrawingStatus::invalid_argument;
    if (surface->GetBytesPerPixel() != 2) return DrawingStatus::unsupported;
    const int x = request.position.X, y = request.position.Y;
    const int level = request.level, intensity = request.intensity, color = request.color;
    bool useZ = request.use_depth;
    const bool flat = request.flat, flag16 = request.flag16, flag17 = request.flag17;
    const auto& clip = request.clip;
    if (clip.Width <= 0 || clip.Height <= 0) return DrawingStatus::skipped;
    auto* alpha = ABuffer::Instance;
    auto* depth = ZBuffer::Instance;
    if (!alpha || alpha->BufferSize <= 0 || !alpha->Surface ||
        (useZ && (!depth || depth->BufferSize <= 0 || !depth->Surface))) return DrawingStatus::unavailable;
    const auto base = useZ ? int(WORD(depth->Bounds.Y + depth->MaxValue - std::int64_t(y) - tmp->Height)) +
        std::int64_t(level) * tmp->Height / -2 : 0;
    RemapScope remap{AlphaLightingRemapClass::FindOrAllocate(convert->ShadeCount)};
    const int row = int(std::min<std::int64_t>(254, std::int64_t(261) * std::max(intensity, 0) / 2048));
    if (!remap.value) return DrawingStatus::backend_failure;
    const WORD* shades = remap.value->Table[row];
    const auto pixel_color = [&](byte pixel, WORD light) {
        // ABuffer uses byte-range intensities stored in WORDs.
        return static_cast<const WORD*>(convert->FullColorData)[pixel | shades[std::min<WORD>(light, 255)]];
    };
    const WORD mask = WORD(Drawing::HalfbrightMask), half_color = WORD((unsigned(color) >> 1) & mask);
    Clip area;
    bool visited = false;
    if (intersect(x, y, 60, 29, clip, area)) {
        const int pitch = surface->GetPitch();
        auto* pixels = static_cast<byte*>(surface->Lock(area.left, area.top));
        if (!pixels) return DrawingStatus::backend_failure;
        LockScope locked{surface};
        visited = true;
        if (!(image->Flags & 2)) useZ = false;
        Ring lights(alpha, area.left, area.top - request.buffer_offset_y);
        // The absent Z case reuses the valid alpha ring but never reads/writes it
        // as depth. This avoids inventing a dummy allocation or a host allocator.
        const Ring depths = useZ ? Ring(depth, area.left, area.top - request.buffer_offset_y) : lights;
        int offset = 0;
        for (int sy = 0; sy < 29; ++sy) {
            const int width = 4 * (sy < 15 ? sy + 1 : 29 - sy), left = (60 - width) / 2;
            if (std::int64_t(y) + sy >= area.top && std::int64_t(y) + sy < area.bottom) {
                const int begin = std::max(left, area.left - x), end = std::min(left + width, area.right - x);
                const int dy = y + sy - area.top;
                auto* dest = reinterpret_cast<WORD*>(pixels + std::ptrdiff_t(dy) * pitch);
                for (int sx = begin; sx < end; ++sx) {
                    const int source = offset + sx - left, dx = x + sx - area.left;
                    if (!useZ) { dest[dx] = pixel_color(image->Pixels()[source], lights.at(dx, dy)); continue; }
                    auto& z = depths.at(dx, dy);
                    if (flat) { z = 0; dest[dx] = half_color; }
                    else if (flag16) z = 0xffff;
                    else if (flag17) {
                        if (request.translucent) {
                            if (z > 0) dest[dx] = WORD(half_color + ((dest[dx] >> 1) & mask));
                        } else if ((sy + source) & 1) dest[dx] = half_color;
                        z = 0;
                    } else {
                        const auto value = base + image->At(image->ZOffset)[source];
                        if (z >= value) { z = WORD(value); dest[dx] = pixel_color(image->Pixels()[source], lights.at(dx, dy)); }
                    }
                }
            }
            offset += width;
        }
    }
    if (!(image->Flags & 1) || flat || flag16 || flag17)
        return visited ? DrawingStatus::drawn : DrawingStatus::skipped;
    // The EXE only clears useZ while visiting the base rectangle. If that
    // rectangle was clipped away, it can read an absent extra-Z plane. Honor
    // the resource flag here too; unused offsets need not address any storage.
    useZ = useZ && (image->Flags & 2);
    const auto extra_x = std::int64_t(x) + image->ExtraX - image->X;
    const auto extra_y = std::int64_t(y) + image->ExtraY - image->Y;
    if (!intersect(extra_x, extra_y, image->ExtraWidth, image->ExtraHeight, clip, area))
        return visited ? DrawingStatus::drawn : DrawingStatus::skipped;
    Ring lights(alpha, area.left, area.top - request.buffer_offset_y);
    const Ring depths = useZ ? Ring(depth, area.left, area.top - request.buffer_offset_y) : lights;
    const int pitch = surface->GetPitch();
    auto* pixels = static_cast<byte*>(surface->Lock(area.left, area.top));
    if (!pixels) return DrawingStatus::backend_failure;
    LockScope locked{surface};
    for (int dy = 0; dy < area.bottom - area.top; ++dy) {
        auto* dest = reinterpret_cast<WORD*>(pixels + std::ptrdiff_t(dy) * pitch);
        auto source = std::size_t(area.top + dy - extra_y) * image->ExtraWidth + std::size_t(area.left - extra_x);
        for (int dx = 0; dx < area.right - area.left; ++dx, ++source) {
            const byte pixel = image->At(image->ExtraOffset)[source];
            if (!pixel) continue;
            if (useZ) {
                auto& z = depths.at(dx, dy);
                const WORD value = WORD(base + image->At(image->ExtraZOffset)[source]);
                if (z < value) continue;
                z = value;
            }
            dest[dx] = pixel_color(pixel, lights.at(dx, dy));
        }
    }
    return DrawingStatus::drawn;
}
