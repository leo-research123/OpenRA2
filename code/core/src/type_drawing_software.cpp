#include "api/software_type_drawing.hpp"
#include "reference_frame.hpp"
#include "yrpp/ConvertClassHelpers.hpp"
#include <memory>
#include "yrpp/IsometricTileTypeClass.h"
#include "type_drawing_software.hpp"
#include "yrpp/Surface.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/MapClass.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include "yrpp/CellClass.h"
#include <cstdint>
#include <bit>
#include <algorithm>
#include <cstring>
#include <vector>

namespace game {
namespace {
DrawingStatus target_adapter(void*, Surface* surface, DrawingTargetHandle*& output) {
    output = reinterpret_cast<DrawingTargetHandle*>(surface);
    return output ? DrawingStatus::drawn : DrawingStatus::invalid_argument;
}
DrawingStatus palette_adapter(void*, ConvertClass* palette, const DrawingPaletteHandle*& output) {
    output = reinterpret_cast<const DrawingPaletteHandle*>(palette);
    return output ? DrawingStatus::drawn : DrawingStatus::invalid_argument;
}
DrawingStatus cell_adapter(void* world, const CellStruct& cell, const DrawingPaletteHandle*& palette, int& intensity) {
    if (!world) return DrawingStatus::unavailable;
    // GetCellAt includes the original MapClass invalid-cell fallback contract.
    const auto* state = static_cast<MapClass*>(world)->GetCellAt(cell);
    if (!state || !state->LightConvert) return DrawingStatus::unavailable;
    palette = reinterpret_cast<const DrawingPaletteHandle*>(state->LightConvert);
    intensity = static_cast<std::int16_t>(state->Intensity_Normal);
    return DrawingStatus::drawn;
}
DrawingStatus draw_shape(void*, const ShapeDrawingRequest& request) {
    auto* target = reinterpret_cast<Surface*>(request.target);
    auto* convert = reinterpret_cast<ConvertClass*>(const_cast<DrawingPaletteHandle*>(request.palette));
    auto* image = request.image;
    if (!target || !convert || !image || !convert->FullColorData) return DrawingStatus::invalid_argument;
    if (target->GetBytesPerPixel() != convert->BytesPerPixel) return DrawingStatus::unsupported;
    if (request.depth_mode != ShapeDepthMode::legacy || request.blend_mode != ShapeBlendMode::palette)
        return DrawingStatus::unsupported;
    // Original palette/alpha/Z/blend modes used by the type/world adapters.
    if ((request.flags & ~0x7F07u) || ((request.flags & 0x100) && !(request.flags & 0x7000)))
        return DrawingStatus::unsupported;
    if (auto* reference = image->AsReference()) {
        if (!reference->Loaded) reference->Load();
        image = reference->Data;
    }
    if (!image) return DrawingStatus::unavailable;
    if (request.frame < 0 || request.frame >= image->Frames) return DrawingStatus::invalid_argument;
    const auto flags = static_cast<BlitterFlags>(request.flags);
    const bool compressed = image->HasCompression(request.frame);
    if (compressed ? !convert->SelectRLEBlitter(flags) : !convert->SelectPlainBlitter(flags))
        return DrawingStatus::unsupported;
    if ((request.flags & 0x800) && !ABuffer::Instance) return DrawingStatus::unavailable;
    if (ZBuffer::Instance) {
        if (compressed && !ABuffer::Instance) return DrawingStatus::unavailable;
        // Native Drawing validates that a real calibrated gradient is bound.
        Drawing::GetZGradient(request.gradient);
    }
    CC_Draw_Shape(target, convert, image, request.frame, &request.position, &request.clip, flags, nullptr,
                  request.depth_adjustment, static_cast<ZGradient>(request.gradient), request.intensity, request.tint,
                  request.depth_image, request.depth_frame, request.depth_offset.X, request.depth_offset.Y);
    return DrawingStatus::drawn;
}
DrawingStatus draw_tile(void*, const TileDrawingRequest& request) { return draw_tmp_software(request); }
// 0x004BFD30: selected building edges read Z strictly and are modulated by
// ABuffer, but the helpers pass false for writing Z. This is not a UI overlay.
DrawingStatus draw_line_pixel(const RasterDrawingRequest& r, Surface* surface) {
    auto* z = ZBuffer::Instance;
    auto* a = ABuffer::Instance;
    if (!z || !a || !z->Surface || !a->Surface) return DrawingStatus::unavailable;
    const int x = r.position.X, y = r.position.Y;
    const int sampleX = r.line_sample_x >= 0 ? r.line_sample_x : x;
    const int lightX = r.line_light_x >= 0 ? r.line_light_x : sampleX;
    auto* zp = static_cast<WORD*>(z->GetBuffer(sampleX, y - z->Bounds.Y));
    auto* ap = static_cast<WORD*>(a->GetBuffer(lightX, y - a->Bounds.Y));
    if (!zp || !ap) return DrawingStatus::backend_failure;
    const int depth = r.blend_mode == RasterBlendMode::particle
                          ? int(std::uint16_t(z->MaxValue + z->Bounds.Y - y)) + r.line_z
                          : int(std::uint16_t(std::uint32_t(std::uint16_t(z->MaxValue + z->Bounds.Y)) -
                                              std::uint32_t(y) - std::uint32_t(r.clip.Y) + std::uint32_t(r.line_z)));
    if (depth >= *zp || (!*ap && r.blend_mode != RasterBlendMode::depth_additive)) return DrawingStatus::skipped;
    auto* dest = static_cast<WORD*>(surface->Lock(x, y));
    if (!dest) return DrawingStatus::backend_failure;
    WORD color = r.color;
    if (r.blend_mode == RasterBlendMode::depth_laser_pair) {
        const int nx = x + r.line_neighbor.X, ny = y + r.line_neighbor.Y;
        if (nx < 0 || ny < 0 || nx >= surface->GetWidth() || ny >= surface->GetHeight()) {
            surface->Unlock();
            return DrawingStatus::skipped;
        }
        // The paired pixel shares the primary sample's Z and light; sample both
        // destinations before writing either. No depth or light buffer is written.
        auto* other = static_cast<WORD*>(surface->Lock(nx, ny));
        if (!other) {
            surface->Unlock();
            return DrawingStatus::backend_failure;
        }
        const WORD primary = *dest, neighbor = *other;
        const auto weight = [](int index) {
            const std::int64_t v = index - 128;
            return int((v * v * v + 2097152) / 16384);
        };
        const auto sample = [&](WORD old, int index) {
            int c[3] = {int((old >> 11) & 31) * 8, int((old >> 5) & 63) * 4, int(old & 31) * 8};
            const int fg = weight(index), bg = weight(256 - index);
            for (int i = 0; i < 3; ++i)
                if (r.line_channels & (1u << i))
                    c[i] = std::min(255, int((unsigned(*ap) * (unsigned((r.line_rgb >> (i * 8)) & 255u) * unsigned(fg) +
                                                               unsigned(c[i]) * unsigned(bg))) >>
                                             15));
            return WORD((unsigned(c[0] >> 3) << 11) | (unsigned(c[1] >> 2) << 5) | unsigned(c[2] >> 3));
        };
        if (r.spotlight_flags & 1) {
            *dest = sample(neighbor, r.line_weight);
            *other = sample(primary, r.line_neighbor_weight);
        } else {
            *dest = sample(primary, r.line_weight);
            *other = sample(neighbor, r.line_neighbor_weight);
        }
        surface->Unlock();
        surface->Unlock();
        return DrawingStatus::drawn;
    } else if (r.blend_mode == RasterBlendMode::depth_additive) {
        int c[3] = {int((*dest >> 11) & 31) * 8, int((*dest >> 5) & 63) * 4, int(*dest & 31) * 8};
        const int source[3] = {r.line_red, r.line_green, r.line_blue};
        int overflow = 0, count = 0;
        for (int i = 0; i < 3; ++i) {
            c[i] += std::bit_cast<int>(unsigned(source[i]) * unsigned(*ap)) >> 7;
            if (c[i] >= 255) {
                overflow += c[i] - 255;
                ++count;
            }
        }
        if (count == 1) overflow >>= 1;
        if (count == 1 || count == 2)
            for (auto& channel : c)
                if (channel < 255) channel += overflow;
        for (auto& channel : c)
            channel = std::min(channel, 255);
        color = WORD((unsigned(c[0] >> 3) << 11) | (unsigned(c[1] >> 2) << 5) | unsigned(c[2] >> 3));
    } else if (r.blend_mode == RasterBlendMode::particle) {
        const auto channel = [&](unsigned shift) {
            const unsigned value = (r.line_rgb >> shift) & 255u;
            return *ap < 127 ? (value * unsigned(*ap)) >> 7 : value;
        };
        color = WORD((channel(0) >> 3) << 11 | (channel(8) >> 2) << 5 | (channel(16) >> 3));
    } else if (r.blend_mode == RasterBlendMode::depth_alpha) {
        const unsigned opacity = r.line_opacity, light = *ap, background = *dest;
        const auto channel = [&](unsigned shift, unsigned old) {
            return (light * (((opacity * ((r.line_rgb >> shift) & 255u)) >> 8) + (((256 - opacity) * old) >> 8))) >> 7;
        };
        color =
            WORD((channel(0, ((background >> 11) & 31u) * 8) >> 3) << 11 |
                 (channel(8, ((background >> 5) & 63u) * 4) >> 2) << 5 | (channel(16, (background & 31u) * 8) >> 3));
    } else if (*ap != 127) {
        const unsigned light = *ap;
        color = WORD((((((color >> 11) & 31u) * 8 * light) >> 7) >> 3) << 11 |
                     (((((color >> 5) & 63u) * 4 * light) >> 7) >> 2) << 5 | ((((color & 31u) * 8 * light) >> 7) >> 3));
    }
    *dest = color;
    surface->Unlock();
    return DrawingStatus::drawn;
}
DrawingStatus draw_wave(const RasterDrawingRequest& r, Surface* surface) {
    auto* z = ZBuffer::Instance;
    if (!z || !z->Surface || !z->BufferHead) return DrawingStatus::unavailable;
    if (surface->GetBytesPerPixel() != 2) return DrawingStatus::unsupported;
    auto* data = static_cast<WORD*>(surface->Lock(0, 0));
    if (!data) return DrawingStatus::backend_failure;
    const int pitch = surface->GetPitch() / 2, width = surface->GetWidth(), height = surface->GetHeight();
    for (unsigned i = 0; i < r.wave_pixel_count; ++i) {
        const auto& pixel = r.wave_pixels[i];
        const auto p = pixel.destination, q = pixel.sample;
        if (p.X < 0 || p.Y < 0 || p.X >= width || p.Y >= height || q.X < 0 || q.Y < 0 || q.X >= width || q.Y >= height)
            continue;
        auto* depth = static_cast<WORD*>(z->GetBuffer(p.X, p.Y - z->Bounds.Y));
        if (!depth || WORD(z->MaxValue + z->Bounds.Y - p.Y + r.line_z) >= *depth) continue;
        const WORD background = data[q.Y * pitch + q.X];
        int red = ((background >> 11) & 31) * 8, green = ((background >> 5) & 63) * 4, blue = (background & 31) * 8;
        const int strength = pixel.strength;
        if (r.color == 0) {
            green += strength * green >> 8;
            blue += strength * blue >> 8;
        } else if (r.color == 3) {
            red += (strength / 2) * red >> 8;
            blue += (strength * blue) >> 6;
        } else {
            red += strength >> 2;
            blue += (3 * strength) >> 3;
        }
        data[p.Y * pitch + p.X] =
            WORD((std::min(red, 255) >> 3) << 11 | (std::min(green, 255) >> 2) << 5 | (std::min(blue, 255) >> 3));
    }
    surface->Unlock();
    return DrawingStatus::drawn;
}
DrawingStatus draw_raster(void*, const RasterDrawingRequest& r) {
    auto* surface = reinterpret_cast<Surface*>(r.target);
    if (r.blend_mode == RasterBlendMode::wave) return draw_wave(r, surface);
    if (surface->GetBytesPerPixel() != 2) return DrawingStatus::unsupported;
    const auto left = std::max({std::int64_t(0), std::int64_t(r.position.X), std::int64_t(r.clip.X)});
    const auto top = std::max({std::int64_t(0), std::int64_t(r.position.Y), std::int64_t(r.clip.Y)});
    const auto right = std::min({std::int64_t(surface->GetWidth()), std::int64_t(r.position.X) + r.width,
                                 std::int64_t(r.clip.X) + r.clip.Width});
    const auto bottom = std::min({std::int64_t(surface->GetHeight()), std::int64_t(r.position.Y) + r.height,
                                  std::int64_t(r.clip.Y) + r.clip.Height});
    if (right <= left || bottom <= top) return DrawingStatus::skipped;
    if (r.blend_mode == RasterBlendMode::shroud || r.blend_mode == RasterBlendMode::fog ||
        r.blend_mode == RasterBlendMode::alpha_shape) {
        auto* a = ABuffer::Instance;
        if (!a || !a->Surface || !a->BufferHead || a->BufferSize <= 0) return DrawingStatus::unavailable;
        for (auto y = top; y < bottom; ++y)
            for (auto x = left; x < right; ++x) {
                const auto value = r.pixels ? r.pixels[(y - r.position.Y) * r.pitch + x - r.position.X] : r.color;
                if (value > 255) continue;
                auto* output = static_cast<WORD*>(a->GetBuffer(int(x), int(y) - a->Bounds.Y));
                if (!output) return DrawingStatus::backend_failure;
                if (r.blend_mode == RasterBlendMode::shroud) {
                    if (value != 254) *output = value;
                } else if (r.blend_mode == RasterBlendMode::fog) {
                    if (value <= 127) *output = WORD(std::max(0, int(*output) + int(value) - 127));
                } else
                    *output = WORD(std::min(255u, unsigned(*output) * unsigned(value) / 127));
            }
        return DrawingStatus::drawn;
    }
    if (r.blend_mode == RasterBlendMode::light_nonzero || r.blend_mode == RasterBlendMode::light_zero) {
        auto* a = ABuffer::Instance;
        if (!a || !a->Surface || !a->BufferHead || a->BufferSize <= 0) return DrawingStatus::unavailable;
        auto* light = static_cast<WORD*>(a->GetBuffer(int(left), int(top) - a->Bounds.Y));
        if (!light) return DrawingStatus::backend_failure;
        if ((*light == 0) != (r.blend_mode == RasterBlendMode::light_zero)) return DrawingStatus::skipped;
        auto* pixel = static_cast<WORD*>(surface->Lock(int(left), int(top)));
        if (!pixel) return DrawingStatus::backend_failure;
        *pixel = r.color;
        surface->Unlock();
        return DrawingStatus::drawn;
    }
    if (r.original_line || r.blend_mode == RasterBlendMode::depth_alpha || r.blend_mode == RasterBlendMode::particle ||
        r.blend_mode == RasterBlendMode::depth_additive || r.blend_mode == RasterBlendMode::depth_laser_pair) {
        if (r.blend_mode == RasterBlendMode::depth_alpha && r.line_opacity < 8) return DrawingStatus::skipped;
        return draw_line_pixel(r, surface);
    }
    if (r.blend_mode == RasterBlendMode::depth_glow) {
        auto* z = ZBuffer::Instance;
        if (!z || !z->Surface) return DrawingStatus::unavailable;
        auto* p = static_cast<WORD*>(z->GetBuffer(r.position.X, r.position.Y - z->Bounds.Y));
        const auto depth = WORD(z->MaxValue + z->Bounds.Y - r.position.Y - r.clip.Y + r.line_z);
        if (!p) return DrawingStatus::backend_failure;
        if (depth >= *p) return DrawingStatus::skipped;
    }

    const int pitch = surface->GetPitch();
    auto* bytes = static_cast<byte*>(surface->Lock(int(left), int(top)));
    if (!bytes) return DrawingStatus::backend_failure;
    for (auto y = top; y < bottom; ++y, bytes += pitch) {
        auto* destination = reinterpret_cast<WORD*>(bytes);
        if (r.blend_mode != RasterBlendMode::copy) {
            for (auto x = left; x < right; ++x) {
                const int strength = r.blend_mode == RasterBlendMode::depth_glow ? r.light_strength
                                     : r.pixels ? (r.pixels[(y - r.position.Y) * r.pitch + x - r.position.X] & 255)
                                                : r.color;
                if (!strength) continue;
                auto& pixel = destination[x - left];
                int rgb[3] = {int((pixel >> 11) & 31) * 8, int((pixel >> 5) & 63) * 4, int(pixel & 31) * 8};
                for (int i = 0; i < 3; ++i)
                    if (r.spotlight_flags & 1)
                        rgb[i] = ((256 - strength) * rgb[i]) >> 8;
                    else if (!(r.spotlight_flags & (2u << i)))
                        rgb[i] = std::min(255, rgb[i] + (std::bit_cast<std::int32_t>(std::uint32_t(strength) *
                                                                                     std::uint32_t(rgb[i])) >>
                                                         8));
                pixel = WORD(std::uint32_t(rgb[0] >> 3) << 11 | std::uint32_t(rgb[1] >> 2) << 5 |
                             std::uint32_t(rgb[2] >> 3));
            }
        } else if (r.pixels)
            std::memcpy(destination, r.pixels + (y - r.position.Y) * r.pitch + left - r.position.X,
                        std::size_t(right - left) * sizeof(WORD));
        else
            std::fill_n(destination, right - left, r.color);
    }
    surface->Unlock();
    return DrawingStatus::drawn;
}
DrawingStatus draw_indexed(void*, const IndexedDrawingRequest& r) {
    auto* surface = reinterpret_cast<Surface*>(r.target);
    auto* convert = reinterpret_cast<const ConvertClass*>(r.palette);
    if (surface->GetBytesPerPixel() != 2 || convert->BytesPerPixel != 2) return DrawingStatus::unsupported;
    if (!convert->FullColorData || convert->ShadeCount < 1) return DrawingStatus::invalid_argument;
    auto* z = ZBuffer::Instance;
    auto* a = ABuffer::Instance;
    if (r.depth_mode != ShapeDepthMode::none && (!z || !z->BufferHead || z->BufferSize <= 0))
        return DrawingStatus::unavailable;
    const int l = int(std::max({std::int64_t(0), std::int64_t(r.position.X), std::int64_t(r.clip.X)})),
              t = int(std::max({std::int64_t(0), std::int64_t(r.position.Y), std::int64_t(r.clip.Y)}));
    const int right = int(std::min({std::int64_t(surface->GetWidth()), std::int64_t(r.position.X) + r.width,
                                    std::int64_t(r.clip.X) + r.clip.Width})),
              bottom = int(std::min({std::int64_t(surface->GetHeight()), std::int64_t(r.position.Y) + r.height,
                                     std::int64_t(r.clip.Y) + r.clip.Height}));
    if (l >= right || t >= bottom) return DrawingStatus::skipped;
    if (r.depth_mode == ShapeDepthMode::legacy) {
        if (!a) return DrawingStatus::unavailable;
        std::vector<byte> indexes(std::size_t(r.width) * r.height);
        for (std::size_t i = 0; i < indexes.size(); ++i)
            indexes[i] = (r.pixels[i] & 0x1000000u) ? byte(r.pixels[i]) : 0;
        alignas(BSurface) std::byte storage[sizeof(BSurface)];
        auto* source = BSurface::Initialize(storage, r.width, r.height, 1, indexes.data());
        struct SourceLifetime {
            BSurface* p;
            ~SourceLifetime() { p->~BSurface(); }
        } source_lifetime{source};
        auto* mutable_convert = const_cast<ConvertClass*>(convert);
        auto* blitter = mutable_convert->SelectPlainBlitter(static_cast<BlitterFlags>(r.flags | (r.shadow ? 1u : 0u)));
        if (!blitter) return DrawingStatus::unsupported;
        const RectangleStruct clip{
            std::max(0, r.clip.X), std::max(0, r.clip.Y),
            int(std::min<std::int64_t>(surface->GetWidth(), std::int64_t(r.clip.X) + r.clip.Width)) -
                std::max(0, r.clip.X),
            int(std::min<std::int64_t>(surface->GetHeight(), std::int64_t(r.clip.Y) + r.clip.Height)) -
                std::max(0, r.clip.Y)};
        const RectangleStruct desired{r.position.X - clip.X, r.position.Y - clip.Y, r.width, r.height};
        const RectangleStruct source_rect{0, 0, r.width, r.height};
        Drawing::PlainBlit(surface, &clip, &desired, reinterpret_cast<Surface*>(source), &source_rect, &source_rect,
                           blitter, r.absolute_depth, r.gradient, r.intensity, int(r.tint), 0);
        return DrawingStatus::drawn;
    }
    auto* remap = AlphaLightingRemapClass::FindOrAllocate(convert->ShadeCount);
    if (!remap) return DrawingStatus::backend_failure;
    struct Release {
        AlphaLightingRemapClass* p;
        ~Release() { AlphaLightingRemapClass::Release(p); }
    } release{remap};
    const auto* shades = remap->Table[std::clamp(int(std::int64_t(261) * r.intensity / 2048), 0, 254)];
    auto* bytes = static_cast<unsigned char*>(surface->Lock(l, t));
    if (!bytes) return DrawingStatus::backend_failure;
    struct Unlock {
        Surface* p;
        ~Unlock() { p->Unlock(); }
    } unlock{surface};
    for (int y = t; y < bottom; ++y) {
        auto* dest = reinterpret_cast<WORD*>(bytes + (y - t) * surface->GetPitch());
        for (int x = l; x < right; ++x) {
            const auto texel = r.pixels[(y - r.position.Y) * r.width + x - r.position.X];
            if (!(texel & 0x1000000u) || !(texel & 255)) continue;
            const auto dz = std::bit_cast<std::int16_t>(std::uint16_t(texel >> 8));
            const auto depth = std::uint16_t(std::uint32_t(r.absolute_depth) + std::uint32_t(dz));
            if (r.depth_mode != ShapeDepthMode::none) {
                auto* p = static_cast<WORD*>(z->GetBuffer(x, y));
                if (!p || depth > *p) continue;
                if (r.depth_mode == ShapeDepthMode::read_write) *p = depth;
            }
            if (r.shadow) {
                dest[x - l] = (dest[x - l] >> 1) & 0x7bef;
                continue;
            }
            WORD light = 127;
            if (a && a->BufferHead && a->BufferSize > 0) {
                auto* p = static_cast<WORD*>(a->GetBuffer(x, y));
                if (p) light = std::min<WORD>(*p, 255);
            }
            dest[x - l] = static_cast<const WORD*>(convert->FullColorData)[(texel & 255) | shades[light]];
        }
    }
    return DrawingStatus::drawn;
}

}
TypeDrawingContext make_software_type_drawing(Surface* target, ConvertClass* palette, MapClass* map) noexcept {
    TypeDrawingContext result;
    result.backend.shape = draw_shape;
    result.backend.tile = draw_tile;
    result.backend.raster = draw_raster;
    result.backend.indexed = draw_indexed;
    result.target = reinterpret_cast<DrawingTargetHandle*>(target);
    result.palette = reinterpret_cast<const DrawingPaletteHandle*>(palette);
    result.world_context = map;
    result.smudge_cell = cell_adapter;
    result.legacy_target = target_adapter;
    result.legacy_palette = palette_adapter;
    result.buffer_offset_y = Drawing::TileDrawOffsetY;
    result.translucent = Drawing::TileDrawTranslucency;
    return result;
}
}

namespace game {
DrawingStatus draw_software_frame(const render::Frame& frame,Surface& target) noexcept {
    if (!frame.reference) return DrawingStatus::unavailable;
    if (frame.width!=target.GetWidth() || frame.height!=target.GetHeight() || target.GetBytesPerPixel()!=2)
        return DrawingStatus::invalid_argument;
    try {
        bool drawn=false;
        for (const auto& d:frame.reference->draws) {
            std::unique_ptr<ConvertClass> convert;
            if (!d.palette.empty()) {
                if (d.shades<1 || d.palette.size()!=std::size_t(d.shades+1)*256) return DrawingStatus::invalid_argument;
                BytePalette empty{};
                convert=std::make_unique<ConvertClass>(empty,empty,2,d.shades,true);
                std::memcpy(convert->FullColorData,d.palette.data()+256,std::size_t(d.shades)*256*2);
                convert->PaletteData=const_cast<std::uint16_t*>(d.palette.data());
                initialize_blitters(convert.get());
            }
            const auto palette=reinterpret_cast<const DrawingPaletteHandle*>(convert.get());
            const auto output=reinterpret_cast<DrawingTargetHandle*>(&target);
            DrawingStatus status=DrawingStatus::unsupported;
            const auto& p=d.words;
            switch(d.kind) {
                case render::ReferenceKind::shape: {
                    if (!convert || d.image.size()<sizeof(SHPStruct)+sizeof(SHPFrame)) return DrawingStatus::invalid_argument;
                    ShapeDrawingRequest q; q.target=output;q.palette=palette;
                    q.image=reinterpret_cast<SHPStruct*>(const_cast<std::uint8_t*>(d.image.data()));
                    q.position=d.position;q.clip=d.clip;q.flags=std::uint32_t(p[0]);q.depth_adjustment=p[1];q.gradient=p[2];
                    q.intensity=p[3];q.tint=p[4];q.depth_mode=static_cast<ShapeDepthMode>(p[5]);
                    q.blend_mode=static_cast<ShapeBlendMode>(p[6]);q.absolute_depth=p[7];q.depth_offset={p[8],p[9]};
                    if (!d.auxiliary.empty()) q.depth_image=reinterpret_cast<SHPStruct*>(const_cast<std::uint8_t*>(d.auxiliary.data()));
                    status=draw_shape(nullptr,q);break;
                }
                case render::ReferenceKind::tile: {
                    if (!convert || d.tile.size()!=sizeof(TMPStruct) || d.image.size()<sizeof(TMPImage)+900) return DrawingStatus::invalid_argument;
                    TileDrawingRequest q; q.target=output;q.palette=palette;q.position=d.position;q.clip=d.clip;
                    q.resource=reinterpret_cast<const TMPStruct*>(d.tile.data());q.image=reinterpret_cast<const TMPImage*>(d.image.data());
                    q.level=p[0];q.intensity=p[1];q.use_depth=p[2];q.flat=p[3];q.flag16=p[4];q.flag17=p[5];q.color=p[6];
                    q.buffer_offset_y=p[7];q.translucent=p[8];status=draw_tile(nullptr,q);break;
                }
                case render::ReferenceKind::raster: {
                    RasterDrawingRequest q;q.target=output;q.position=d.position;q.clip=d.clip;
                    q.width=p[0];q.height=p[1];q.pitch=p[2];q.color=std::uint16_t(p[3]);q.original_line=p[4];
                    q.blend_mode=static_cast<RasterBlendMode>(p[5]);q.spotlight_flags=std::uint32_t(p[6]);q.light_strength=p[7];
                    q.line_z=p[8];q.line_rgb=std::uint32_t(p[9]);q.line_opacity=p[10];q.line_neighbor={p[11],p[12]};
                    q.line_weight=p[13];q.line_neighbor_weight=p[14];q.line_channels=std::uint32_t(p[15]);
                    q.line_light_x=p[16];q.line_sample_x=p[17];q.line_red=p[18];q.line_green=p[19];q.line_blue=p[20];
                    if (!d.pixels.empty()) q.pixels=d.pixels.data();q.pixel_count=std::uint32_t(d.pixels.size());
                    std::vector<WavePixel> wave;for(const auto& pixel:d.wave) wave.push_back({pixel.destination,pixel.sample,pixel.strength});
                    q.wave_pixels=wave.data();q.wave_pixel_count=std::uint32_t(wave.size());status=draw_raster(nullptr,q);break;
                }
                case render::ReferenceKind::indexed: {
                    if (!convert) return DrawingStatus::invalid_argument;
                    IndexedDrawingRequest q;q.target=output;q.palette=palette;q.position=d.position;q.clip=d.clip;
                    q.width=p[0];q.height=p[1];q.intensity=p[2];q.absolute_depth=p[3];q.depth_mode=static_cast<ShapeDepthMode>(p[4]);
                    q.shadow=p[5];q.flags=std::uint32_t(p[6]);q.tint=std::uint32_t(p[7]);q.gradient=p[8];
                    q.pixels=d.indexed.data();q.pixel_count=std::uint32_t(d.indexed.size());status=draw_indexed(nullptr,q);break;
                }
            }
            if (status!=DrawingStatus::drawn && status!=DrawingStatus::skipped) return status;
            drawn|=status==DrawingStatus::drawn;
        }
        return drawn?DrawingStatus::drawn:DrawingStatus::skipped;
    } catch (...) {return DrawingStatus::backend_failure;}
}
}
