#include "support/test_support.hpp"
#include "yrpp/TacticalClass.h"
#include "map_runtime.hpp" // Explicit test-only access to the projection binding.
#include <bit>
#include <cfenv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void check_reference() {
    std::ifstream input(RA2_MAP_PROJECTION_FIXTURE);
    EXPECT_TRUE((bool(input))) << "open original projection fixture";
    char kind;
    int count = 0;
    while (input >> kind) {
        if (kind == 'H') {
            int height, expected;
            input >> height >> expected;
            const int actual = TacticalClass::AdjustForZ(height);
            if (actual != expected) {
                std::cerr << "height=" << height << " expected=" << expected << " actual=" << actual << '\n';
                EXPECT_TRUE((false)) << "original height projection";
            }
        } else if (kind == 'P') {
            CoordStruct value;
            Point2D expected;
            input >> value.X >> value.Y >> value.Z >> expected.X >> expected.Y;
            const auto actual = TacticalClass::CoordsToScreen(value);
            EXPECT_TRUE((actual == expected)) << "original screen projection including wrapping";
            const auto flat = TacticalClass::AdjustForZShapeMove(value.X, value.Y);
            const auto preview = TacticalClass::CoordsToMapPixel(value.X, value.Y);
            EXPECT_TRUE((flat.Y == preview.Y &&
                std::uint32_t(flat.X) + 15360u == std::uint32_t(preview.X))) << "preview and tactical XY geometry agree";
        } else if (kind == 'C') {
            CoordStruct value;
            Point2D camera, expected, actual;
            RectangleStruct bounds{37, 53, 0, 0};
            int visible;
            input >> value.X >> value.Y >> value.Z >> camera.X >> camera.Y >>
                bounds.Width >> bounds.Height >> expected.X >> expected.Y >> visible;
            const bool result = TacticalClass::CoordsToClient(value, camera, bounds, actual);
            EXPECT_TRUE((actual == expected && result == bool(visible))) << "original client projection and culling edges";
        } else EXPECT_TRUE((false)) << "unknown projection fixture record";
        EXPECT_TRUE((bool(input))) << "complete fixture record";
        ++count;
    }
    EXPECT_TRUE((input.eof() && count > 4000)) << "all projection records consumed";
    std::cout << count << " original projection results passed\n";
}
void bindings() {
    EXPECT_TRUE((std::bit_cast<std::uint64_t>(*game::map_runtime().height_scale) ==
        UINT64_C(0x3fc25e5374344960))) << "calibrated startup bits";
    RectangleStruct out{1, 2, 3, 4};
    EXPECT_TRUE((!game::map_view_bounds(out) && out.X == 1 && out.Height == 4)) << "missing viewport preserves output";
    const double scale = 0.25;
    const RectangleStruct viewport{4, 5, 640, 400};
    const game::MapRuntimeServices service{&scale, &viewport};
    EXPECT_TRUE((game::with_map_runtime(service, [](void*) {
        RectangleStruct actual;
        EXPECT_TRUE((game::map_view_bounds(actual) && actual.X == 4 && actual.Width == 640)) << "bound viewport";
        EXPECT_TRUE((TacticalClass::AdjustForZ(104) == 26)) << "bound height scale";
        const double nested_scale = 0.5;
        EXPECT_TRUE((!game::with_map_runtime({&nested_scale, nullptr}, [](void*) {
            EXPECT_TRUE((TacticalClass::AdjustForZ(104) == 52)) << "nested projection binding";
            throw std::runtime_error("local callback failure");
        }, nullptr))) << "callback exception reports failure";
        EXPECT_TRUE((TacticalClass::AdjustForZ(104) == 26)) << "exception restores outer binding";
    }, nullptr))) << "successful scope";
    EXPECT_TRUE((TacticalClass::AdjustForZ(104) == 15)) << "scope restores default binding";
    bool called = false;
    auto operation = [](void* data) { *static_cast<bool*>(data) = true; };
    for (double invalid : {-1.0, 1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        EXPECT_TRUE((!game::with_map_runtime({&invalid, nullptr}, operation, &called) && !called)) << "invalid scale rejected before callback";
    EXPECT_TRUE((!game::with_map_runtime({}, operation, &called) && !called)) << "missing scale rejected";
    EXPECT_TRUE((!game::with_map_runtime(service, nullptr, nullptr))) << "missing operation rejected";
}
}

TEST(MapProjection, Contracts) {
    const int rounding = std::fegetround();
    check_reference();
    bindings();
    EXPECT_TRUE((std::fegetround() == rounding)) << "projection does not mutate host rounding mode";
}
