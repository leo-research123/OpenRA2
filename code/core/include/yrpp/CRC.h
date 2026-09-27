// YRpp CRCEngine interface, with target CRC32/padding instead of EA's rotate-add.
// Used by the image resource-name cache.
// Scalar encodings follow gamemd 4A1CA0..4A1D90: bool 0/1, little-endian
// short/int (2/4 bytes), IEEE binary32/binary64 (4/8 bytes), shared staging.
#pragma once

#include <cstdint>
class CRCEngine {
public:
    int operator()() const { return Value(); }
    operator int() const { return Value(); }
    void operator()(char datum);
    int operator()(const void* buffer, int length);
    void operator()(bool datum);
    void operator()(short datum);
    void operator()(int datum);
    void operator()(float datum);
    void operator()(double datum);
    template<class T> int operator()(const T& value) {
        return (*this)(static_cast<const void*>(&value), static_cast<int>(sizeof(T)));
    }
    static int Memory(const void* data, int length, int crc);
    static int String(const char* text, int crc);
    static const unsigned int Table[256];
protected:
    bool Buffer_Needs_Data() const { return Index != 0; }
    int Value() const;
public:
    int CRC = 0;
    int Index = 0;
    mutable union Buffer { int Composite; char Buffer[4]; } StagingBuffer{};
};
