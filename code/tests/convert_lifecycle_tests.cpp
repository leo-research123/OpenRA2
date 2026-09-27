#include "support/test_support.hpp"
// Native object/ownership tests using production Drawing and Surface operations.
// Only the Alpha allocation-counting fixture and resource palette data below are
// test doubles; software_render_tests.cpp separately uses every real component.
#include "yrpp/ConvertClassHelpers.hpp"
#include "yrpp/BlitterVariants.hpp"
#include "yrpp/ArrayClasses.h"
#include "yrpp/DrawingBuffers.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include "memory_testing.hpp"

#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/FileSystem.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

namespace {

auto& converts = ConvertClass::Array;
auto& lights = LightConvertClass::Array;
std::map<int, std::unique_ptr<AlphaLightingRemapClass>> alpha_cache;
unsigned acquires = 0, releases = 0;
unsigned surface_queries = 0;
BytePalette palette;

void clean() {
    EXPECT_TRUE((game::outstanding_memory_allocations() == 0)) << "owned buffer or blitter leaked";
    EXPECT_TRUE((alpha_cache.empty() && acquires == releases)) << "Alpha reference leaked";
    EXPECT_TRUE((converts.Count == 0)) << "Convert registration leaked";
}
}
AlphaLightingRemapClass::AlphaLightingRemapClass(int count) noexcept
    : Table{}, IntensityCount(count), RefCount(0) {}
AlphaLightingRemapClass* YRPP_STDCALL AlphaLightingRemapClass::FindOrAllocate(int count) {
    auto& item = alpha_cache[count];
    if (!item) item = std::make_unique<AlphaLightingRemapClass>(count);
    ++acquires;
    ++item->RefCount;
    return item.get();
}
void YRPP_STDCALL AlphaLightingRemapClass::Release(AlphaLightingRemapClass* item) {
    if (!item) return;
    EXPECT_TRUE((item->RefCount > 0)) << "Alpha reference released twice";
    ++releases;
    if (!--item->RefCount) alpha_cache.erase(item->IntensityCount);
}
// Original resource state inputs, not replacement method definitions.
BytePalette& FileSystem::TEMPERAT_PAL = palette;
BytePalette& FileSystem::ISOx_PAL = palette;
namespace {
void native_slots_and_remap() {
    ConvertClass c(palette, palette, 1, 1, false);
    unsigned populated = 0;
    for (auto* b : c.Blitters) populated += b != nullptr;
    for (auto* b : c.RLEBlitters) populated += b != nullptr;
    EXPECT_TRUE((populated == 25 && converts.Count == 1)) << "8-bit construction slots";
    EXPECT_TRUE((dynamic_cast<BlitPlainXlat<BYTE>*>(c.Blitters[0]))) << "plain native dynamic type";
    EXPECT_TRUE((dynamic_cast<RLEBlitTransXlat<BYTE>*>(c.RLEBlitters[0]))) << "RLE native dynamic type";
    auto table = std::make_unique<std::array<BYTE, 256>>();
    table->fill(7);
    c.CurrentZRemap = table->data();
    if constexpr (sizeof(void*) > 4)
        EXPECT_TRUE((reinterpret_cast<std::uintptr_t>(c.CurrentZRemap) > UINT32_MAX)) << "64-bit pointer fixture";
    BYTE input[] = {1, 2};
    BYTE output[] = {99, 99};
    auto* remap = c.SelectPlainBlitter(static_cast<BlitterFlags>(0x10));
    remap->Blit_Copy(output, input, 2, 0, nullptr, nullptr, 1000, 0);
    const auto* colors = static_cast<const BYTE*>(c.PaletteData);
    EXPECT_TRUE((output[0] == colors[7] && output[1] == colors[7])) << "native remap read";
    table->fill(11);
    remap->Blit_Copy(output, input, 2, 0, nullptr, nullptr, 1000, 0);
    EXPECT_TRUE((output[0] == colors[11])) << "remap remains borrowed";
    auto replacement = std::make_unique<std::array<BYTE, 256>>();
    replacement->fill(19);
    c.CurrentZRemap = replacement->data();
    remap->Blit_Copy(output, input, 2, 0, nullptr, nullptr, 1000, 0);
    EXPECT_TRUE((output[0] == colors[19])) << "blitter follows the current remap pointer";
    BYTE rle[] = {1, 0, 1, 2};
    BYTE target[] = {99, 99, 99};
    c.RLEBlitters[0]->Blit_Copy(target, rle, 3, 0, 0, nullptr, nullptr, 1000, 0, nullptr);
    EXPECT_TRUE((target[0] == colors[1] && target[1] == 99 && target[2] == colors[2])) << "native RLE virtual call";
}
void shared_alpha_and_lazy_creation() {
    auto first = std::make_unique<ConvertClass>(palette, palette, 2, 3, false);
    for (auto* b : first->Blitters) EXPECT_TRUE((b != nullptr)) << "16-bit plain slot missing";
    for (auto* b : first->RLEBlitters) EXPECT_TRUE((b != nullptr)) << "16-bit RLE slot missing";
    EXPECT_TRUE((alpha_cache.size() == 1)) << "Alpha remaps were not shared";
    const int references = alpha_cache.begin()->second->RefCount;
    EXPECT_TRUE((references > 0)) << "Alpha references missing";
    auto second = std::make_unique<ConvertClass>(palette, palette, 2, 3, true);
    EXPECT_TRUE((!second->Blitters[0] && !second->RLEBlitters[0])) << "skipBlitters ignored";
    second->SelectPlainBlitter(static_cast<BlitterFlags>(0));
    EXPECT_TRUE((alpha_cache.begin()->second->RefCount == references * 2)) << "lazy initialization ownership";
    first.reset();
    EXPECT_TRUE((alpha_cache.begin()->second->RefCount == references)) << "first teardown lost shared Alpha";
    second.reset();
}
void derived_destruction() {
    const auto before = converts.Count;
    std::unique_ptr<ConvertClass> c = std::make_unique<LightConvertClass>(
        &palette, &palette, 2, 1000, 1000, 1000, false, nullptr, 53);
    auto* light = dynamic_cast<LightConvertClass*>(c.get());
    EXPECT_TRUE((light && light->IndexesToIgnore == LightConvertClass::DefaultIndexes)) << "native LightConvert construction";
    light->UpdateColors(600, 700, 800, true);
    EXPECT_TRUE((light->Tinted && light->Color2.Red == 600)) << "native LightConvert virtual call";
    c.reset();
    EXPECT_TRUE((converts.Count == before)) << "derived/base cleanup left a registry entry";
}
void supplied_storage_lifetimes() {
    alignas(LightConvertClass) std::byte storage[sizeof(LightConvertClass)];
    for (int pass = 0; pass != 2; ++pass) {
        auto* light = new (storage) LightConvertClass(&palette, &palette, 2,
            1000, 800, 600, false, nullptr, 27);
        ConvertClass* base = light;
        light->UpdateColors(400, 500, 600, true);
        EXPECT_TRUE((light->Tinted && converts.Count == 1)) << "placement constructor/virtual update";
        base->~ConvertClass(); // flags=0 semantics: keep caller storage for reuse.
        clean();
    }
    void* owned = YRMemory::Allocate(sizeof(ConvertClass));
    struct QuerySurface final : DSurface {
        QuerySurface() : DSurface(noinit_t{}) {}
        int GetBytesPerPixel() override { return 2; }
    } surface;
    auto* convert = game::construct_convert(owned, palette, palette, &surface, 7, false);
    const auto count = alpha_cache.begin()->second->RefCount;
    game::initialize_blitters(convert);
    EXPECT_TRUE((alpha_cache.begin()->second->RefCount == count)) << "rebuild leaked Alpha references";
    game::clear_blitters(convert);
    game::initialize_blitters(convert);
    EXPECT_TRUE((alpha_cache.begin()->second->RefCount == count)) << "clear/rebuild ownership mismatch";
    convert->~ConvertClass();
    YRMemory::Deallocate(convert);
    clean();
}
void allocation_failures() {
    for (unsigned index : {1u, 2u}) {
        game::fail_memory_allocation_after(index);
        bool failed = false;
        try { ConvertClass c(palette, palette, 1, 1, false); }
        catch (const std::bad_alloc&) { failed = true; }
        EXPECT_TRUE((failed)) << "palette allocation failure was not reported";
        game::fail_memory_allocation_after(0);
        clean();
    }
    game::fail_memory_allocation_after(2); // 16-bit palette succeeds; first blitter fails.
    {
        ConvertClass c(palette, palette, 2, 3, false);
        EXPECT_TRUE((!c.Blitters[0] && c.Blitters[1])) << "partial blitter construction state";
    }
    game::fail_memory_allocation_after(0);
    clean();
    for (const auto shades : {2u, 7u, 13u}) {
        bool failed = false;
        try { LightConvertClass light(&palette, &palette, 2, 1000, 1000, 1000, false, nullptr, shades); }
        catch (const std::invalid_argument&) { failed = true; }
        EXPECT_TRUE((failed)) << "invalid LightConvert shades must fail before allocation";
        clean();
    }
}
}
void public_blitter_tests();

TEST(ConvertLifecycle, Contracts) {
    for (unsigned i = 0; i < 256; ++i) palette.Entries[i] = ColorStruct(BYTE(i), BYTE(255-i), BYTE(i^73));
    std::fill_n(LightConvertClass::DefaultIndexes, 256, BYTE(1));
    public_blitter_tests(); clean();
    native_slots_and_remap(); clean();
    shared_alpha_and_lazy_creation(); clean();
    derived_destruction(); clean();
    supplied_storage_lifetimes(); clean();
    allocation_failures(); clean();
    EXPECT_TRUE((surface_queries == 0)) << "explicit pixel depth queried an original Surface";
    struct QuerySurface final : DSurface {
        QuerySurface() : DSurface(noinit_t{}) {}
        int GetBytesPerPixel() override { ++surface_queries; return 2; }
    } surface;
    {
        ConvertClass c(palette, palette, &surface, 1, true);
        LightConvertClass light(&palette, &palette, &surface, 1000, 1000, 1000, true, nullptr, 1);
    }
    EXPECT_TRUE((surface_queries == 2)) << "original-signature constructor delegation"; clean();
}
