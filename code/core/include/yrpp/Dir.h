#pragma once

#include "yrpp/YRMath.h"

#include <bit>

enum class DirType : unsigned char;

// North -> 0x0000
// South -> 0x8000
// ...
// Just a very simple BAM
struct DirStruct
{
public:
    constexpr explicit DirStruct() noexcept : Raw { 0 } { }
    constexpr explicit DirStruct(int raw) noexcept : Raw { static_cast<unsigned short>(raw) } { }
    constexpr explicit DirStruct(double rad) noexcept { SetRadian<65536>(rad); }
    constexpr explicit DirStruct(const DirType dir) noexcept { SetDir(dir); }
    constexpr explicit DirStruct(const noinit_t&) noexcept { }

    constexpr bool operator==(const DirStruct& another) const
    {
        return Raw == another.Raw;
    }

    constexpr bool operator!=(const DirStruct& another) const
    {
        return Raw != another.Raw;
    }

    constexpr void SetDir(DirType dir)
    {
        Raw = static_cast<unsigned short>(static_cast<unsigned char>(dir) * 256);
    }

    constexpr DirType GetDir() const
    {
        return static_cast<DirType>(Raw / 256);
    }

    // If you want to divide it into 32 facings, as 32 has 5 bits
    // then you should type <5> here.
    // So does the others.
    template<size_t Bits>
    constexpr size_t GetValue(size_t offset = 0) const
    {
        return TranslateFixedPoint<16, Bits>(Raw, offset);
    }

    template<size_t Bits>
    constexpr void SetValue(size_t value, size_t offset = 0)
    {
        Raw = static_cast<unsigned short>(TranslateFixedPoint<Bits, 16>(value, offset));
    }

    template<size_t Count>
    constexpr size_t GetFacing(size_t offset = 0) const
    {
        static_assert(std::has_single_bit(Count));

        constexpr size_t Bits = std::bit_width(Count - 1);
        return GetValue<Bits>(offset);
    }

    template<size_t Count>
    constexpr void SetFacing(size_t value, size_t offset = 0)
    {
        static_assert(std::has_single_bit(Count));

        constexpr size_t Bits = std::bit_width(Count - 1);
        SetValue<Bits>(value, offset);
    }

    template<size_t FacingCount>
    constexpr double GetRadian() const
    {
        static_assert(std::has_single_bit(FacingCount));

        constexpr size_t Bits = std::bit_width(FacingCount - 1);

        size_t value = GetValue<Bits>();
        int dir = static_cast<int>(value) - FacingCount / 4; // LRotate 90 degrees
        return dir * (-Math::TwoPi / FacingCount);
    }

    template<size_t FacingCount>
    constexpr void SetRadian(double rad)
    {
        static_assert(std::has_single_bit(FacingCount));

        constexpr size_t Bits = std::bit_width(FacingCount - 1);
        constexpr size_t Max = (1 << Bits) - 1;

        int dir = static_cast<int>(rad / (-Math::TwoPi / FacingCount));
        size_t value = dir + FacingCount / 4; // RRotate 90 degrees
        SetValue<Bits>(value & Max);
    }

private:
    template<size_t BitsFrom, size_t BitsTo>
    constexpr static size_t TranslateFixedPoint(size_t value, size_t offset = 0)
    {
        constexpr size_t MaskIn = ((1u << BitsFrom) - 1);
        constexpr size_t MaskOut = ((1u << BitsTo) - 1);

        if constexpr (BitsFrom > BitsTo)
            return (((((value & MaskIn) >> (BitsFrom - BitsTo - 1)) + 1) >> 1) + offset) & MaskOut;
        else if constexpr (BitsFrom < BitsTo)
            return (((value - offset) & MaskIn) << (BitsTo - BitsFrom)) & MaskOut;
        else
            return value & MaskOut;
    }

public:
    unsigned short Raw;
private:
    unsigned short Padding;
};

static_assert(sizeof(DirStruct) == 4);
