#include "support/test_support.hpp"
#include "filesystem/file_system.hpp"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"
#include "filesystem/search_paths.hpp"
#include "yrpp/CCFileClass.h"
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

struct Fixture {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("ra2-file-chain-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Fixture() { std::filesystem::create_directories(root / "search"); }
    ~Fixture() { std::error_code error; std::filesystem::remove_all(root, error); }
};
void platform() {
    Fixture fixture;
    game::FileSystem files(fixture.root);
    game::NativeFileHandle handle;
    EXPECT_TRUE((bool(files.open("MiXeD.bin", game::FileMode::write, game::FileCreation::truncate, handle, 0)))) << "platform create";
    std::size_t actual = 99;
    EXPECT_TRUE((bool(files.write(handle, "hello", 5, actual)) && actual == 5)) << "platform write";
    EXPECT_TRUE((bool(files.close(handle)))) << "platform close";
    EXPECT_TRUE((bool(files.open("mixed.BIN", game::FileMode::read_write, game::FileCreation::open_or_create, handle, 0)))) << "platform existing write";
    std::uint64_t size = 0;
    EXPECT_TRUE((bool(files.size(handle, size)) && size == 5)) << "read/write must not truncate";
    std::array<char, 8> bytes{};
    EXPECT_TRUE((bool(files.read(handle, bytes.data(), bytes.size(), actual)) && actual == 5 && std::string_view(bytes.data(), 5) == "hello")) << "short EOF read";
    EXPECT_TRUE((bool(files.read(handle, bytes.data(), bytes.size(), actual)) && actual == 0)) << "EOF count";
    EXPECT_TRUE((bool(files.close(handle)))) << "close read/write";
    EXPECT_TRUE((bool(files.open("mixed.bin", game::FileMode::read, game::FileCreation::existing, handle)))) << "read-only open";
    EXPECT_TRUE((!files.write(handle, "x", 1, actual) && actual == 0)) << "write failure status/count";
    files.close(handle);
    EXPECT_TRUE((bool(files.remove("MIXED.BIN")))) << "case-insensitive delete";
    EXPECT_TRUE((!files.open("missing", game::FileMode::read, game::FileCreation::existing, handle) && handle == game::invalid_file_handle())) << "missing output handle";
}
void buffered_and_search() {
    Fixture fixture;
    std::string payload(4096, '\0');
    for (std::size_t i = 0; i < payload.size(); ++i) payload[i] = char('A' + i % 23);
    { std::ofstream file(fixture.root / "search" / "buffer.bin", std::ios::binary); file.write(payload.data(), payload.size()); }
    game::ResourceEnvironment environment(fixture.root);
    game::ResourceScope scope({&environment, nullptr, {}});
    game::set_file_search_paths({"search"});
    CCFileClass file("BUFFER.BIN");
    EXPECT_TRUE((file.Exists() && file.Open(FileAccessMode::Read))) << "CD search path open";
    EXPECT_TRUE((file.Cache(1024))) << "BufferIO cache initialization";
    std::array<char, 3072> output{};
    EXPECT_TRUE((file.ReadBytes(output.data(), output.size()) == int(output.size()) && std::equal(output.begin(), output.end(), payload.begin()))) << "buffer read across three windows";
    EXPECT_TRUE((file.Seek(13, FileSeekMode::Set) == 13 && file.ReadBytes(output.data(), 29) == 29
        && std::equal(output.begin(), output.begin() + 29, payload.begin() + 13))) << "buffered seek/refill";
    file.Close(); file.Free();
    EXPECT_TRUE((!file.IOBuffer && !file.UseBuffer)) << "buffer release state";
    file.RawFileClass::Close(); // Cache borrowed an already-open disk handle.
    EXPECT_TRUE((file.Cache(1024) && file.Open(FileAccessMode::ReadWrite))) << "buffer read/write open";
    char patch[] = "PATCH";
    EXPECT_TRUE((file.Seek(0, FileSeekMode::Set) == 0 && file.WriteBytes(patch, 5) == 5 && file.IsChanged)) << "buffered write marks dirty range";
    EXPECT_TRUE((file.Commit() && !file.IsChanged)) << "buffer commit clears dirty state";
    file.Close(); file.Free();
    std::ifstream independent(fixture.root / "search" / "buffer.bin", std::ios::binary);
    std::string written((std::istreambuf_iterator<char>(independent)), {});
    EXPECT_TRUE((written.size() == payload.size() && written.substr(0, 5) == "PATCH" && written.substr(5) == payload.substr(5))) << "independent verification of buffered writeback";
}
}

TEST(FileChain, Contracts) {
    platform(); buffered_and_search();
}
