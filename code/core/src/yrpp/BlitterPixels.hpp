// Shared algorithms from YRpp 9402d7da Blitters/*.h, calibrated for gamemd.
// Original named classes retain their fields; these templates have no state.
#pragma once
#include "Blitter.hpp"

namespace {
enum class PixelOperation {
    Direct, Palette, Darken, Blend25, Blend50, Blend75,
    RemapDestination, RemapPalette, ZRemapPalette, BlendAlpha,
    WriteAlpha, MultiplyAlpha
};
enum class PixelDepth { None, Read, ReadWrite };
enum class PixelAlpha { None, Palette, Zero, Nonzero, Channel };
enum class PixelFlags : unsigned {
    None = 0, Opaque = 1, Warp = 2, Tint = 4,
    MinimumOne = 8, KeepAlpha = 16, SeparateDepthAdjust = 32,
    KeepDepthOnSkip = 64
};
constexpr PixelFlags operator|(PixelFlags a, PixelFlags b) {
    return PixelFlags(unsigned(a) | unsigned(b));
}
template<PixelOperation Operation, PixelDepth Depth = PixelDepth::None,
    PixelAlpha Alpha = PixelAlpha::None, PixelFlags Flags = PixelFlags::None>
struct BlitterRule {
    static constexpr auto operation = Operation;
    static constexpr auto depth = Depth;
    static constexpr auto alpha = Alpha;
    static constexpr bool has(PixelFlags flag) { return (unsigned(Flags) & unsigned(flag)) != 0; }
    static constexpr bool use_z = Depth != PixelDepth::None;
    static constexpr bool use_alpha = Alpha != PixelAlpha::None;
    static constexpr bool write_alpha = Operation == PixelOperation::WriteAlpha || Operation == PixelOperation::MultiplyAlpha;
};

} // anonymous namespace: rules are used only by BlitterVariants.cpp

// Member of the original class, shared by its existing variants.
struct Blitter::Kernel {
    template<class Rule, class Self>
    static WORD* alpha_table(const Self& self, int level) {
        if constexpr (Rule::alpha == PixelAlpha::Palette)
            return Blitter::Lookup_Alpha_Remapper(level, self.AlphaRemapper);
        else return nullptr;
    }

    template<PixelOperation Operation, typename T>
    static T blend(T source, T destination, WORD mask) {
        if constexpr (Operation == PixelOperation::Blend50)
            return T((mask & (destination >> 1)) + (mask & (source >> 1)));
        else if constexpr (Operation == PixelOperation::Blend25)
            return T((mask & (destination >> 2)) + 3 * (mask & (source >> 2)));
        else
            return T(3 * (mask & (destination >> 2)) + (mask & (source >> 2)));
    }

    template<class Rule, class Self, typename T>
    static void pixel(const Self& self, T& destination, unsigned index,
        WORD alpha, const WORD* table, int warp, WORD tint) {
        constexpr auto op = Rule::operation;
        if constexpr (op == PixelOperation::Direct) destination = T(index);
        else if constexpr (op == PixelOperation::Darken)
            destination = T(self.Mask & (destination >> 1));
        else if constexpr (op == PixelOperation::RemapDestination)
            destination = self.RemapDest[destination];
        else {
            if constexpr (op == PixelOperation::RemapPalette) index = self.RemapData[index];
            if constexpr (op == PixelOperation::ZRemapPalette) index = (*self.Remap)[index];
            if constexpr (Rule::alpha == PixelAlpha::Palette) index |= table[alpha];
            const T source = self.PaletteData[index];
            if constexpr (op == PixelOperation::Blend25 || op == PixelOperation::Blend50 || op == PixelOperation::Blend75)
                destination = blend<op>(source, (&destination)[Rule::has(PixelFlags::Warp) ? warp : 0], self.Mask);
            else if constexpr (op == PixelOperation::BlendAlpha)
                destination = T(Blitter::BlendAlphaRGB(WORD(source), WORD(destination), alpha));
            else if constexpr (Rule::has(PixelFlags::Tint)) destination = T(source | tint);
            else destination = source;
        }
    }

    template<class Rule, typename T, class Self>
    static void plain(Self& self, void* output, byte* source, int length,
        int zbase, WORD* zbuf, WORD* abuf, int level, int warp, WORD tint) {
        if constexpr (Rule::has(PixelFlags::MinimumOne)) length = std::max(length, 1);
        else if (length < 0) return;
        auto* destination = static_cast<T*>(output);
        auto* table = alpha_table<Rule>(self, level);
        for (int i = 0; i < length; ++i, ++destination) {
            bool visible = true;
            if constexpr (Rule::use_z) visible = zbase < *zbuf;
            if (visible) {
                const unsigned index = Rule::operation == PixelOperation::Direct
                    ? reinterpret_cast<T*>(source)[i] : source[i];
                if (Rule::has(PixelFlags::Opaque) || index) {
                    WORD alpha = 0;
                    if constexpr (Rule::use_alpha && !Rule::write_alpha) alpha = *abuf;
                    if constexpr (Rule::alpha == PixelAlpha::Zero) visible = alpha == 0;
                    if constexpr (Rule::alpha == PixelAlpha::Nonzero) visible = alpha != 0;
                    if (visible) {
                        if constexpr (Rule::write_alpha) {
                            const int value = Rule::operation == PixelOperation::MultiplyAlpha
                                ? int(index) * level + zbase : int(index) + zbase;
                            *abuf = WORD(std::min(value, 255));
                        } else pixel<Rule>(self, *destination, index, alpha, table, warp, tint);
                        if constexpr (Rule::depth == PixelDepth::ReadWrite) *zbuf = WORD(zbase);
                    }
                }
            }
            if constexpr (Rule::use_z) {
                ++zbuf;
                ZBuffer::Instance->AdjustPointer(zbuf);
            }
            // 495A50 holds the Alpha pointer even while its Z pointer advances.
            if constexpr (Rule::use_alpha && !Rule::has(PixelFlags::KeepAlpha)) {
                ++abuf;
                ABuffer::Instance->AdjustPointer(abuf);
            }
        }
    }

    template<class Rule, typename T, class Self>
    static void rle(Self& self, void* output, byte* source, int length, int lead,
        int zbase, WORD* zbuf, WORD* abuf, int level, int warp, byte* adjust, WORD tint) {
        auto* destination = static_cast<T*>(output);
        auto* table = alpha_table<Rule>(self, level);
        RLEBlitter::Process_Pre_Lines<Rule::use_z, Rule::use_alpha>(destination, source, length, lead, zbuf, abuf);
        auto shade = [&](T& target, byte index, int z, WORD& depth, signed char delta, WORD alpha, signed char write_delta) {
            if constexpr (Rule::use_z) { if (z - delta >= depth) return; }
            pixel<Rule>(self, target, index, alpha, table, warp, tint);
            if constexpr (Rule::depth == PixelDepth::ReadWrite) depth = WORD(z - write_delta);
        };
        // The original traversal owns transparent runs and pointer wrapping.
        // Two methods consume distinct check/write adjustments; two others
        // leave Z/adjust in place across transparent runs. Keep both explicit.
        constexpr int mode = Rule::has(PixelFlags::SeparateDepthAdjust) ? 1 : Rule::has(PixelFlags::KeepDepthOnSkip) ? 2 : 0;
        if constexpr (mode == 1) {
            RLEBlitter::Process_Pixel_Datas<true, false, mode>(destination, source, length, zbase, zbuf, abuf, adjust,
                [&](T& d, byte i, int z, WORD& depth, signed char delta, signed char write_delta) {
                    shade(d, i, z, depth, delta, 0, write_delta);
                });
        } else if constexpr (Rule::use_z && Rule::use_alpha) {
            RLEBlitter::Process_Pixel_Datas<true, true>(destination, source, length, zbase, zbuf, abuf, adjust,
                [&](T& d, byte i, int z, WORD& depth, signed char delta, WORD alpha) {
                    shade(d, i, z, depth, delta, alpha, delta);
                });
        } else if constexpr (Rule::use_z) {
            RLEBlitter::Process_Pixel_Datas<true, false, mode>(destination, source, length, zbase, zbuf, abuf, adjust,
                [&](T& d, byte i, int z, WORD& depth, signed char delta) {
                    shade(d, i, z, depth, delta, 0, delta);
                });
        } else if constexpr (Rule::use_alpha) {
            RLEBlitter::Process_Pixel_Datas<false, true>(destination, source, length, zbase, zbuf, abuf, adjust,
                [&](T& d, byte i, WORD alpha) { pixel<Rule>(self, d, i, alpha, table, warp, tint); });
        } else {
            RLEBlitter::Process_Pixel_Datas<false, false>(destination, source, length, zbase, zbuf, abuf, adjust,
                [&](T& d, byte i) { pixel<Rule>(self, d, i, 0, table, warp, tint); });
        }
    }
};
