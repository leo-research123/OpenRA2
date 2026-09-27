#include "support/test_support.hpp"
// Optional PCX dictionary lifetime and original non-owning name-tree contract.
#include "api/filesystem.hpp"
#include "yrpp/FileSystem.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/Memory.h"
#include "yrpp/PCX.h"
#include "yrpp/Surface.h"
#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

void operation(void*) {
    auto* first=FileSystem::LoadFile("cache.bin",false);
    EXPECT_TRUE((first && FileSystem::LoadFile("CACHE.BIN",false)==first)) << "case-folded name cache";
    FileSystem::InvalidateName("cache.bin");
    auto* next=FileSystem::LoadFile("cache.bin",false);
    EXPECT_TRUE((next && next!=first)) << "invalidated value must be reread";
    FileSystem::ClearNameCache();
    EXPECT_TRUE((static_cast<char*>(first)[0]=='x' && static_cast<char*>(next)[0]=='x')) << "tree borrows values";
    YRMemory::Deallocate(first); YRMemory::Deallocate(next);
    EXPECT_TRUE((PCX::Instance.LoadFile("cache.pcx", 1, 0))) << "global cache load";
    auto* global = PCX::Instance.GetSurface("cache.pcx");
    EXPECT_TRUE((global && PCX::Instance.Count == 1)) << "global cache populated";
    {
        PCX local;
        EXPECT_TRUE((local.Count==0 && local.TableSize==128 && local.Buffer)) << "host-width PCX dictionary";
        EXPECT_TRUE((!local.GetSurface("absent.pcx"))) << "empty lookup";
        EXPECT_TRUE((local.ForceLoadFile("cache.pcx", 1, 0))) << "independent cache load";
        EXPECT_TRUE((local.Count == 1 && local.GetSurface("cache.pcx") != global)) << "independent owned surfaces";
        EXPECT_TRUE((local.ForceLoadFile("cache.pcx", 1, 0) && local.Count == 1)) << "replacement releases old surface";
    } // Destroy populated local buckets/surface; global cache survives.
    auto* pixels = static_cast<unsigned char*>(global->Lock(0, 0));
    EXPECT_TRUE((pixels && pixels[0] == 7 && pixels[1] == 11)) << "global surface survives local destruction";
    global->Unlock();
    // The real static PCX instance is destroyed at process exit after resources.
    // ASan checks that final ownership path as well as the local destruction.

}
}

TEST(ImageCache, Contracts) {
    const auto path=std::filesystem::temp_directory_path()/
        ("ra2-cache-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    game::ResourceHandle* files=nullptr;
    int status=0;
    const auto cleanup = ra2::test::scope_exit([&] {
        FileSystem::ClearNameCache(); Unload_All_Shapes(); game::destroy_resources(files);
        std::error_code ec; std::filesystem::remove_all(path,ec);
    });

        std::filesystem::create_directory(path); std::ofstream(path/"cache.bin")<<"xyz";
        std::vector<unsigned char> pcx(128 + 2048 + 768, 0);
        pcx[0] = 10; pcx[1] = 5; pcx[2] = 1; pcx[3] = 8;
        pcx[8] = 1; pcx[65] = 1; pcx[66] = 2;
        pcx[128] = 7; pcx[129] = 11;
        std::ofstream image(path/"cache.pcx", std::ios::binary);
        image.write(reinterpret_cast<const char*>(pcx.data()), pcx.size()); image.close();
        auto utf8=path.u8string(); std::string error;
        EXPECT_TRUE((game::create_resources({reinterpret_cast<const char*>(utf8.data()),utf8.size()},files,error))) << error.c_str();
        EXPECT_TRUE((game::with_resources(*files,operation,nullptr,error))) << error.c_str();
}
