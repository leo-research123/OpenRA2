// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class BlitTransLucent75ZReadWarp final : public Blitter {
    friend struct Blitter::Kernel;
public:
    explicit BlitTransLucent75ZReadWarp(T* data, WORD mask) noexcept;
    ~BlitTransLucent75ZReadWarp() override final;
    void Blit_Copy(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int w) override final;
    void Blit_Copy_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int, WORD t) override final;
    void Blit_Move(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l) override final;
    void Blit_Move_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, WORD t) override final;
private:
    T* PaletteData;
    WORD Mask;
};

extern template class BlitTransLucent75ZReadWarp<BYTE>;
extern template class BlitTransLucent75ZReadWarp<WORD>;
