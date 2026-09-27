#include "support/test_support.hpp"
// No environment doubles; every optional renderer object is forced into this link.
#include "yrpp/Drawing.h"
#include "yrpp/FileSystem.h"
#include "yrpp/PCX.h"
#include "yrpp/ConvertClass.h"
#include <iostream>

TEST(SoftwareRenderLink, WholeArchive) {
    FileSystem::ClearNameCache();
    ASSERT_TRUE(Drawing::SetColorMode(static_cast<RGBMode>(2)));
    EXPECT_EQ(LightConvertClass::InitLightConvert(1000, 1000, 1000), nullptr);
}
