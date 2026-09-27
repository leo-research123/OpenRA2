// Production DrawObjects against original x86 storage; only submission is stubbed.
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_runtime.hpp"
#include "tactical_drawing.hpp"
#include <cstddef>
static_assert(offsetof(ObjectClass,IsVisible)==0x99);
static_assert(offsetof(TacticalClass,IsoTransformMatrix)==0xDE4);
static_assert(offsetof(ScenarioClass,SpecialFlags)==0x0);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x887324);
RectangleStruct& TacticalClass::ViewBounds=*reinterpret_cast<RectangleStruct*>(0xB0CE28);
ScenarioClass*& ScenarioClass::Instance=*reinterpret_cast<ScenarioClass**>(0xA8B230);
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
LayerClass (&MapClass::ObjectsInLayers)[5]=*reinterpret_cast<LayerClass(*)[5]>(0x8A0360);
CoordStruct AbstractClass::GetCoords() const {CoordStruct out;GetCoords(&out);return out;}
namespace game {
const MapRuntimeServices& map_runtime() noexcept {
 static const MapRuntimeServices runtime=[] {
  MapRuntimeServices r;r.height_scale=reinterpret_cast<const double*>(0xB0CD48);
  r.view_bounds=reinterpret_cast<const RectangleStruct*>(0xB0CE28);
  r.drawing_bounds=reinterpret_cast<const RectangleStruct*>(0x886FA0);
  r.debug_map=reinterpret_cast<const bool*>(0xA8ED6B);
  r.has_window=[]() noexcept {return *reinterpret_cast<void**>(0xB73550)!=nullptr;};return r;
 }();return runtime;
}
bool map_view_bounds(RectangleStruct& out) noexcept {out=*map_runtime().view_bounds;return true;}
bool drawing_completed(DrawingStatus s) noexcept {return s==DrawingStatus::drawn||s==DrawingStatus::skipped;}
void record_tactical_drawing(DrawingStatus) noexcept {}
DrawingStatus dispatch_tactical_object(ObjectClass& object,TacticalObjectPass pass,Point2D* point,RectangleStruct* clip,bool forced) noexcept {
 if(pass==TacticalObjectPass::behind)object.DrawBehind(point,clip);
 else if(pass==TacticalObjectPass::extras)object.DrawExtras(point,clip);
 else if(pass==TacticalObjectPass::building_info)static_cast<BuildingClass&>(object).DrawInfoTipAndSpiedSelection(point,clip);
 else object.DrawIfVisible(clip,forced,pass==TacticalObjectPass::building_upper);
 return DrawingStatus::drawn;
}
}
extern "C" __declspec(dllexport) void YRPP_FASTCALL DrawObjects(TacticalClass* self,void*,bool forced){self->DrawObjects(forced);}
