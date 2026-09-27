// Bounded primitive trace for the production RadarClass::DrawNames body.
#include "yrpp/RadarClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Surface.h"
#include "yrpp/StringTable.h"
#include "game_ui_runtime.hpp"
#include "scenario_runtime.hpp"
#include <cstddef>
#include <cwchar>
#include <cstdarg>

static_assert(offsetof(HouseClass,UIName)==0x1602A);
static_assert(offsetof(HouseClass,ColorSchemeIndex)==0x16054);
static_assert(offsetof(HouseClass,KilledUnitsOfHouses)==0x53E4);
static_assert(offsetof(HouseClass,KilledBuildingsOfHouses)==0x5438);
static_assert(offsetof(HouseTypeClass,Multiplay)==0x1A5);
static_assert(offsetof(ColorScheme,BaseColor)==0x308);
static_assert(offsetof(SidebarClass,IsSidebarActive)==0x53A5);

SidebarClass& SidebarClass::Instance=*reinterpret_cast<SidebarClass*>(0x87F7E8);
DynamicVectorClass<HouseClass*>& HouseClass::Array=*reinterpret_cast<DynamicVectorClass<HouseClass*>*>(0xA80228);
RectangleStruct& DSurface::SidebarBounds=*reinterpret_cast<RectangleStruct*>(0x0100C000);
RectangleStruct& DSurface::WindowBounds=*reinterpret_cast<RectangleStruct*>(0x0100C010);

extern "C" {
__declspec(dllexport) volatile unsigned RadarNamesArgs[8]{};
__declspec(dllexport) void __fastcall RadarNamesDraw(RadarClass* self,void*) {self->DrawNames();}
__declspec(dllexport) __declspec(noinline) void __cdecl RadarNamesText(const wchar_t* text,const Point2D* point,const ColorScheme* scheme,unsigned flags) {
    RadarNamesArgs[0]=reinterpret_cast<unsigned>(text);RadarNamesArgs[1]=reinterpret_cast<unsigned>(point);
    RadarNamesArgs[2]=reinterpret_cast<unsigned>(scheme);RadarNamesArgs[3]=flags;__debugbreak();
}
__declspec(dllexport) __declspec(noinline) void __cdecl RadarNamesShape(int frame,const Point2D* point) {
    RadarNamesArgs[0]=frame;RadarNamesArgs[1]=reinterpret_cast<unsigned>(point);__debugbreak();
}
__declspec(dllexport) __declspec(noinline) void __cdecl RadarNamesLine(const RectangleStruct* rect,WORD color) {
    RadarNamesArgs[0]=reinterpret_cast<unsigned>(rect);RadarNamesArgs[1]=color;__debugbreak();
}
__declspec(dllexport) __declspec(noinline) int __cdecl RadarNamesFormat(wchar_t* buffer,std::size_t count,const wchar_t* format,int value) {
    RadarNamesArgs[0]=reinterpret_cast<unsigned>(buffer);RadarNamesArgs[1]=count;
    RadarNamesArgs[2]=reinterpret_cast<unsigned>(format);RadarNamesArgs[3]=value;__debugbreak();return 0;
}
int __cdecl _strcmpi(const char* a,const char* b) {
    while(*a && *b && ((*a|32)==(*b|32))){++a;++b;}return (*a|32)-(*b|32);
}
wchar_t* __cdecl wcscpy(wchar_t* out,const wchar_t* source){auto* begin=out;do{*out++=*source;}while(*source++);return begin;}
int __cdecl RadarNamesVFormat(unsigned long long,wchar_t* out,std::size_t count,const wchar_t* format,void*,va_list args) {
    const int value=va_arg(args,int);
    return RadarNamesFormat(out,count,format,value);
}
auto RadarNamesPrintfImport=&RadarNamesVFormat;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void* __cdecl memset(void* p,int value,std::size_t count){auto* b=static_cast<volatile byte*>(p);for(std::size_t i=0;i<count;++i)b[i]=byte(value);return p;}
}
#pragma comment(linker,"/alternatename:__imp____stdio_common_vswprintf=_RadarNamesPrintfImport")

const wchar_t* YRPP_FASTCALL StringTable::LoadString(const char* label,char**,const char*,int) {
    return reinterpret_cast<const wchar_t*>(label[4]=='N' ? 0x01008000 : 0x01008100);
}
namespace game {
GameUiFrame* game_ui_frame() noexcept {return reinterpret_cast<GameUiFrame*>(1);}
void record_ui_drawing(DrawingStatus) noexcept {__debugbreak();}
void draw_ui_shape(UiImage,const Point2D& point,int frame) noexcept {RadarNamesShape(frame,&point);}
void draw_ui_fill(const RectangleStruct& rect,WORD color) noexcept {RadarNamesLine(&rect,color);}
void draw_ui_text(const wchar_t* text,const RectangleStruct&,const Point2D& point,const ColorScheme* scheme,TextPrintType flags) noexcept {
    RadarNamesText(text,&point,scheme,unsigned(flags));
}
bool ui_palette_color(unsigned,WORD& color) noexcept {color=0x1234;return true;}
const ScenarioRuntimeServices& scenario_runtime() {
    static const ScenarioRenderServices render{.color_schemes=reinterpret_cast<DynamicVectorClass<ColorScheme*>*>(0xB054D0)};
    static const ScenarioRuntimeServices runtime{.render=&render};return runtime;
}
}
