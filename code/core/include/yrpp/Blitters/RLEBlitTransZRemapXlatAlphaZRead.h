// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class RLEBlitTransZRemapXlatAlphaZRead final : public RLEBlitter {
    friend struct Blitter::Kernel;
public:
    explicit RLEBlitTransZRemapXlatAlphaZRead(byte** remap, T* data, int shadecount) noexcept;
    ~RLEBlitTransZRemapXlatAlphaZRead() override final;
    void Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) override final;
private:
    byte** Remap;
    T* PaletteData;
    AlphaLightingRemapClass* AlphaRemapper;
};

extern template class RLEBlitTransZRemapXlatAlphaZRead<BYTE>;
extern template class RLEBlitTransZRemapXlatAlphaZRead<WORD>;
