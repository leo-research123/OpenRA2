// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class RLEBlitTransLucent50Alpha final : public RLEBlitter {
    friend struct Blitter::Kernel;
public:
    explicit RLEBlitTransLucent50Alpha(T* data, WORD mask, int shadecount) noexcept;
    ~RLEBlitTransLucent50Alpha() override final;
    void Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) override final;
private:
    T* PaletteData;
    AlphaLightingRemapClass* AlphaRemapper;
    WORD Mask;
};

extern template class RLEBlitTransLucent50Alpha<BYTE>;
extern template class RLEBlitTransLucent50Alpha<WORD>;
