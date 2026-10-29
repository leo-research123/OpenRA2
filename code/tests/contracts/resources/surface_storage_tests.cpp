#include <gtest/gtest.h>
#include "yrpp/Surface.h"
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
void owned_and_borrowed() {
    for (int depth : {1, 2}) {
        BSurface owned(7, 3, depth);
        Surface& surface = owned;
        EXPECT_TRUE((surface.GetWidth() == 7 && surface.GetHeight() == 3 && surface.GetBytesPerPixel() == depth &&
                     surface.GetPitch() == 7 * depth))
            << "dimensions/pixel depth/pitch";
        EXPECT_TRUE((owned.Buffer.Allocated && owned.Buffer.Size == 21 * depth && owned.Buffer.Buffer))
            << "owned pixels";
        auto bounds = surface.GetRect();
        EXPECT_TRUE((bounds.X == 0 && bounds.Y == 0 && bounds.Width == 7 && bounds.Height == 3)) << "surface bounds";
        RectangleStruct output{1, 2, 3, 4};
        EXPECT_TRUE((surface.GetRect(&output) == &output && output.Width == 7)) << "rectangle output pointer";
        auto* pixels = static_cast<unsigned char*>(surface.Lock(0, 0));
        EXPECT_TRUE((pixels && surface.IsLocked() && owned.LockLevel == 1)) << "first lock";
        auto* last = static_cast<unsigned char*>(surface.Lock(6, 2));
        EXPECT_TRUE((last == pixels + 20 * depth && owned.LockLevel == 2)) << "nested lock byte offset";
        for (int i = 0; i < depth; ++i)
            last[i] = static_cast<unsigned char>(91 + i);
        EXPECT_TRUE((pixels[20 * depth] == 91)) << "writes target actual buffer";
        EXPECT_TRUE((surface.Unlock() && surface.IsLocked())) << "nested unlock";
        EXPECT_TRUE((surface.Unlock() && !surface.IsLocked())) << "balanced unlock";
        std::array<unsigned char, 42> external{};
        {
            auto borrowed = std::make_unique<BSurface>(7, 3, depth, external.data());
            EXPECT_TRUE((!borrowed->Buffer.Allocated && borrowed->Buffer.Buffer == external.data()))
                << "borrowed pixels";
            auto* location = static_cast<unsigned char*>(borrowed->Lock(6, 2));
            *location = 123;
            borrowed->Unlock();
            std::unique_ptr<Surface> polymorphic = std::move(borrowed);
        }
        EXPECT_TRUE((external[20 * depth] == 123)) << "virtual destruction preserves borrowed buffer";
    }
}
void storage_failures() {
    BSurface empty(0, 0, 1);
    EXPECT_TRUE((!empty.Buffer.Buffer && !empty.Buffer.Allocated && !empty.Lock(0, 0)))
        << "empty borrowed/default storage";
    empty.Unlock();
    for (const auto& dimensions : {std::array<int, 3>{-1, 1, 1}, {1, -1, 1}, {1, 1, 3}}) {
        bool rejected = false;
        try {
            BSurface invalid(dimensions[0], dimensions[1], dimensions[2]);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        EXPECT_TRUE((rejected)) << "unsupported software dimensions/depth rejected";
    }
    bool rejected = false;
    try {
        BSurface oversized(std::numeric_limits<int>::max(), 2, 2);
    } catch (const std::length_error&) {
        rejected = true;
    }
    EXPECT_TRUE((rejected)) << "buffer and pitch integer bounds";
}
}

TEST(SurfaceStorage, OwnedAndBorrowedBuffers) { owned_and_borrowed(); }
TEST(SurfaceStorage, RejectsInvalidAndOverflowingDimensions) { storage_failures(); }
