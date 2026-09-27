#include "support/test_support.hpp"
#include "yrpp/Surface.h"
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

namespace {

void owned_and_borrowed() {
    for (int depth : {1, 2}) {
        BSurface owned(7, 3, depth);
        Surface& surface = owned;
        EXPECT_TRUE((surface.GetWidth() == 7 && surface.GetHeight() == 3 &&
            surface.GetBytesPerPixel() == depth && surface.GetPitch() == 7 * depth)) << "dimensions/pixel depth/pitch";
        EXPECT_TRUE((owned.Buffer.Allocated && owned.Buffer.Size == 21 * depth && owned.Buffer.Buffer)) << "owned pixels";
        auto bounds = surface.GetRect();
        EXPECT_TRUE((bounds.X == 0 && bounds.Y == 0 && bounds.Width == 7 && bounds.Height == 3)) << "surface bounds";
        RectangleStruct output{1,2,3,4};
        EXPECT_TRUE((surface.GetRect(&output) == &output && output.Width == 7)) << "rectangle output pointer";
        EXPECT_TRUE((surface.CanLock() && surface.vt_entry_68(0, 0) && !surface.IsDSurface())) << "software surface queries";
        auto* pixels = static_cast<unsigned char*>(surface.Lock(0, 0));
        EXPECT_TRUE((pixels && surface.IsLocked() && owned.LockLevel == 1)) << "first lock";
        auto* last = static_cast<unsigned char*>(surface.Lock(6, 2));
        EXPECT_TRUE((last == pixels + 20 * depth && owned.LockLevel == 2)) << "nested lock byte offset";
        for (int i = 0; i < depth; ++i) last[i] = static_cast<unsigned char>(91 + i);
        EXPECT_TRUE((pixels[20 * depth] == 91)) << "writes target actual buffer";
        EXPECT_TRUE((surface.Unlock() && surface.IsLocked())) << "nested unlock";
        EXPECT_TRUE((surface.Unlock() && !surface.IsLocked())) << "balanced unlock";
        std::array<unsigned char, 42> external{};
        {
            auto borrowed = std::make_unique<BSurface>(7, 3, depth, external.data());
            EXPECT_TRUE((!borrowed->Buffer.Allocated && borrowed->Buffer.Buffer == external.data())) << "borrowed pixels";
            auto* location = static_cast<unsigned char*>(borrowed->Lock(6, 2));
            *location = 123;
            borrowed->Unlock();
            std::unique_ptr<Surface> polymorphic = std::move(borrowed);
        }
        EXPECT_TRUE((external[20 * depth] == 123)) << "virtual destruction preserves borrowed buffer";
    }
    BSurface defaults;
    EXPECT_TRUE((defaults.Width == 640 && defaults.Height == 400 && defaults.BytesPerPixel == 2)) << "existing default dimensions";
    BSurface two_arguments(2, 3);
    EXPECT_TRUE((two_arguments.Buffer.Size == 12)) << "existing two-argument constructor";
}
void lock_query_order() {
    class SwitchingSurface : public BSurface {
    public:
        SwitchingSurface(void* first, void* second, bool during_pitch)
            : BSurface(3, 2, 2, first), second_(second), during_pitch_(during_pitch) {}
        int GetBytesPerPixel() override {
            EXPECT_TRUE((LockLevel == 1 && queries_++ == 0)) << "lock count precedes pixel-depth query";
            if (!during_pitch_) Buffer.Buffer = second_;
            return BSurface::GetBytesPerPixel();
        }
        int GetPitch() override {
            EXPECT_TRUE((queries_++ == 1)) << "pixel-depth query precedes pitch query";
            if (during_pitch_) Buffer.Buffer = second_;
            return BSurface::GetPitch();
        }
    private:
        void* second_;
        bool during_pitch_;
        int queries_ = 0;
    };
    for (bool during_pitch : {false, true}) {
        std::array<unsigned char, 12> first{}, second{};
        SwitchingSurface surface(first.data(), second.data(), during_pitch);
        auto* pixel = static_cast<unsigned char*>(surface.Lock(2, 1));
        EXPECT_TRUE((pixel == first.data() + 10 && surface.Buffer.Buffer == second.data())) << "Lock uses buffer captured before virtual queries, preserving query side effects";
        *pixel = 73;
        EXPECT_TRUE((first[10] == 73 && second[10] == 0)) << "write reaches the originally captured buffer";
        EXPECT_TRUE((surface.Unlock() && !surface.IsLocked())) << "query side effects preserve lock balance";
    }
}
void counter_and_failures() {
    XSurface base(9, 5);
    EXPECT_TRUE((base.Lock(0, 0) == nullptr && base.LockLevel == 1)) << "original XSurface lock only increments";
    base.Unlock();
    EXPECT_TRUE((base.Unlock() && base.LockLevel == -1 && base.IsLocked())) << "original unmatched unlock is not clamped";
    base.Lock(0, 0);
    EXPECT_TRUE((!base.IsLocked())) << "counter returns to zero";
    BSurface empty(0, 0, 1);
    EXPECT_TRUE((!empty.Buffer.Buffer && !empty.Buffer.Allocated && !empty.Lock(0, 0))) << "empty borrowed/default storage";
    empty.Unlock();
    for (const auto& dimensions : {std::array<int,3>{-1, 1, 1}, {1, -1, 1}, {1, 1, 3}}) {
        bool rejected = false;
        try { BSurface invalid(dimensions[0], dimensions[1], dimensions[2]); }
        catch (const std::invalid_argument&) { rejected = true; }
        EXPECT_TRUE((rejected)) << "unsupported software dimensions/depth rejected";
    }
    bool rejected = false;
    try { BSurface oversized(std::numeric_limits<int>::max(), 2, 2); }
    catch (const std::length_error&) { rejected = true; }
    EXPECT_TRUE((rejected)) << "buffer and pitch integer bounds";
}
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(Surface) == 0x0c && sizeof(XSurface) == 0x14 && sizeof(BSurface) == 0x20);
static_assert(offsetof(XSurface, LockLevel) == 0x0c && offsetof(XSurface, BytesPerPixel) == 0x10);
static_assert(offsetof(BSurface, Buffer) == 0x14);
void original_virtual_slots() {
    alignas(BSurface) std::byte storage[sizeof(BSurface)];
    auto* surface = new (storage) BSurface(3, 2, 2);
    auto** slots = *reinterpret_cast<void***>(surface);
    auto lock = reinterpret_cast<void* (__thiscall*)(Surface*, int, int)>(slots[0x5c / 4]);
    auto unlock = reinterpret_cast<bool (__thiscall*)(Surface*)>(slots[0x60 / 4]);
    auto pitch = reinterpret_cast<int (__thiscall*)(Surface*)>(slots[0x74 / 4]);
    auto bounds = reinterpret_cast<RectangleStruct* (__thiscall*)(Surface*, RectangleStruct*)>(slots[0x78 / 4]);
    RectangleStruct rectangle{};
    EXPECT_TRUE((lock(surface, 2, 1) == static_cast<char*>(surface->Buffer.Buffer) + 10 && pitch(surface) == 6)) << "original thiscall Lock/Pitch slots";
    EXPECT_TRUE((unlock(surface) && bounds(surface, &rectangle) == &rectangle && rectangle.Height == 2)) << "original thiscall Unlock/Rect slots";
    auto destroy = reinterpret_cast<void* (__thiscall*)(Surface*, unsigned)>(slots[0]);
    EXPECT_TRUE((destroy(surface, 0) == surface)) << "original deleting-destructor flag zero retains external object storage";
    // Flags=1 frees the native C++ object exactly once through its generated slot.
    auto* heap = new BSurface(1, 1, 1);
    slots = *reinterpret_cast<void***>(heap);
    reinterpret_cast<void* (__thiscall*)(Surface*, unsigned)>(slots[0])(heap, 1);
}
#endif
}

TEST(Surface, Contracts) {
    owned_and_borrowed(); lock_query_order(); counter_and_failures();
    #if defined(_MSC_VER) && defined(_M_IX86)
            original_virtual_slots();
    #endif
}
