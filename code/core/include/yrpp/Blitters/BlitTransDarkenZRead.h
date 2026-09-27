// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class BlitTransDarkenZRead final : public Blitter {
    friend struct Blitter::Kernel;
public:
    explicit BlitTransDarkenZRead(WORD mask) noexcept;
    ~BlitTransDarkenZRead() override final;
    void Blit_Copy(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int w) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int, WORD t) override final;
    void Blit_Move(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l) override final;
    void Blit_Move_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, WORD t) override final;
private:
    WORD Mask;
};

extern template class BlitTransDarkenZRead<BYTE>;
extern template class BlitTransDarkenZRead<WORD>;
