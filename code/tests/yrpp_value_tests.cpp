#include "support/test_support.hpp"
#include "yrpp/ArrayClasses.h"
#include "yrpp/BasicStructures.h"
#include "yrpp/YRAllocator.h"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace {


void colors() {
    static_assert(sizeof(ColorStruct) == 3);
    static_assert(sizeof(Color16Struct) == 2);
    static_assert(sizeof(BytePalette) == 768);
    static_assert(sizeof(TintStruct) == 12);
    // Every RGB565 value must survive expansion to RGB888 and repacking.
    for (unsigned value = 0; value <= 0xffff; ++value) {
        const auto packed = static_cast<WORD>(value);
        const Color16Struct color16(packed);
        const ColorStruct color(color16);
        EXPECT_TRUE((static_cast<WORD>(color) == packed)) << "RGB565 round trip";
        EXPECT_TRUE((ColorStruct(packed) == color)) << "WORD constructor";
        EXPECT_TRUE((Color16Struct(color) == color16)) << "RGB888 constructor";
        EXPECT_TRUE((Color16Struct(static_cast<DWORD>(color)) == color16)) << "DWORD constructor";
        EXPECT_TRUE((static_cast<DWORD>(color16) == static_cast<DWORD>(color))) << "DWORD conversion";
    }
    ColorStruct color(240, 10, 0);
    const ColorStruct delta(30, 20, 255);
    const ColorStruct sum = color + delta;
    color += delta;
    EXPECT_TRUE((color == ColorStruct(255, 30, 255) && color == sum)) << "saturating addition";
    EXPECT_TRUE((color != delta && Color16Struct(color) != Color16Struct(delta))) << "color inequality";
    EXPECT_TRUE((ColorStruct(static_cast<DWORD>(color)) == color)) << "RGB888 round trip";
    BytePalette palette{};
    palette[255] = color;
    const BytePalette& view = palette;
    EXPECT_TRUE((&view[255] == &palette.Entries[255] && view[255] == color)) << "palette reference access";
    const TintStruct tint(1, 2, 3);
    EXPECT_TRUE((tint == TintStruct(1, 2, 3) && tint != TintStruct(3, 2, 1))) << "tint equality";
    // Preserve the existing any-component comparison, not lexicographic order.
    EXPECT_TRUE((TintStruct(9, 1, 9) < tint && !(tint < tint))) << "tint comparison contract";
}

void buffers() {
    MemoryBuffer owned(32);
    EXPECT_TRUE((owned.Buffer && owned.Size == 32 && owned.Allocated)) << "owned buffer allocation";
    static_cast<unsigned char*>(owned.Buffer)[0] = 42;
    void* storage = owned.Buffer;
    MemoryBuffer borrowed(owned);
    EXPECT_TRUE((borrowed.Buffer == storage && !borrowed.Allocated)) << "copy borrows storage";
    MemoryBuffer moved(std::move(owned));
    EXPECT_TRUE((moved.Buffer == storage && moved.Allocated && !owned.Allocated)) << "move transfers ownership";
    MemoryBuffer assigned(8);
    assigned = borrowed;
    EXPECT_TRUE((assigned.Buffer == storage && !assigned.Allocated)) << "copy assignment releases previous storage";
    assigned = assigned;
    EXPECT_TRUE((assigned.Buffer == storage && assigned.Size == 32)) << "copy self assignment";
    MemoryBuffer destination(8);
    destination = std::move(moved);
    EXPECT_TRUE((destination.Buffer == storage && destination.Allocated && !moved.Allocated)) << "move assignment transfers ownership";
    borrowed.Clear();
    EXPECT_TRUE((!borrowed.Buffer && !borrowed.Size && !borrowed.Allocated)) << "borrowed clear";
    EXPECT_TRUE((static_cast<unsigned char*>(destination.Buffer)[0] == 42)) << "borrowed clear kept owned storage";
    destination.Clear();
    EXPECT_TRUE((!destination.Buffer && !destination.Size && !destination.Allocated)) << "owned clear";
    unsigned char external[4]{};
    MemoryBuffer external_view(external, 4);
    EXPECT_TRUE((external_view.Buffer == external && !external_view.Allocated)) << "external storage stays borrowed";
    MemoryBuffer empty;
    MemoryBuffer zero(0);
    EXPECT_TRUE((!empty.Buffer && !zero.Buffer && !zero.Allocated)) << "empty buffers";
}

void counters() {
    CounterClass counter;
    EXPECT_TRUE((counter.GetTotal() == 0 && counter.Increment(15) == 1)) << "counter growth";
    EXPECT_TRUE((counter.Increment(15) == 2 && counter.Decrement(15) == 1)) << "counter update";
    EXPECT_TRUE((counter.GetItemCount(14) == 0 && counter.GetTotal() == 1)) << "new slots initialized";
    CounterClass copied(counter);
    EXPECT_TRUE((copied.Items != counter.Items && copied[15] == 1)) << "counter deep copy";
    CounterClass moved(std::move(copied));
    CounterClass assigned;
    assigned = counter;
    CounterClass destination;
    destination = std::move(assigned);
    EXPECT_TRUE((destination.GetTotal() == 1 && moved.GetTotal() == 1)) << "counter assignments";
    counter.Clear();
    EXPECT_TRUE((counter.GetTotal() == 0 && counter[15] == 0)) << "counter clear";
    counter.Swap(destination);
    const CounterClass& view = counter;
    EXPECT_TRUE((view.GetItemCount(15) == 1 && view.GetItemCount(100) == 0 && destination.GetTotal() == 0)) << "counter swap and const lookup";
}
}


TEST(YrppValue, Contracts) {
    colors();
    buffers();
    counters();
}
