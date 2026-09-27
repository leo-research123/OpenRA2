#include "support/test_support.hpp"
#include "yrpp/Memory.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/IndexClass.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {


void core_crt_operations() {
    for (std::size_t size : {0u, 1u, 16u, 255u, 65536u}) {
        auto* memory = YRMemory::AllocateZeroed(size);
        EXPECT_TRUE((memory != nullptr)) << "zero-size and zero-initialized allocation";
        const auto bytes = size ? size : 1;
        for (std::size_t i = 0; i < bytes; ++i)
            EXPECT_TRUE((static_cast<unsigned char*>(memory)[i] == 0)) << "calloc clears every byte";
        std::size_t capacity = 99;
#if defined(_WIN32) || defined(__APPLE__) || defined(__GLIBC__)
        EXPECT_TRUE((YRMemory::TryGetAllocationSize(memory, capacity) && capacity >= bytes)) << "live CRT block size query";
#else
        EXPECT_TRUE((!YRMemory::TryGetAllocationSize(memory, capacity) && capacity == 0)) << "unsupported size queries return explicit failure";
#endif
        std::memset(memory, 0x5a, bytes);
        auto* grown = YRMemory::Reallocate(memory, bytes + 32);
        EXPECT_TRUE((grown != nullptr)) << "core reallocation";
        for (std::size_t i = 0; i < bytes; ++i)
            EXPECT_TRUE((static_cast<unsigned char*>(grown)[i] == 0x5a)) << "growth preserves content";
        EXPECT_TRUE((!YRMemory::Reallocate(grown, 0))) << "zero reallocation frees";
        memory = YRMemory::Reallocate(nullptr, size);
        EXPECT_TRUE((memory != nullptr)) << "null realloc uses zero-normalized allocation";
        YRMemory::Deallocate(memory);
    }
    // All these blocks use the same CRT family, with no per-provider metadata.
    std::free(YRMemory::Allocate(23));
    YRMemory::Deallocate(std::malloc(29));
    YRMemory::Deallocate(nullptr);
}

struct Value {
    static inline int live = 0;
    static inline int destroyed = 0;
    static inline int order[128]{};
    int number;
    explicit Value(int initial = 17) : number(initial) { ++live; }
    Value(const Value& other) : number(other.number) { ++live; }
    Value& operator=(const Value&) = default;
    ~Value() {
        --live;
        if (destroyed < 128) order[destroyed++] = number;
    }
    bool operator==(const Value&) const = default;
};

struct alignas(64) AlignedValue {
    unsigned number = 42;
    bool operator==(const AlignedValue&) const = default;
};

void dll_arrays() {
    auto* integers = DLLCreateArray<int>(4);
    for (int i = 0; i < 4; ++i) EXPECT_TRUE((integers[i] == 0)) << "DLL scalar array initialization";
    DLLDeleteArray(integers, 4);
    auto* pointers = DLLCreateArray<void*>(3);
    for (int i = 0; i < 3; ++i) EXPECT_TRUE((pointers[i] == nullptr)) << "DLL pointer initialization";
    DLLDeleteArray(pointers, 3);
    auto* empty = DLLCreateArray<int>(0);
    DLLDeleteArray(empty, 0);
    DLLDeleteArray<int>(nullptr, 0);

    auto* values = DLLCreateArray<Value>(3, 42);
    EXPECT_TRUE((Value::live == 3)) << "DLL constructs every element";
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE((values[i].number == 42)) << "constructor arguments reused for each element";
        values[i].number = i + 1;
    }
    Value::destroyed = 0;
    DLLDeleteArray(values, 3);
    EXPECT_TRUE((Value::live == 0 && Value::destroyed == 3)) << "DLL destroys every element";
    EXPECT_TRUE((Value::order[0] == 1 && Value::order[1] == 2 && Value::order[2] == 3)) << "upstream forward destruction order";

    VectorClass<AlignedValue> aligned(3);
    EXPECT_TRUE((reinterpret_cast<std::uintptr_t>(aligned.Items) % alignof(AlignedValue) == 0)) << "DLL allocator preserves native element alignment";
    EXPECT_TRUE((aligned[2].number == 42)) << "DLL class array initialization";

    bool failed = false;
    try {
        core_crt_operations();
        auto* impossible = DLLCreateArray<std::uint64_t>(std::numeric_limits<std::size_t>::max());
        DLLDeleteArray(impossible, std::numeric_limits<std::size_t>::max());
    } catch (const std::bad_alloc&) { failed = true; }
    EXPECT_TRUE((failed)) << "DLL allocation failure must throw";
}

void vectors() {
    {
        VectorClass<Value> source(3);
        source[1].number = 91;
        VectorClass<Value> copy(source);
        EXPECT_TRUE((Value::live == 6 && copy.Items != source.Items && copy[1].number == 91)) << "vector copy owns distinct constructed elements";
        EXPECT_TRUE((copy.SetCapacity(5) && copy[1].number == 91 && copy[4].number == 17)) << "vector growth preserves values and initializes new elements";
        EXPECT_TRUE((Value::live == 8)) << "vector growth releases every old element";
        copy.Clear();
        EXPECT_TRUE((Value::live == 3)) << "vector clear destroys full capacity";
    }
    EXPECT_TRUE((Value::live == 0)) << "vector destruction releases remaining elements";
}

void game_arrays_and_index() {
    auto* values = GameCreateArray<Value>(4, 23);
    EXPECT_TRUE((Value::live == 4 && values[3].number == 23)) << "host Game provider constructs arrays";
    GameDeleteArray(values, 4);
    EXPECT_TRUE((Value::live == 0)) << "host Game provider destroys arrays";
    auto* empty = GameCreateArray<int>(0);
    GameDeleteArray(empty, 0);
    GameDeleteArray<int>(nullptr, 0);
    {
        Value value(99);
        IndexClass<int, Value> index;
        for (int i = 0; i < 11; ++i) EXPECT_TRUE((index.AddIndex(i, value))) << "index insertion";
        EXPECT_TRUE((index.IndexSize == 20 && index.IndexCount == 11 && Value::live == 21)) << "index growth destroys old array and retains new capacity";
        EXPECT_TRUE((index.IndexTable[10].Data.number == 99)) << "index growth preserves entries";
        index.Clear();
        EXPECT_TRUE((Value::live == 1 && !index.IndexTable && !index.IndexSize)) << "index clear uses array destruction, not scalar GameDelete";
    }
    EXPECT_TRUE((Value::live == 0)) << "index destruction leaves no live values";
}
}


TEST(YrppMemory, Contracts) {
    dll_arrays();
    vectors();
    game_arrays_and_index();
}
