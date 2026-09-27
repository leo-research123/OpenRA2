// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class RLEBlitTransRemapDest final : public RLEBlitter {
    friend struct Blitter::Kernel;
public:
    explicit RLEBlitTransRemapDest(T* data) noexcept;
    ~RLEBlitTransRemapDest() override final;
    void Blit_Copy(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int lead, int z, WORD* zb, WORD* ab, int l, int w, byte* za, WORD t) override final;
private:
    T* RemapDest;
};

extern template class RLEBlitTransRemapDest<BYTE>;
extern template class RLEBlitTransRemapDest<WORD>;
