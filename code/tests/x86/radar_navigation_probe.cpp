// Production Navigate body, compared with the navigation branch of the full
// original RTactical::Action. Picking, cell height and final camera positioning
// are shared boundaries; the ordered cell clamp and redraw/depth writes execute.
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/DrawingBuffers.h"
#include <cstddef>
static_assert(offsetof(MapClass,MapRect)==0xEC);
static_assert(offsetof(MapClass,Redraws)==0x1158);
static_assert(offsetof(GScreenClass,Bitfield)==0x0C);
static_assert(offsetof(ZBuffer,MaxValue)==0x24);
static_assert(offsetof(TacticalClass,Redrawing)==0xD7D);
GScreenClass& GScreenClass::Instance=*reinterpret_cast<GScreenClass*>(0x87F7E8);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x887324);
RectangleStruct& TacticalClass::ViewBounds=*reinterpret_cast<RectangleStruct*>(0x886FA0);
ZBuffer*& ZBuffer::Instance=*reinterpret_cast<ZBuffer**>(0x887644);
bool RadarClass::RadarToCell(const Point2D& point,CellStruct& cell,TechnoClass*& object) const noexcept {
    reinterpret_cast<void(__thiscall*)(const RadarClass*,const Point2D*,CellStruct*,TechnoClass**)>(0x656750)(this,&point,&cell,&object);
    return cell!=CellStruct{-1,-1};
}
CellClass* MapClass::GetCellAt(const CellStruct& cell) const {
    return reinterpret_cast<CellClass*(__thiscall*)(const MapClass*,const CellStruct*)>(0x5657A0)(this,&cell);
}
CoordStruct* CellClass::GetCellCoords(CoordStruct* out) const {
    return reinterpret_cast<CoordStruct*(__thiscall*)(const CellClass*,CoordStruct*)>(0x480A30)(this,out);
}
extern "C" {
__declspec(dllexport) bool __fastcall RadarNavigate(RadarClass* self,void*,const Point2D* point){return self->Navigate(*point);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
