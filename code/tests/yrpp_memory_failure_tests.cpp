#include "support/test_support.hpp"
#include "yrpp/Memory.h"
#include "memory_testing.hpp"
#include "yrpp/YRAllocator.h"
#include "yrpp/Surface.h"
#include "filesystem/file_names.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void contracts() {
    game::fail_memory_allocation_after(1);
    EXPECT_TRUE((!YRMemory::Allocate(17))) << "unchecked failure returns null";
    game::fail_memory_allocation_after(1);
    EXPECT_TRUE((!YRMemory::AllocateZeroed(19))) << "zero allocation failure returns null";
    auto* memory = YRMemory::Allocate(32);
    EXPECT_TRUE((memory != nullptr)) << "allocation recovers after injected failure";
    std::memset(memory, 0x5a, 32);
    game::fail_memory_allocation_after(1);
    EXPECT_TRUE((!YRMemory::Reallocate(memory, 64))) << "reallocation failure returns null";
    EXPECT_TRUE((game::outstanding_memory_allocations() == 1)) << "failed reallocation retains ownership";
    for (int i = 0; i < 32; ++i)
        EXPECT_TRUE((static_cast<unsigned char*>(memory)[i] == 0x5a)) << "failed reallocation retains data";
    memory = YRMemory::Reallocate(memory, 64);
    EXPECT_TRUE((memory && game::outstanding_memory_allocations() == 1)) << "successful realloc replaces ownership";
    YRMemory::Deallocate(memory);
    game::fail_memory_allocation_after(1);
    EXPECT_TRUE((!game::DuplicateName("test"))) << "name duplication preserves null failure";
    {
        game::fail_memory_allocation_after(1);
        MemoryBuffer failed(8);
        EXPECT_TRUE((!failed.Buffer && failed.Size == 8 && failed.Allocated)) << "MemoryBuffer failure retains original size/ownership flag";
    }
    unsigned char external[8]{};
    {
        game::fail_memory_allocation_after(1);
        BSurface failed(2, 2, 1);
        EXPECT_TRUE((!failed.Buffer.Buffer && failed.Buffer.Allocated && failed.Buffer.Size == 4)) << "BSurface preserves MemoryBuffer allocation-failure ownership state";
        EXPECT_TRUE((failed.Lock(0, 0) == nullptr && failed.LockLevel == 1)) << "failed pixel allocation still follows original lock count";
        failed.Unlock();
    }
    {
        const auto attempts = game::memory_allocation_attempts();
        MemoryBuffer borrowed(external, 8);
        EXPECT_TRUE((!borrowed.Allocated && game::memory_allocation_attempts() == attempts)) << "borrowed MemoryBuffer does not allocate";
        borrowed.Clear();
        EXPECT_TRUE((!borrowed.Buffer && !borrowed.Size)) << "borrowed clear does not free external storage";
    }
    {
        MemoryBuffer owned(16);
        auto* original = owned.Buffer;
        EXPECT_TRUE((original && owned.Allocated)) << "owned MemoryBuffer allocation";
        MemoryBuffer moved(std::move(owned));
        EXPECT_TRUE((moved.Buffer == original && moved.Allocated && !owned.Allocated)) << "move transfers the release obligation";
        MemoryBuffer replacement(4);
        replacement = std::move(moved);
        EXPECT_TRUE((replacement.Buffer == original && replacement.Allocated && !moved.Allocated)) << "move assignment frees replaced buffer and transfers ownership";
    }
    EXPECT_TRUE((game::outstanding_memory_allocations() == 0)) << "memory/name/buffer allocations all paired";
    std::cout << "PASS: real CRT failures, retained realloc data, names and MemoryBuffer ownership\n";
}
}

TEST(YrppMemoryFailure, AllocationContracts) {
    contracts();
}

namespace {
std::optional<int> legacy_command(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--contracts") == 0) return std::nullopt;
    if (argc == 1 && !ra2::test::gtest_requested()) {
        // Exercise the real checked termination path, as in the original probe.
        game::fail_memory_allocation_after(1);
        YRMemory::AllocateChecked(37);
        return 0;
    }
    return std::nullopt;
}
const ra2::test::CommandRegistration command(legacy_command);
}
