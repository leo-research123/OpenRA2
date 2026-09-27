#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#include "type_drawing.hpp"
#include <cstddef>
static_assert(offsetof(HouseClass,ColorSchemeIndex)==0x16054);
static_assert(offsetof(ColorScheme,LightConvert)==0x30C);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
DynamicVectorClass<HouseClass*>& HouseClass::Array=*reinterpret_cast<DynamicVectorClass<HouseClass*>*>(0xA80228);
DynamicVectorClass<ColorScheme*>& ColorScheme::Array=*reinterpret_cast<DynamicVectorClass<ColorScheme*>*>(0xB054D0);
BeaconManagerClass& BeaconManagerClass::Instance=*reinterpret_cast<BeaconManagerClass*>(0x89C3B0);
SHPStruct*& BeaconManagerClass::RadarBeaconArt=*reinterpret_cast<SHPStruct**>(0x89C478);
RadarClass& RadarClass::Instance=*reinterpret_cast<RadarClass*>(0x87F7E8);
int& Unsorted::CurrentFrame=*reinterpret_cast<int*>(0xA8ED84);
bool BeaconClass::VisibleToPlayer() const noexcept {
    return reinterpret_cast<bool(__thiscall*)(const BeaconClass*)>(0x4308B0)(this);
}
Point2D* RadarClass::GetCrdOnRadar(Point2D* out,CoordStruct* coord,bool restricted) {
    return reinterpret_cast<Point2D*(__thiscall*)(RadarClass*,Point2D*,const CoordStruct*,bool)>(0x6557F0)(this,out,coord,restricted);
}
namespace game {
GameUiFrame* game_ui_frame() noexcept{return nullptr;}
void record_ui_drawing(DrawingStatus) noexcept{}
void record_type_drawing_result(DrawingStatus) noexcept{}
static DrawingStatus target(void*,Surface* in,DrawingTargetHandle*& out){out=reinterpret_cast<DrawingTargetHandle*>(in);return DrawingStatus::drawn;}
static DrawingStatus palette(void*,ConvertClass* in,const DrawingPaletteHandle*& out){out=reinterpret_cast<const DrawingPaletteHandle*>(in);return DrawingStatus::drawn;}
const TypeDrawingContext* active_type_drawing() noexcept {
    static TypeDrawingContext context;
    context.legacy_target=target;context.legacy_palette=palette;return &context;
}
DrawingStatus submit_type_shape(const TypeDrawingContext&,const ShapeDrawingRequest& r) noexcept {
    using Draw=void(__fastcall*)(void*,const void*,SHPStruct*,int,const Point2D*,const RectangleStruct*,unsigned,void*,int,int,int,int,void*,int,int,int);
    reinterpret_cast<Draw>(0x4AED70)(r.target,r.palette,r.image,r.frame,&r.position,&r.clip,r.flags,nullptr,r.depth_adjustment,r.gradient,r.intensity,r.tint,nullptr,0,0,0);
    return DrawingStatus::drawn;
}
}
extern "C" {
__declspec(dllexport) void __fastcall DrawBeacon(BeaconClass* beacon,void*,Surface* target,RectangleStruct bounds,bool clear){beacon->DrawRadar(target,bounds,clear);}
__declspec(dllexport) void __fastcall DrawManager(BeaconManagerClass* manager,void*,Surface* target,RectangleStruct bounds){manager->DrawRadar(target,bounds);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
int _fltused=0;
void* __cdecl memset(void* out,int value,std::size_t count){auto* p=static_cast<volatile unsigned char*>(out);while(count--)*p++=static_cast<unsigned char>(value);return out;}
}
