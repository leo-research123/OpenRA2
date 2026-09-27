#pragma once

// Map-only test support. Callers explicitly opt into core private headers.
#include "map_world_fixture.hpp"
#include <exception>
#include <functional>

namespace map_fixture {

inline void edit(const std::filesystem::path& path, const std::string& from, const std::string& to) {
    std::ifstream input(path, std::ios::binary);
    EXPECT_TRUE((bool(input))) << "fixture file exists";
    std::string text((std::istreambuf_iterator<char>(input)), {});
    const auto pos = text.find(from);
    if (pos == std::string::npos) throw std::runtime_error("fixture edit target absent: " + from);
    text.replace(pos, from.size(), to);
    std::ofstream output(path, std::ios::binary); output << text;
    EXPECT_TRUE((bool(output))) << "write fixture edit";
}

inline void session(const std::function<void(const std::filesystem::path&)>& prepare,
             const std::function<void(game::MapViewHandle&)>& observe) {
    auto root = std::filesystem::temp_directory_path() /
        ("ra2-fidelity-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove_all(path, error);
    } } cleanup{root};
    fixtures(root); prepare(root);
    game::ResourceHandle* raw = nullptr; std::string error;
    if (!game::create_resources(root.string(), raw, error)) throw std::runtime_error(error);
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> resources(raw, game::destroy_resources);
    game::MapViewHandle* view = nullptr;
    EXPECT_TRUE((game::create_map_view(*raw, view))) << "create native map view";
    struct Close { game::MapViewHandle*& value; ~Close() {
        game::destroy_map_view(value);
        // The existing file-only ResourceEnvironment contract leaves image
        // teardown to the host. Each test is a new host/resource directory.
        FileSystem::ClearNameCache(); Destroy_All_Shapes(); Unload_All_Shapes();
    } } close{view};
    if (!game::load_map_view(*view, "world.map", 9)) throw std::runtime_error(game::map_view_error(*view));
    observe(*view);
}

inline void native(game::MapViewHandle& view, const std::function<void()>& function) {
    struct Call { const std::function<void()>* function; std::exception_ptr error; } call{&function, {}};
    const bool ok = game::with_map_view(view, [](void* context) {
        auto& call = *static_cast<Call*>(context);
        try { (*call.function)(); } catch (...) { call.error = std::current_exception(); }
    }, &call);
    EXPECT_TRUE((ok)) << "with_map_view boundary";
    if (call.error) std::rethrow_exception(call.error);
}

inline BuildingClass& placed() {
    EXPECT_TRUE((BuildingClass::Array.Count == 1)) << "one synthetic building";
    return *BuildingClass::Array[0];
}

inline void turret_fixture(const std::filesystem::path& root) {
    edit(root/"RULESMD.INI", "[BLDG]\n", "[BLDG]\nTurret=yes\nTurretAnim=TURRET\nTurretAnimIsVoxel=no\n");
    std::ofstream(root/"ARTMD.INI", std::ios::app) << "\n[TURRET]\nStart=0\nEnd=32\nRate=1\nLoopEnd=32\nShadow=no\n";
    shp(root/"TURRET.SHP", 32);
}

template<class Draw>
const game::ShapeDrawingRequest& body_request(const Draw& draw) {
    auto image = placed().Type->Image;
    for (const auto& request : draw.shapes) if (request.image == image && !(request.flags & 1)) return request;
    throw std::runtime_error("body request absent");
}

} // namespace map_fixture
