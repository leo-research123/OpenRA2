// YRpp 9402d7da class implementations with the existing calibrated pixel rules.
#include "BlitterVariants.hpp"
#include "BlitterPixels.hpp"

namespace {
using Op = PixelOperation;
using Depth = PixelDepth;
using Alpha = PixelAlpha;
using Flags = PixelFlags;
}

#define RA2_PLAIN_METHODS(Name, ...) \
    template<typename T> void Name<T>::Blit_Copy(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int w) { \
        using Rules = BlitterRule<__VA_ARGS__>; \
        Blitter::Kernel::plain<Rules, T>(*this, d, s, n, z, zb, ab, l, w, 0); \
    } \
    template<typename T> void Name<T>::Blit_Copy_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int, WORD t) { \
        using Rules = BlitterRule<__VA_ARGS__>; \
        Blitter::Kernel::plain<Rules, T>(*this, d, s, n, z, zb, ab, l, 0, t); \
    } \
    template<typename T> void Name<T>::Blit_Move(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l) { \
        using Rules = BlitterRule<__VA_ARGS__>; \
        Blitter::Kernel::plain<Rules, T>(*this, d, s, n, z, zb, ab, l, 0, 0); \
    } \
    template<typename T> void Name<T>::Blit_Move_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, WORD t) { \
        using Rules = BlitterRule<__VA_ARGS__>; \
        Blitter::Kernel::plain<Rules, T>(*this, d, s, n, z, zb, ab, l, 0, t); \
    }

#define RA2_RLE_METHODS(Name, ...) \
    template<typename T> void Name<T>::Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) { \
        using Rules = BlitterRule<__VA_ARGS__>; \
        Blitter::Kernel::rle<Rules, T>(*this, d, s, n, lead, z, zb, ab, l, w, za, 0); \
    } \
    template<typename T> void Name<T>::Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) { \
        using Rules = BlitterRule<__VA_ARGS__>; \
        Blitter::Kernel::rle<Rules, T>(*this, d, s, n, lead, z, zb, ab, l, w, za, t); \
    }

#define RA2_EMPTY_CONSTRUCTOR(Name) template<typename T> Name<T>::Name() noexcept {  }
#define RA2_EMPTY_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_PALETTE_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(T* data) noexcept { PaletteData = data; }
#define RA2_PALETTE_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_PALETTE_ALPHA_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(T* data, int shadecount) noexcept { PaletteData = data; AlphaRemapper = AlphaLightingRemapClass::FindOrAllocate(shadecount); }
#define RA2_PALETTE_ALPHA_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() { AlphaLightingRemapClass::Release(AlphaRemapper); AlphaRemapper = nullptr; }
#define RA2_MASK_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(WORD mask) noexcept { Mask = mask; }
#define RA2_MASK_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_PALETTE_MASK_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(T* data, WORD mask) noexcept { PaletteData = data; Mask = mask; }
#define RA2_PALETTE_MASK_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_PALETTE_ALPHA_MASK_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(T* data, WORD mask, int shadecount) noexcept { PaletteData = data; Mask = mask; AlphaRemapper = AlphaLightingRemapClass::FindOrAllocate(shadecount); }
#define RA2_PALETTE_ALPHA_MASK_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() { AlphaLightingRemapClass::Release(AlphaRemapper); AlphaRemapper = nullptr; }
#define RA2_REMAP_DESTINATION_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(T* data) noexcept { RemapDest = data; }
#define RA2_REMAP_DESTINATION_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_REMAP_PALETTE_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(T* remap, T* palette) noexcept { RemapData = remap; PaletteData = palette; }
#define RA2_REMAP_PALETTE_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_ZREMAP_PALETTE_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(byte** remap, T* data) noexcept { Remap = remap; PaletteData = data; }
#define RA2_ZREMAP_PALETTE_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() = default;
#define RA2_ZREMAP_PALETTE_ALPHA_CONSTRUCTOR(Name) template<typename T> Name<T>::Name(byte** remap, T* data, int shadecount) noexcept { Remap = remap; PaletteData = data; AlphaRemapper = AlphaLightingRemapClass::FindOrAllocate(shadecount); }
#define RA2_ZREMAP_PALETTE_ALPHA_DESTRUCTOR(Name) template<typename T> Name<T>::~Name() { AlphaLightingRemapClass::Release(AlphaRemapper); AlphaRemapper = nullptr; }

#define RA2_VARIANT(Name, Methods, Layout, ...) \
    RA2_##Layout##_CONSTRUCTOR(Name) \
    RA2_##Layout##_DESTRUCTOR(Name) \
    Methods(Name, __VA_ARGS__) \
    template class Name<BYTE>; \
    template class Name<WORD>;
#define RA2_PLAIN_VARIANT(Name, Layout, ...) RA2_VARIANT(Name, RA2_PLAIN_METHODS, Layout, __VA_ARGS__)
#define RA2_RLE_VARIANT(Name, Layout, ...) RA2_VARIANT(Name, RA2_RLE_METHODS, Layout, __VA_ARGS__)
#include "BlitterVariants.inc"
