// YRpp 9402d7da: original class layout; compiled BYTE/WORD implementations.
#pragma once

#include "yrpp/Blitters/Blitter.h"

template<typename T>
class BlitTransLucent50AlphaZReadWarp final : public Blitter {
    friend struct Blitter::Kernel;
public:
    explicit BlitTransLucent50AlphaZReadWarp(T* data, WORD mask, int shadecount) noexcept;
    ~BlitTransLucent50AlphaZReadWarp() override final;
    // Fixed gamemd WORD specialization; BYTE has a separate implementation.
    /// VA: 0x00495590
    void Blit_Copy(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int w) override final;
    // WORD wrapper ignores tint and invokes Blit_Copy with warp=0.
    /// VA: 0x004956D0
    void Blit_Copy_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, int, WORD t) override final;
    void Blit_Move(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l) override final;
    void Blit_Move_Tinted(void* d, byte* s, int n, int z, WORD* zb, WORD* ab, int l, WORD t) override final;
private:
    T* PaletteData;
    AlphaLightingRemapClass* AlphaRemapper;
    WORD Mask;
};

extern template class BlitTransLucent50AlphaZReadWarp<BYTE>;
extern template class BlitTransLucent50AlphaZReadWarp<WORD>;
