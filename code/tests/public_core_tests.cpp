#include "support/test_support.hpp"
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
    std::filesystem::path path = std::filesystem::temp_directory_path() /
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
struct Observation {
    unsigned completed = 0, total = 0, calls = 0;
    bool cancel = false, cancel_after_language_md = false, invalid_order = false;
    static void progress(void* data, const char*, unsigned completed, unsigned total) noexcept {
        auto& state = *static_cast<Observation*>(data);
        state.invalid_order |= completed < state.completed || completed > total;
        state.completed = completed; state.total = total; ++state.calls;
        if (state.cancel_after_language_md && completed == 1) state.cancel = true;
    }
    static bool cancelled(void* data) noexcept { return static_cast<Observation*>(data)->cancel; }
    game::ResourceCallbacks callbacks() { return {this, progress, cancelled}; }
};
}

TEST(PublicCore, Contracts) {
    Fixture fixture;
    std::string error;
    game::ResourceHandle* output = nullptr;
    EXPECT_TRUE((!game::create_resources(fixture.directory() + "/absent", output, error) && !output && !error.empty())) << "creation failure must preserve null output";
    EXPECT_TRUE((game::create_resources(fixture.directory(), output, error) && error.empty())) << "create public resources";
    Resources resources(output, game::destroy_resources);
    EXPECT_TRUE((!game::create_resources(fixture.directory(), output, error) && output == resources.get())) << "non-null output must not be replaced";
    output = nullptr;
    EXPECT_TRUE((!game::create_resources(fixture.directory(), output, error) && !output)) << "single environment ownership";
    EXPECT_TRUE((!game::with_resources(*resources, nullptr, nullptr, error) && !error.empty())) << "null operation";
    struct Nested { game::ResourceHandle& resources; } nested{*resources};
    EXPECT_TRUE((game::with_resources(*resources, [](void* context) {
        auto& state = *static_cast<Nested*>(context);
        std::string error;
        EXPECT_TRUE((game::with_resources(state.resources, read_ini, nullptr, error))) << "nested resource operation";
        EXPECT_TRUE((!game::with_resources(state.resources, [](void*) {
            throw std::runtime_error("callback failure");
        }, nullptr, error) && error == "callback failure")) << "callback exception translation";
        read_ini(nullptr); // The nested exception must restore the outer context.
    }, &nested, error))) << "outer resource operation";

    Observation cancelled;
    cancelled.cancel_after_language_md = true;
    EXPECT_TRUE((game::load_resources(*resources, cancelled.callbacks(), error) == game::ResourceLoadResult::cancelled)) << "cancel between original language package entries";
    EXPECT_TRUE((cancelled.completed == 1 && !cancelled.invalid_order && error.empty())) << "cancel progress";
    EXPECT_TRUE((MixFileClass::Generics.LANGMD && !MixFileClass::Generics.LANGUAGE)) << "cancel preserves partial original state";
    Unload_All_Shapes();
    resources.reset();
    EXPECT_TRUE((!MixFileClass::MIXes.front() && !MixFileClass::Generics.LANGMD)) << "public teardown clears original state";

    EXPECT_TRUE((game::create_resources(fixture.directory(), output, error))) << "recreate after teardown";
    resources.reset(output);
    Observation completed;
    EXPECT_TRUE((game::load_resources(*resources, completed.callbacks(), error) == game::ResourceLoadResult::complete)) << "public startup completion";
    EXPECT_TRUE((completed.calls > 2 && completed.completed == 108 && completed.total == 108 && !completed.invalid_order)) << "public startup progress contract";
    EXPECT_TRUE((game::with_resources(*resources, read_ini, nullptr, error))) << "read after public startup";
    const uint8_t raw[] = {1, 2, 3, 4};
    std::vector<uint8_t> pixels;
    EXPECT_TRUE((game::decode_shp_pixels(raw, 2, 2, false, pixels, error) && pixels == std::vector<uint8_t>({1, 2, 3, 4}))) << "public image decode";
}
