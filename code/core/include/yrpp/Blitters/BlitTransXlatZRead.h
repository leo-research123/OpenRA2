// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class BlitTransXlatZRead final : public Blitter {
    friend struct Blitter::Kernel;
public:
    explicit BlitTransXlatZRead(T* data) noexcept;
    ~BlitTransXlatZRead() override final;
    void Blit_Copy(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int w) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int, WORD t) override final;
    void Blit_Move(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l) override final;
    void Blit_Move_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, WORD t) override final;
private:
    T* PaletteData;
};

extern template class BlitTransXlatZRead<BYTE>;
extern template class BlitTransXlatZRead<WORD>;
