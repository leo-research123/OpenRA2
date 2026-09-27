#include "support/test_support.hpp"
// Public-only lifecycle regression: host orchestration, not filesystem side effects.
#include "api/filesystem.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

struct Fixture {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("ra2-shutdown-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Fixture() { std::filesystem::create_directory(root); write(7); }
    ~Fixture() { std::error_code ec; std::filesystem::remove_all(root, ec); }
    void write(unsigned char value) {
        std::array<unsigned char, 33> bytes{};
        bytes[2] = bytes[4] = bytes[6] = bytes[12] = bytes[14] = 1;
        bytes[28] = 32; bytes[32] = value;
        std::ofstream f(root / "frame.shp", std::ios::binary);
        f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        EXPECT_TRUE((bool(f))) << "write SHP fixture";
    }
    std::string path() const { auto p = root.u8string(); return {reinterpret_cast<const char*>(p.data()), p.size()}; }
};
struct FilesDeleter {
    void operator()(game::ResourceHandle* handle) const noexcept {
        if (!handle) return;
        Unload_All_Shapes(); // outer host guarantee for failure as well as success
        game::destroy_resources(handle);
    }
};
using Files = std::unique_ptr<game::ResourceHandle, FilesDeleter>;
Files open(const Fixture& fixture) {
    game::ResourceHandle* output = nullptr;
    std::string error;
    EXPECT_TRUE((game::create_resources(fixture.path(), output, error))) << error.c_str();
    return Files(output);
}
struct Use {
    SHPReference* reference = nullptr;
    bool persistent = false;
    unsigned char expected = 7;
    bool fail_after_load = false;
};
void use(void* pointer) {
    auto& state = *static_cast<Use*>(pointer);
    if (!state.reference) state.reference = new SHPReference("frame.shp");
    if (state.persistent) state.reference->Load();
    auto* data = state.reference->GetData();
    EXPECT_TRUE((data && reinterpret_cast<unsigned char*>(data)[32] == state.expected)) << "SHP bytes";
    if (state.fail_after_load) throw std::runtime_error("after image use");
}
void run_case(bool persistent, bool fail) {
    Fixture fixture;
    Use state; state.persistent = persistent; state.fail_after_load = fail;
    auto files = open(fixture);
    std::string error;
    bool ok = game::with_resources(*files, use, &state, error);
    std::unique_ptr<SHPReference> reference(state.reference);
    EXPECT_TRUE((ok != fail)) << "operation exception status";
    const int index = reference->Index;
    Unload_All_Shapes();
    EXPECT_TRUE((!reference->Loaded && !reference->Data && reference->Index == index)) << "unload must not delete caller-owned reference";
    Unload_All_Shapes(); // idempotent, no double release
    files.reset();
    fixture.write(19);
    files = open(fixture);
    state.expected = 19; state.fail_after_load = false;
    EXPECT_TRUE((game::with_resources(*files, use, &state, error))) << error.c_str();
    EXPECT_TRUE((reference->Index == index)) << "reference survives file-session turnover";
    files.reset();
}
}

class ResourceShutdown : public testing::TestWithParam<std::tuple<bool, bool>> {};
TEST_P(ResourceShutdown, OwnershipAndReload) {
    const auto [persistent, fail] = GetParam();
    run_case(persistent, fail);
}
INSTANTIATE_TEST_SUITE_P(StorageAndFailure, ResourceShutdown,
    testing::Combine(testing::Values(false, true), testing::Values(false, true)));
