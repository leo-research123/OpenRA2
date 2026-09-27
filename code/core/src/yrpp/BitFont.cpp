// YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc BitFont.h interfaces.
// Algorithms reconstructed against gamemd 433880..434A40.
// The fixed EA WWFontClass sources use a different disk format; no substitute
// font object or font renderer is introduced here.
#include "yrpp/BitFont.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/Memory.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <bit>
#include <climits>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
constexpr std::uint32_t dense_signature = 0x546e6f66; // fonT
constexpr std::uint32_t range_signature = 0x744e6f46; // FoNt
constexpr std::size_t character_count = 0x10000;

std::uint32_t read_word(const unsigned char* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
void free_data(BitFont::InternalData* data) noexcept {
    if (!data) return;
    YRMemory::Deallocate(data->SymbolTable);
    YRMemory::Deallocate(data->Bitmaps);
    YRMemory::Deallocate(data);
}
using DataOwner = std::unique_ptr<BitFont::InternalData, decltype(&free_data)>;
struct FreeBlock {
    void operator()(void* block) const noexcept { YRMemory::Deallocate(block); }
};
using BlockOwner = std::unique_ptr<unsigned char, FreeBlock>;

// BitFont's 434840 helper uses inclusive right/bottom coordinates.
bool intersect(LTRBStruct& out, const LTRBStruct& a, const LTRBStruct& b) {
    if (a.Top > b.Bottom || b.Top > a.Bottom || a.Left > b.Right || b.Left > a.Right) {
        out = {0, 0, -1, -1};
        return false;
    }
    const LTRBStruct value{std::max(a.Left, b.Left), std::max(a.Top, b.Top),
        std::min(a.Right, b.Right), std::min(a.Bottom, b.Bottom)};
    out = value;
    return true;
}
bool valid_dimensions(const BitFont::InternalData& data) {
    // Reject corrupt payloads before allocating or addressing glyph pixels.
    // These limits preserve the byte-width glyph header and original int I/O.
    if (data.FontWidth < 0 || data.FontWidth > 255 || data.Stride <= 0 ||
        data.FontHeight <= 0 || data.Lines <= 0 || data.Count < 0 ||
        data.Count > 65535 || data.SymbolDataSize <= 0) return false;
    const auto bits = std::int64_t(data.Stride) * 8;
    const auto minimum = std::int64_t(data.Stride) * data.FontHeight + 1;
    return bits >= data.FontWidth && minimum <= data.SymbolDataSize &&
        std::int64_t(data.Count) * data.SymbolDataSize <= std::numeric_limits<int>::max();
}

// 434700: the original fallback is the inverse bitmap of capital X.
void create_fallback(BitFont& font) {
    const auto& data = *font.InternalPTR;
    const auto bytes = std::size_t(data.Stride) * data.FontHeight;
    auto* glyph = static_cast<unsigned char*>(YRMemory::AllocateBytes(bytes + 2));
    if (!glyph) return;
    // Original leaves this allocation uninitialized when X is missing. Keep
    // that malformed/incomplete font case deterministic and non-drawing.
    std::memset(glyph, 0, bytes + 2);
    font.Pointer_8 = glyph;
    if (const auto* source = font.GetCharacterBitmap(L'X')) {
        glyph[0] = source[0];
        for (std::size_t i = 0; i < bytes; ++i)
            glyph[i + 1] = static_cast<unsigned char>(~source[i + 1]);
    }
}
}

#ifndef RA2_BITFONT_GAME
namespace { BitFont* font_instance = nullptr; }
BitFont*& BitFont::Instance = font_instance;
#endif

BitFont::BitFont(const char* filename) {
    InternalPTR = nullptr;
    Pointer_8 = nullptr;
    pGraphBuffer = nullptr;
    Color = 0x7fff;
    DefaultColor2 = 0x3555;
    Unknown_28 = 64;
    Bool_40 = true;
    field_41 = true;
    field_18 = 1;
    field_1C = 1;
    field_20 = 0;
    Bounds = {0, 0, 0, 0};
    State_2C = 1;
    InternalPTR = LoadInternalData(filename);
    if (InternalPTR) {
        field_18 = InternalPTR->Stride;
        field_1C = InternalPTR->Lines;
        create_fallback(*this);
    }
    // +10, +14, +42/+43 are not initialized by the original constructor.
}

BitFont::~BitFont() {
    YRMemory::Deallocate(Pointer_8);
    free_data(InternalPTR);
}

BitFont::InternalData* YRPP_FASTCALL BitFont::LoadInternalData(const char* filename) {
    if (!filename || !*filename) return nullptr;
    try {
        CCFileClass file(filename);
        if (!file.Exists(false) || !file.Open(FileAccessMode::Read)) return nullptr;
        DataOwner data(static_cast<InternalData*>(YRMemory::Allocate(sizeof(InternalData))), free_data);
        if (!data) return nullptr;
        std::memset(data.get(), 0, sizeof(InternalData));
        data->SymbolTable = static_cast<short*>(YRMemory::Allocate(character_count * sizeof(short)));
        if (!data->SymbolTable) return nullptr;
        std::memset(data->SymbolTable, 0, character_count * sizeof(short));
        unsigned char header[36]{};
        if (file.ReadBytes(header, 28) != 28) return nullptr;
        const auto signature = read_word(header);
        if (signature != dense_signature && signature != range_signature) return nullptr;
        if (signature == range_signature) {
            if (file.Seek(0, FileSeekMode::Set) != 0 || file.ReadBytes(header, 36) != 36) return nullptr;
        }
        int* fields[] = {&data->FontWidth, &data->Stride, &data->FontHeight,
            &data->Lines, &data->Count, &data->SymbolDataSize};
        for (unsigned i = 0; i < 6; ++i)
            *fields[i] = std::bit_cast<std::int32_t>(read_word(header + 4 + 4 * i));
        if (signature == dense_signature) {
            if (!valid_dimensions(*data)) return nullptr;
            if (file.ReadBytes(data->SymbolTable, int(character_count * sizeof(short))) !=
                int(character_count * sizeof(short))) return nullptr;
            const auto* raw = reinterpret_cast<const unsigned char*>(data->SymbolTable);
            for (std::size_t i = 0; i < character_count; ++i) {
                const auto index = std::uint16_t(raw[2 * i] | (unsigned(raw[2 * i + 1]) << 8));
                data->SymbolTable[i] = std::bit_cast<short>(index);
            }
        } else {
            const auto range_count = read_word(header + 20);
            const auto ranges_offset = read_word(header + 28);
            const auto bitmap_offset = read_word(header + 32);
            if (range_count > 65536 || ranges_offset > INT_MAX || bitmap_offset > INT_MAX) return nullptr;
            data->Count = 0;
            if (!valid_dimensions(*data)) return nullptr;
            const auto bytes = range_count * 12;
            BlockOwner ranges(static_cast<unsigned char*>(YRMemory::AllocateBytes(bytes)));
            if (bytes && !ranges) return nullptr;
            if (file.Seek(int(ranges_offset), FileSeekMode::Set) != int(ranges_offset) ||
                file.ReadBytes(ranges.get(), int(bytes)) != int(bytes)) return nullptr;
            for (std::uint32_t i = 0; i < range_count; ++i) {
                const auto* record = ranges.get() + 12 * i;
                const auto first_index = std::uint16_t(record[0] | (unsigned(record[1]) << 8));
                const auto first = std::bit_cast<std::int32_t>(read_word(record + 4));
                const auto last = std::bit_cast<std::int32_t>(read_word(record + 8));
                if (first > last) continue;
                if (first < 0 || last >= int(character_count)) return nullptr;
                const auto count = std::uint32_t(last - first) + 1;
                if (count > 65535u - unsigned(data->Count) || count > 65535u - first_index) return nullptr;
                for (std::uint32_t j = 0; j < count; ++j)
                    data->SymbolTable[first + j] = std::bit_cast<short>(std::uint16_t(first_index + j + 1));
                data->Count += int(count);
            }
            if (!valid_dimensions(*data) ||
                file.Seek(int(bitmap_offset), FileSeekMode::Set) != int(bitmap_offset)) return nullptr;
        }
        for (std::size_t i = 0; i < character_count; ++i) {
            const auto index = static_cast<std::uint16_t>(data->SymbolTable[i]);
            if (index > data->Count) return nullptr;
            data->ValidSymbolCount += index != 0;
        }
        const int bytes = data->Count * data->SymbolDataSize;
        data->Bitmaps = static_cast<char*>(YRMemory::Allocate(bytes));
        if (bytes && !data->Bitmaps) return nullptr;
        if (file.ReadBytes(data->Bitmaps, bytes) != bytes) return nullptr;
        for (int i = 0; i < data->Count; ++i) {
            const auto width = static_cast<unsigned char>(data->Bitmaps[i * data->SymbolDataSize]);
            if (width > std::int64_t(data->Stride) * 8) return nullptr;
        }
        return data.release();
    } catch (...) {
        // File/name allocation errors cannot cross the original-game boundary.
        return nullptr;
    }
}

unsigned char* BitFont::GetCharacterBitmap(wchar_t character) {
    if (!InternalPTR || static_cast<std::uint32_t>(character) >= character_count) return nullptr;
    const auto index = static_cast<std::uint16_t>(InternalPTR->SymbolTable[character]);
    return index ? reinterpret_cast<unsigned char*>(InternalPTR->Bitmaps) +
        (std::size_t(index) - 1) * InternalPTR->SymbolDataSize : nullptr;
}

bool BitFont::GetTextDimension(const wchar_t* text, int* width, int* height, int max_width) {
    if (!InternalPTR || !text) {
        if (height) *height = 0;
        if (width) *width = 0;
        return false;
    }
    wchar_t previous = 0;
    const wchar_t* next = text + 1;
    const wchar_t* last_space = nullptr;
    int current = 0, characters = 0, maximum = 0, space_maximum = 0;
    int total_height = field_1C;
    for (wchar_t character = *text; character; character = *next++) {
        if (character == L'\t') {
            if (Unknown_28) current = Unknown_28 + current - (Unknown_28 + current) % Unknown_28;
        } else if (character == L'\n' || character == L'\r') {
            if (previous != L'\r') { current = 0; characters = 0; total_height += field_1C; }
        } else {
            if (character == L' ') { last_space = next; space_maximum = maximum; }
            const auto* glyph = GetCharacterBitmap(character);
            if (!glyph) glyph = static_cast<const unsigned char*>(Pointer_8);
            if (glyph) {
                ++characters;
                const int advance = int(*glyph) + State_2C;
                current += advance;
                if (max_width && current > max_width) {
                    if (last_space) {
                        if (characters > 1) { next = last_space; maximum = space_maximum; }
                    } else {
                        if (characters > 1) { current -= advance; --next; }
                        if (current > maximum) maximum = current;
                    }
                    current = 0; characters = 0; last_space = nullptr; total_height += field_1C;
                } else if (current > maximum) maximum = current;
            }
        }
        previous = character;
    }
    if (width) *width = maximum;
    if (height) *height = total_height;
    return true;
}

int BitFont::Blit(wchar_t character, int x, int y, int color) {
    if (character == L'\t') return Unknown_28 ?
        Unknown_28 + x - (Unknown_28 + x - field_20) % Unknown_28 : x;
    auto pixel = color == -1 ? Color : static_cast<WORD>(color);
    const auto* glyph = GetCharacterBitmap(character);
    if (!glyph) {
        pixel ^= 0x5555;
        glyph = static_cast<const unsigned char*>(Pointer_8);
    }
    if (!glyph || !pGraphBuffer || !InternalPTR) return x;
    const int width = *glyph;
    const int height = InternalPTR->FontHeight;
    const int next = x + width + State_2C;
    const LTRBStruct rectangle{x, y, x + width - 1, y + height - 1};
    LTRBStruct clipped = rectangle;
    if (field_41 && !intersect(clipped, rectangle, Bounds)) return next;
    // Original bitmap rows are MSB-first, opaque bits only, on 16-bit surfaces.
    // Integer offsets are formed only for pixels that survive clipping.
    for (int row = clipped.Top; row <= clipped.Bottom; ++row) {
        const auto* bits = glyph + 1 + std::size_t(row - y) * field_18;
        for (int column = clipped.Left; column <= clipped.Right; ++column) {
            const auto source_x = unsigned(column - x);
            if (bits[source_x / 8] & (0x80u >> (source_x % 8))) {
                const auto offset = std::ptrdiff_t(row) * PitchDiv2 + column;
                pGraphBuffer[offset] = std::bit_cast<short>(pixel);
            }
        }
    }
    return next;
}

void BitFont::Lock(Surface* surface) {
    pGraphBuffer = static_cast<short*>(surface->Lock(0, 0));
    PitchDiv2 = int(static_cast<unsigned>(surface->GetPitch()) >> 1);
    const LTRBStruct available{0, 0, surface->GetWidth() - 1, surface->GetHeight() - 1};
    if (Bounds.Left | Bounds.Top | Bounds.Right | Bounds.Bottom)
        intersect(Bounds, Bounds, available);
    else Bounds = available;
}
void BitFont::UnLock(Surface* surface) {
    surface->Unlock();
    pGraphBuffer = nullptr;
    PitchDiv2 = 0;
}
void BitFont::SetBounds(LTRBStruct* bounds) { Bounds = bounds ? *bounds : LTRBStruct{0, 0, 0, 0}; }
void BitFont::SetRectangle(LTRBStruct* rectangle) { SetBounds(rectangle); }
void BitFont::SetColor(WORD color) { Color = color; }
void BitFont::SetClipMode(bool clipped) { field_41 = clipped; }
void BitFont::SetField20(int value) { field_20 = value; }
void BitFont::SetField41(char value) { field_41 = value; }
