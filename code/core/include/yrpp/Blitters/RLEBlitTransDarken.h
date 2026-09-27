// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class RLEBlitTransDarken final : public RLEBlitter {
    friend struct Blitter::Kernel;
public:
    explicit RLEBlitTransDarken(WORD mask) noexcept;
    ~RLEBlitTransDarken() override final;
    void Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) override final;
private:
    WORD Mask;
};

extern template class RLEBlitTransDarken<BYTE>;
extern template class RLEBlitTransDarken<WORD>;
