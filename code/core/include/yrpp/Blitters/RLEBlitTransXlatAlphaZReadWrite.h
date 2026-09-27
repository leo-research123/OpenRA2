// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class RLEBlitTransXlatAlphaZReadWrite final : public RLEBlitter {
    friend struct Blitter::Kernel;
public:
    explicit RLEBlitTransXlatAlphaZReadWrite(T* data, int shadecount) noexcept;
    ~RLEBlitTransXlatAlphaZReadWrite() override final;
    void Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) override final;
private:
    T* PaletteData;
    AlphaLightingRemapClass* AlphaRemapper;
};

extern template class RLEBlitTransXlatAlphaZReadWrite<BYTE>;
extern template class RLEBlitTransXlatAlphaZReadWrite<WORD>;
