// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class RLEBlitTransLucent50AlphaZRead final : public RLEBlitter {
    friend struct Blitter::Kernel;
public:
    explicit RLEBlitTransLucent50AlphaZRead(T* data, WORD mask, int shadecount) noexcept;
    ~RLEBlitTransLucent50AlphaZRead() override final;
    void Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) override final;
private:
    T* PaletteData;
    AlphaLightingRemapClass* AlphaRemapper;
    WORD Mask;
};

extern template class RLEBlitTransLucent50AlphaZRead<BYTE>;
extern template class RLEBlitTransLucent50AlphaZRead<WORD>;
