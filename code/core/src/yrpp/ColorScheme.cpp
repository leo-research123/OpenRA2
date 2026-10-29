// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 scheme.cpp lifecycle.
// Electronic Arts / OpenTS; additional terms: third_party/opents/LICENSE.md.
// YR 0x0068C710 / 0x0068C8D0; shared palette arithmetic is calibrated separately.
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/FileSystem.h"
#include "type_resources.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>
#include <new>

#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<ColorScheme*> color_schemes;
byte player_color_schemes[9]{3, 11, 21, 29, 13, 25, 17, 15, 5};
using PaletteEntry = HashObject<HashString, SchemeNode*>;
using PaletteTable = HashTable<HashString, SchemeNode*>;
// The original singleton at 0xAC48F0 contains these four fields, in this order.
// All vectors, keys and nodes use existing YRpp types.
struct PaletteState {
    TheaterType Theater = TheaterType::None;
    DynamicVectorClass<char*>* Names = nullptr;
    DynamicVectorClass<ColorStruct>* Colors = nullptr;
    PaletteTable* Table = nullptr;
};
PaletteState* palette_state = nullptr;
#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(PaletteState) == 0x10);
static_assert(sizeof(PaletteEntry) == 0x104);
static_assert(sizeof(DynamicVectorClass<PaletteEntry>) == 0x18);
#endif
DWORD YRPP_FASTCALL palette_hash(const HashString& key) {
    int hash = 0;
    for (const char* p = key.Name; *p; ++p)
        hash = (static_cast<signed char>(*p) + 26 * hash) % 31;
    return static_cast<DWORD>(hash);
}
HashString palette_key(const char* name) {
    HashString key{};
    const auto count = std::strlen(name);
    if (count >= sizeof(key.Name)) throw std::bad_alloc();
    for (std::size_t i = 0; i < count; ++i)
        key.Name[i] = name[i] >= 'a' && name[i] <= 'z' ? char(name[i] - 'a' + 'A') : name[i];
    return key;
}
PaletteState& palette_registry() {
    if (!palette_state) {
        std::unique_ptr<PaletteState> state(new PaletteState);
        std::unique_ptr<DynamicVectorClass<char*>> names(new DynamicVectorClass<char*>);
        std::unique_ptr<DynamicVectorClass<ColorStruct>> colors(new DynamicVectorClass<ColorStruct>);
        std::unique_ptr<PaletteTable> table(new PaletteTable{});
        std::unique_ptr<DynamicVectorClass<PaletteEntry>[]> buckets(new DynamicVectorClass<PaletteEntry>[31]);
        table->BucketHashFunction = palette_hash;
        table->BucketCount = 31;
        table->BucketGrowthStep = 10;
        table->Buckets = buckets.release();
        state->Names = names.release();
        state->Colors = colors.release();
        state->Table = table.release();
        palette_state = state.release();
    }
    return *palette_state;
}
PaletteEntry* palette_find(PaletteTable& table, const HashString& key) {
    auto& bucket = table.Buckets[table.BucketHashFunction(key) % table.BucketCount];
    for (int i = bucket.Count - 1; i >= 0; --i)
        if (std::strcmp(bucket.Items[i].Key.Name, key.Name) == 0) return &bucket.Items[i];
    return nullptr;
}
void destroy_palette(SchemeNode& node) noexcept {
    if (node.Schemes) {
        for (int i = 0; i < node.Schemes->Count; ++i)
            delete node.Schemes->Items[i];
        delete node.Schemes;
        node.Schemes = nullptr;
    }
}
struct PaletteDeleter {
    void operator()(DynamicVectorClass<ColorScheme*>* schemes) const noexcept {
        SchemeNode node{};
        node.Schemes = schemes;
        destroy_palette(node);
    }
};
DynamicVectorClass<ColorScheme*>* build_palette(PaletteState& state, const char* name) {
    if (state.Theater == TheaterType::None) return nullptr;
    const auto* theater = Theater::Get(state.Theater);
    char filename[260];
    std::snprintf(filename, sizeof(filename), "%s%s.PAL", name, theater->Extension);
    for (char* p = filename; *p; ++p)
        if (*p >= 'a' && *p <= 'z') *p = char(*p - 'a' + 'A');
    const auto* source = static_cast<const BytePalette*>(FileSystem::LoadFile(filename, false));
    if (!source) return nullptr;
    BytePalette palette;
    for (int i = 0; i < 256; ++i) {
        palette.Entries[i].R = byte(source->Entries[i].R << 2);
        palette.Entries[i].G = byte(source->Entries[i].G << 2);
        palette.Entries[i].B = byte(source->Entries[i].B << 2);
    }
    std::unique_ptr<DynamicVectorClass<ColorScheme*>> result(new DynamicVectorClass<ColorScheme*>(20));
    try {
        for (int i = 0; i < state.Colors->Count; ++i) {
            for (int pair = 0; pair < 2; ++pair) {
                std::unique_ptr<ColorScheme> scheme(new ColorScheme(state.Names->Items[i], state.Colors->Items[i],
                                                                    palette, FileSystem::TEMPERAT_PAL, pair ? 53 : 1,
                                                                    false));
                scheme->ArrayIndex = 2 * i + pair;
                if (!result->AddItem(scheme.get())) throw std::bad_alloc();
                scheme.release();
            }
        }
    } catch (...) {
        for (int i = 0; i < result->Count; ++i)
            delete result->Items[i];
        throw;
    }
    return result.release();
}
struct PaletteCleanup {
    ~PaletteCleanup() { ColorScheme::ReleasePaletteCache(); }
} palette_cleanup;
}
DynamicVectorClass<ColorScheme*>& ColorScheme::Array = color_schemes;
byte (&ColorScheme::PlayerColorToColorSchemeLUT)[9] = player_color_schemes;
int YRPP_FASTCALL ColorScheme::GetNumberOfSchemes() noexcept {
    try {
        return palette_registry().Names->Count;
    } catch (...) {
        game::type_resource_failure("Palette");
        return 0;
    }
}
byte YRPP_FASTCALL ColorScheme::AddPaletteColor(const char* name, const ColorStruct& color) noexcept {
    try {
        const auto key = palette_key(name);
        (void)key;
        auto& state = palette_registry();
        std::unique_ptr<char[]> copy(new char[256]);
        std::strcpy(copy.get(), name);
        if (state.Names->Count == state.Names->Capacity && !state.Names->SetCapacity(state.Names->Capacity + 10))
            throw std::bad_alloc();
        if (state.Colors->Count == state.Colors->Capacity && !state.Colors->SetCapacity(state.Colors->Capacity + 10))
            throw std::bad_alloc();
        state.Names->Items[state.Names->Count++] = copy.release();
        state.Colors->Items[state.Colors->Count++] = color;
        return color.B;
    } catch (...) {
        game::type_resource_failure(name);
        return 0;
    }
}
void ColorScheme::ClearPaletteColors() noexcept {
    try {
        auto& state = palette_registry();
        for (int i = 0; i < state.Names->Count; ++i)
            delete[] state.Names->Items[i];
        state.Names->Clear();
        state.Colors->Clear();
    } catch (...) {
        game::type_resource_failure("Palette");
    }
}
DynamicVectorClass<ColorScheme*>* YRPP_FASTCALL ColorScheme::GeneratePalette(char* name) noexcept {
    try {
        auto& state = palette_registry();
        const auto key = palette_key(name);
        auto* existing = palette_find(*state.Table, key);
        if (existing && existing->Value->Schemes) return existing->Value->Schemes;
        // Missing palettes remain nodes and are retried on the next request.
        // The target builds before lookup-insertion, including a null-node retry.
        std::unique_ptr<DynamicVectorClass<ColorScheme*>, PaletteDeleter> schemes(build_palette(state, name));
        if (!existing) {
            std::unique_ptr<SchemeNode> node(new SchemeNode{});
            std::strcpy(node->Name, name);
            node->Schemes = schemes.get();
            PaletteEntry entry{key, node.get()};
            auto& bucket = state.Table->Buckets[palette_hash(key) % state.Table->BucketCount];
            if (!bucket.AddItem(entry)) {
                destroy_palette(*node);
                schemes.release();
                throw std::bad_alloc();
            }
            node.release();
            schemes.release();
        } else {
            // Original duplicate insertion leaves the old node unchanged. The
            // newly built replacement is not installed when an entry exists.
            schemes.reset();
        }
        return palette_find(*state.Table, key)->Value->Schemes;
    } catch (...) {
        game::type_resource_failure(name);
        return nullptr;
    }
}
void YRPP_FASTCALL ColorScheme::SetPaletteTheater(TheaterType theater) noexcept {
    try {
        auto& state = palette_registry();
        if (state.Theater == theater) return;
        state.Theater = theater;
        for (int b = 0; b < state.Table->BucketCount; ++b) {
            auto& bucket = state.Table->Buckets[b];
            for (int i = 0; i < bucket.Count; ++i) {
                auto& node = *bucket.Items[i].Value;
                auto* replacement = build_palette(state, bucket.Items[i].Key.Name);
                destroy_palette(node);
                node.Schemes = replacement;
            }
        }
    } catch (...) {
        game::type_resource_failure("Palette");
    }
}
DynamicVectorClass<ColorScheme*>* YRPP_FASTCALL ColorScheme::GetPaletteSchemesFromIterator(HashIterator* it) noexcept {
    if (!palette_state || it->OutOfBuckets) return nullptr;
    auto& table = *palette_state->Table;
    while (it->BucketIndex < table.BucketCount) {
        auto& bucket = table.Buckets[it->BucketIndex];
        while (it->InBucketIndex < bucket.Count) {
            auto& entry = bucket.Items[it->InBucketIndex++];
            if (*entry.Key.Name) return palette_find(table, entry.Key)->Value->Schemes;
        }
        ++it->BucketIndex;
        it->InBucketIndex = 0;
    }
    it->OutOfBuckets = true;
    return nullptr;
}
int ColorScheme::CountLoadedPalettes() noexcept {
    try {
        palette_registry();
        int count = 0;
        HashIterator iterator{0, 0, false};
        // Original iteration stops at a null palette, even with later nodes.
        while (GetPaletteSchemesFromIterator(&iterator))
            ++count;
        return count;
    } catch (...) {
        game::type_resource_failure("Palette");
        return 0;
    }
}
void ColorScheme::ReleasePaletteCache() noexcept {
    if (!palette_state) return;
    for (int i = 0; i < palette_state->Names->Count; ++i)
        delete[] palette_state->Names->Items[i];
    for (int b = 0; b < palette_state->Table->BucketCount; ++b) {
        auto& bucket = palette_state->Table->Buckets[b];
        for (int i = 0; i < bucket.Count; ++i) {
            destroy_palette(*bucket.Items[i].Value);
            delete bucket.Items[i].Value;
        }
    }
    delete palette_state->Names;
    delete palette_state->Colors;
    delete[] palette_state->Table->Buckets;
    delete palette_state->Table;
    delete palette_state;
    palette_state = nullptr;
}
ColorScheme::ColorScheme(const char* id, const ColorStruct& baseColor, const BytePalette& palette, int shadeCount,
                         bool addToArray)
    : ColorScheme(id, baseColor, palette, palette, shadeCount, addToArray) {}
ColorScheme::~ColorScheme() {
    std::free(ID);
    ID = nullptr;
    delete LightConvert;
    LightConvert = nullptr;
    Array.Remove(this);
}
#endif
