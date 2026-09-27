#include "support/test_support.hpp"
#include "yrpp/SwizzleManagerClass.h"
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <limits>
#include <type_traits>
#include <vector>

namespace {
void* pointer(std::uint32_t value) { return reinterpret_cast<void*>(std::uintptr_t(value)); }
LONG identity(std::uint32_t value) { return std::bit_cast<LONG>(value); }
}
TEST(Swizzle, AllComEntries) {
    static_assert(!std::has_virtual_destructor_v<SwizzleManagerClass>);
    static_assert(sizeof(LONG)==4);
    SwizzleManagerClass manager;
    EXPECT_EQ(manager.Swizzles_Old.CapacityIncrement,1000);
    EXPECT_EQ(manager.Swizzles_New.CapacityIncrement,1000);
    EXPECT_EQ(manager.Swizzles_Old.Capacity,0);
    EXPECT_EQ(manager.Swizzles_New.Capacity,0);
    EXPECT_EQ(manager.AddRef(),1u); EXPECT_EQ(manager.Release(),1u);
    constexpr GUID iid{0x5FF0CA70,0x8B12,0x11D1,{0xB7,0x08,0,0xA0,0x24,0xDD,0xAF,0xD1}};
    constexpr GUID unknown{0,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    void* output=nullptr;
    EXPECT_EQ(manager.QueryInterface(iid,&output),0);
    EXPECT_EQ(output,static_cast<ISwizzle*>(&manager));
    EXPECT_EQ(manager.QueryInterface(unknown,&output),0);
    EXPECT_EQ(manager.QueryInterface(GUID{1,2,3,{}},&output),HRESULT(0x80004002u));
    EXPECT_EQ(output,static_cast<ISwizzle*>(&manager)); // failure does not clear
    EXPECT_EQ(manager.QueryInterface(iid,nullptr),HRESULT(0x80004003u));
    LONG id=17;
    EXPECT_EQ(manager.Fetch_Swizzle_ID(pointer(0xFEDCBA98),&id),0);
    EXPECT_EQ(std::uint32_t(id),0xFEDCBA98u);
    EXPECT_EQ(manager.Fetch_Swizzle_ID(nullptr,&id),HRESULT(0x80004003u));
    EXPECT_EQ(manager.Fetch_Swizzle_ID(pointer(5),nullptr),HRESULT(0x80004003u));
    if constexpr (sizeof(void*)>4) {
        auto* wide=reinterpret_cast<void*>(std::uintptr_t(0x100000000ULL));
        EXPECT_EQ(manager.Fetch_Swizzle_ID(wide,&id),HRESULT(0x80070057u));
        EXPECT_EQ(manager.Swizzle(&wide),HRESULT(0x80070057u));
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(wide),0x100000000ULL);
    }
    int size=123;
    EXPECT_EQ(manager.Get_Save_Size(&size),0); EXPECT_EQ(size,4);
    EXPECT_EQ(manager.Get_Save_Size(nullptr),HRESULT(0x80004003u));
    EXPECT_EQ(manager.Save_Interface(nullptr,nullptr),HRESULT(0x80004001u));
    output=pointer(17);
    EXPECT_EQ(manager.Load_Interface(nullptr,nullptr,&output),HRESULT(0x80004001u));
    EXPECT_EQ(output,pointer(17));
    EXPECT_EQ(manager.Swizzle(nullptr),HRESULT(0x80004003u));
}
TEST(Swizzle, ForwardRepeatedAndUnsignedReferences) {
    std::array<void*,5> slots{pointer(5),pointer(0xFFFFFFFF),pointer(5),nullptr,pointer(2)};
    SwizzleManagerClass manager;
    int first=1,second=2,third=3;
    for(auto& slot:slots) { EXPECT_EQ(manager.Swizzle(&slot),0); EXPECT_EQ(slot,nullptr); }
    EXPECT_EQ(manager.Swizzles_Old.Count,4);
    EXPECT_EQ(manager.Here_I_Am(5,&first),0);
    EXPECT_EQ(manager.Here_I_Am(identity(0xFFFFFFFF),&second),0);
    EXPECT_EQ(manager.Here_I_Am(2,&third),0);
    EXPECT_EQ(manager.Reset(),0);
    EXPECT_EQ(slots[0],&first); EXPECT_EQ(slots[2],&first);
    EXPECT_EQ(slots[1],&second); EXPECT_EQ(slots[3],nullptr); EXPECT_EQ(slots[4],&third);
    EXPECT_EQ(manager.Swizzles_Old.Count,0); EXPECT_EQ(manager.Swizzles_New.Count,0);
    EXPECT_EQ(manager.Swizzles_Old.Capacity,0); EXPECT_EQ(manager.Swizzles_New.Capacity,0);
}
TEST(Swizzle, ExceptionsStayOnThisSideOfCom) {
    struct ThrowingManager : SwizzleManagerClass {
        ULONG YRPP_STDCALL AddRef() override { throw 1; }
        HRESULT YRPP_STDCALL Fetch_Swizzle_ID(void*,LONG*) const override { throw 2; }
    } manager;
    constexpr GUID unknown{0,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    void* output=nullptr;
    EXPECT_EQ(manager.QueryInterface(unknown,&output),HRESULT(0x80004005u));
    EXPECT_EQ(output,static_cast<ISwizzle*>(&manager));
    void* slot=pointer(5);
    EXPECT_EQ(manager.Swizzle(&slot),HRESULT(0x80004005u));
    EXPECT_EQ(slot,pointer(5));
    EXPECT_EQ(manager.Swizzles_Old.Count,0);
}
TEST(Swizzle, NoRequestsRetainsAnnouncementsAndDestructorResolves) {
    void* slot=pointer(41); int destination=9;
    {
        SwizzleManagerClass manager;
        EXPECT_EQ(manager.Here_I_Am(41,&destination),0);
        EXPECT_EQ(manager.Reset(),0);
        EXPECT_EQ(manager.Swizzles_New.Count,1);
        EXPECT_EQ(manager.Swizzle(&slot),0);
    }
    EXPECT_EQ(slot,&destination);
}
TEST(Swizzle, OrphanSafetyAndRetry) {
    void* a=pointer(1); void* b=pointer(2); int value=7;
    SwizzleManagerClass manager;
    manager.Swizzle(&a); manager.Swizzle(&b); manager.Here_I_Am(1,&value);
    EXPECT_EQ(manager.Reset(),HRESULT(0x80004005u));
    EXPECT_EQ(a,nullptr); EXPECT_EQ(b,nullptr);
    EXPECT_EQ(manager.Swizzles_Old.Count,2);
    manager.Here_I_Am(2,&value);
    EXPECT_EQ(manager.Reset(),0); EXPECT_EQ(a,&value); EXPECT_EQ(b,&value);
}
TEST(Swizzle, RefusedGrowthKeepsOriginalSuccessAndClearsSlot) {
    void* slot=pointer(1);
    SwizzleManagerClass manager;
    manager.Swizzles_Old.CapacityIncrement=0;
    manager.Swizzles_New.CapacityIncrement=0;
    EXPECT_EQ(manager.Swizzle(&slot),0); EXPECT_EQ(slot,nullptr);
    EXPECT_EQ(manager.Swizzles_Old.Count,0);
    EXPECT_EQ(manager.Here_I_Am(1,pointer(9)),0); EXPECT_EQ(manager.Swizzles_New.Count,0);
}
TEST(Swizzle, OriginalInstructionCorpus) {
    std::ifstream input(RA2_SWIZZLE_FIXTURE);
    ASSERT_TRUE(input.good());
    int cases=0;
    char kind;
    while(input>>kind) {
        ASSERT_EQ(kind,'S');
        int requests,announcements,old_count,new_count,old_cap,new_cap;
        std::uint32_t status;
        ASSERT_TRUE(bool(input>>requests>>announcements>>status>>old_count>>new_count>>old_cap>>new_cap));
        std::vector<void*> slots(requests);
        std::array<SwizzlePointerClass,64> old_storage{},new_storage{};
        SwizzleManagerClass manager;
        manager.Swizzles_Old.SetCapacity(64,old_storage.data());
        manager.Swizzles_New.SetCapacity(64,new_storage.data());
        for(auto& slot:slots) {
            std::uint32_t id; ASSERT_TRUE(bool(input>>id));
            slot=pointer(id); ASSERT_EQ(manager.Swizzle(&slot),0);
        }
        for(int i=0;i<announcements;++i) {
            std::uint32_t id,value; ASSERT_TRUE(bool(input>>id>>value));
            ASSERT_EQ(manager.Here_I_Am(identity(id),pointer(value)),0);
        }
        EXPECT_EQ(std::uint32_t(manager.Reset()),status);
        EXPECT_EQ(manager.Swizzles_Old.Count,old_count); EXPECT_EQ(manager.Swizzles_New.Count,new_count);
        EXPECT_EQ(manager.Swizzles_Old.Capacity,old_cap); EXPECT_EQ(manager.Swizzles_New.Capacity,new_cap);
        EXPECT_EQ(manager.Swizzles_Old.Items,old_storage.data());
        EXPECT_EQ(manager.Swizzles_New.Items,new_storage.data());
        for(auto slot:slots) {
            std::uint32_t value; ASSERT_TRUE(bool(input>>value));
            EXPECT_EQ(slot,pointer(value)) << "case " << cases;
        }
        ++cases;
    }
    EXPECT_EQ(cases,31);
}
