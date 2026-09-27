#include "support/test_support.hpp"
#include "yrpp/platform/ABI.h"
#include "support/yrpp_abi32.hpp"
#include "yrpp/SidebarClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Surface.h"
#include <type_traits>

static_assert(sizeof(LinkClass)==0x0C);
static_assert(sizeof(GadgetClass)==0x24);
static_assert(sizeof(ControlClass)==0x2C);
static_assert(sizeof(ToggleClass)==0x34);
static_assert(sizeof(ShapeButtonClass)==0x60);
static_assert(offsetof(ShapeButtonClass,ShapeData)==0x58);
static_assert(sizeof(SelectClass)==0x38);
static_assert(sizeof(RadarClass)==0x150C);
static_assert(sizeof(RadarTrackingStruct)==0xC);
static_assert(offsetof(RadarTrackingStruct,Object)==0);
static_assert(sizeof(HashObject<RadarTrackingStruct,TechnoClass*>)==0x10);
static_assert(offsetof(RadarClass,unknown_123C)==0x123C);
static_assert(offsetof(RadarClass,unknown_timer_1500)==0x1500);
static_assert(sizeof(RadarEventClass)==0x40);
static_assert(offsetof(RadarEventClass,DurationTimer)==0x24);
static_assert(std::is_same_v<decltype(&DSurface::DrawGradientLine),bool (DSurface::*)(
    RectangleStruct*,Point2D*,Point2D*,ColorStruct*,ColorStruct*,float*,float*)>);
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
int destroyed=0;
class Probe final : public FileClass {
public:
    ~Probe() override { ++destroyed; }
    const char* GetFileName() const override { return "name"; }
    const char* SetFileName(const char* s) override { return s; }
    BOOL CreateFile() override { return 103; }
    BOOL DeleteFile() override { return 104; }
    bool Exists(bool v) override { return v; }
    bool HasHandle() override { return true; }
    bool Open(FileAccessMode v) override { return v==FileAccessMode::Write; }
    bool OpenEx(const char* s,FileAccessMode v) override { return s && v==FileAccessMode::Read; }
    int ReadBytes(void* b,int n) override { *static_cast<int*>(b)=109; return n; }
    int Seek(int n,FileSeekMode v) override { return n+static_cast<int>(v); }
    int GetFileSize() override { return 111; }
    int WriteBytes(void* b,int n) override { return *static_cast<int*>(b)+n; }
    void Close() override { ++closed; }
    DWORD GetFileTime() override { return 114; }
    bool SetFileTime(DWORD n) override { return n==115; }
    void CDCheck(DWORD n,bool retry,const char* s) override { EXPECT_TRUE((n==116 && retry && s)); ++errors; }
    int closed=0,errors=0;
};
template<class Fn> Fn slot(FileClass* p,unsigned n) {
    void** table=nullptr; std::memcpy(&table,p,sizeof(table));
    return reinterpret_cast<Fn>(table[n]);
}
__declspec(noinline) int ordinary_read(FileClass* p,int& value) { return p->ReadBytes(&value,9); }
}

TEST(MsvcAbi, VirtualSlotsAndLayout) {
    auto* p=new Probe;
    EXPECT_TRUE((!std::strcmp(slot<const char*(__thiscall*)(FileClass*)>(p,1)(p),"name")));
    EXPECT_TRUE((!std::strcmp(slot<const char*(__thiscall*)(FileClass*,const char*)>(p,2)(p,"new"),"new")));
    EXPECT_TRUE((slot<BOOL(__thiscall*)(FileClass*)>(p,3)(p)==103));
    EXPECT_TRUE((slot<BOOL(__thiscall*)(FileClass*)>(p,4)(p)==104));
    EXPECT_TRUE((slot<bool(__thiscall*)(FileClass*,bool)>(p,5)(p,true)));
    EXPECT_TRUE((slot<bool(__thiscall*)(FileClass*)>(p,6)(p)));
    EXPECT_TRUE((slot<bool(__thiscall*)(FileClass*,FileAccessMode)>(p,7)(p,FileAccessMode::Write)));
    EXPECT_TRUE((slot<bool(__thiscall*)(FileClass*,const char*,FileAccessMode)>(p,8)(p,"x",FileAccessMode::Read)));
    int value=0;
    EXPECT_TRUE((slot<int(__thiscall*)(FileClass*,void*,int)>(p,9)(p,&value,9)==9 && value==109));
    EXPECT_TRUE((slot<int(__thiscall*)(FileClass*,int,FileSeekMode)>(p,10)(p,10,FileSeekMode::End)==12));
    EXPECT_TRUE((slot<int(__thiscall*)(FileClass*)>(p,11)(p)==111));
    EXPECT_TRUE((slot<int(__thiscall*)(FileClass*,void*,int)>(p,12)(p,&value,12)==121));
    slot<void(__thiscall*)(FileClass*)>(p,13)(p); EXPECT_TRUE((p->closed==1));
    EXPECT_TRUE((slot<DWORD(__thiscall*)(FileClass*)>(p,14)(p)==114));
    EXPECT_TRUE((slot<bool(__thiscall*)(FileClass*,DWORD)>(p,15)(p,115)));
    slot<void(__thiscall*)(FileClass*,DWORD,bool,const char*)>(p,16)(p,116,true,"error"); EXPECT_TRUE((p->errors==1));
    value=0; EXPECT_TRUE((ordinary_read(p,value)==9 && value==109));
    slot<void*(__thiscall*)(FileClass*,unsigned)>(p,0)(p,1); EXPECT_TRUE((destroyed==1));
}
