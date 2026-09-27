#include "support/test_support.hpp"
// Deliberately public-only. CMake links ALL base-core objects, not just these calls.
#include "api/filesystem.hpp"
#include "api/images.hpp"
#include "api/map_view.hpp"
#include "yrpp/Surface.h"
#include "yrpp/FileFormats/SHP.h"
#include <iostream>

TEST(CoreFullLink, WholeArchiveAndPublicLifecycle) {
    BSurface surface(2, 2, 1);
    ASSERT_NE(surface.Lock(0, 0), nullptr);
    surface.Unlock();
    Unload_All_Shapes();
    game::ResourceHandle* resources = nullptr;
    std::string error;
    ASSERT_TRUE(game::create_resources(".", resources, error)) << error;
    game::MapViewHandle* view = nullptr;
    const bool created = game::create_map_view(*resources, view);
    game::destroy_map_view(view);
    game::destroy_resources(resources);
    EXPECT_TRUE(created && !view);
}
