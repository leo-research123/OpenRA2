// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class BlitTransZRemapXlatAlpha final : public Blitter {
    friend struct Blitter::Kernel;
public:
    explicit BlitTransZRemapXlatAlpha(byte** remap, T* data, int shadecount) noexcept;
    ~BlitTransZRemapXlatAlpha() override final;
    void Blit_Copy(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int w) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int, WORD t) override final;
    void Blit_Move(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l) override final;
    void Blit_Move_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, WORD t) override final;
private:
    byte** Remap;
    T* PaletteData;
    AlphaLightingRemapClass* AlphaRemapper;
};

extern template class BlitTransZRemapXlatAlpha<BYTE>;
extern template class BlitTransZRemapXlatAlpha<WORD>;
