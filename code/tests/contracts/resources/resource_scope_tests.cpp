#include <gtest/gtest.h>
// This target receives only ra2_core's public include directory.
#include "api/images.hpp"
#include "api/filesystem.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/Memory.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

using Resources = std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)>;
struct Fixture {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("ra2-public-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Fixture() {
        std::filesystem::create_directory(path);
        std::ofstream(path / "public-consumer.ini") << "[General]\nCount=42\n";
        for (const char* name : {"langmd.mix", "language.mix"}) {
            std::ofstream file(path / name, std::ios::binary);
            const char empty_mix[10]{};
            file.write(empty_mix, sizeof(empty_mix));
        }
    }
    ~Fixture() { std::filesystem::remove_all(path); }
    std::string directory() const {
        const auto utf8 = path.u8string();
        return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
    }
};
void read_ini(void*) {
    CCINIClass ini;
    EXPECT_TRUE((ini.LoadFromFile("public-consumer.ini") == 1)) << "public YRpp file access";
    EXPECT_TRUE((ini.ReadInteger("General", "Count", 0) == 42)) << "public YRpp INI result";
    CCFileClass file("public-consumer.ini");
    void* bytes = file.ReadWholeFile();
    EXPECT_TRUE((bytes != nullptr)) << "public YRpp owned file buffer";
    YRMemory::Deallocate(bytes);
}
struct Progress {
    unsigned completed = 0, total = 0;
    bool cancel = false, stop_after_progress = false, invalid_order = false;
    static void progress(void* raw, const char*, unsigned done, unsigned total) noexcept {
        auto& state = *static_cast<Progress*>(raw);
        state.invalid_order |= done < state.completed || done > total;
        state.completed = done;
        state.total = total;
        if (state.stop_after_progress && done) state.cancel = true;
    }
    static bool cancelled(void* raw) noexcept { return static_cast<Progress*>(raw)->cancel; }
    game::ResourceCallbacks callbacks() { return {this, progress, cancelled}; }
};
}
TEST(ResourceScope, OutputOwnershipAndNestedExceptions) {

    Fixture fixture;
    std::string error;
    game::ResourceHandle* output = nullptr;
    EXPECT_TRUE((!game::create_resources(fixture.directory() + "/absent", output, error) && !output && !error.empty()))
        << "creation failure must preserve null output";
    EXPECT_TRUE((game::create_resources(fixture.directory(), output, error) && error.empty()))
        << "create public resources";
    Resources resources(output, game::destroy_resources);
    EXPECT_TRUE((!game::create_resources(fixture.directory(), output, error) && output == resources.get()))
        << "non-null output must not be replaced";
    output = nullptr;
    EXPECT_TRUE((!game::create_resources(fixture.directory(), output, error) && !output))
        << "single environment ownership";
    EXPECT_TRUE((!game::with_resources(*resources, nullptr, nullptr, error) && !error.empty())) << "null operation";
    struct Nested {
        game::ResourceHandle& resources;
    } nested{*resources};
    EXPECT_TRUE((game::with_resources(
        *resources,
        [](void* context) {
            auto& state = *static_cast<Nested*>(context);
            std::string error;
            EXPECT_TRUE((game::with_resources(state.resources, read_ini, nullptr, error)))
                << "nested resource operation";
            EXPECT_TRUE(
                (!game::with_resources(
                     state.resources, [](void*) { throw std::runtime_error("callback failure"); }, nullptr, error) &&
                 error == "callback failure"))
                << "callback exception translation";
            read_ini(nullptr);  // The nested exception must restore the outer context.
        },
        &nested, error)))
        << "outer resource operation";


}
TEST(ResourceScope, CancelAndRestartKeepsMonotonicProgress) {
    Fixture fixture;
    std::string error;
    game::ResourceHandle* output = nullptr;
    ASSERT_TRUE(game::create_resources(fixture.directory(), output, error));
    Resources resources(output, game::destroy_resources);
    Progress cancelled;
    cancelled.stop_after_progress = true;
    EXPECT_EQ(game::load_resources(*resources, cancelled.callbacks(), error), game::ResourceLoadResult::cancelled);
    EXPECT_TRUE(cancelled.cancel && !cancelled.invalid_order && error.empty());
    Unload_All_Shapes();
    resources.reset();
    output = nullptr;
    ASSERT_TRUE(game::create_resources(fixture.directory(), output, error));
    resources.reset(output);
    Progress completed;
    EXPECT_EQ(game::load_resources(*resources, completed.callbacks(), error), game::ResourceLoadResult::complete);
    EXPECT_TRUE(completed.total > 0 && completed.completed == completed.total && !completed.invalid_order);
    Unload_All_Shapes();
}
TEST(ResourceScope, RawImageDecodeCopiesTheSuppliedPixels) {
    const std::uint8_t raw[] = {1, 2, 3, 4};
    std::vector<std::uint8_t> pixels;
    std::string error;
    ASSERT_TRUE(game::decode_shp_pixels(raw, 2, 2, false, pixels, error));
    EXPECT_EQ(pixels, (std::vector<std::uint8_t>{1, 2, 3, 4}));
}
