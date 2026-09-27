// Export production methods with the original Microsoft x86 ABI. Projection
// remains the same original dependency on both sides of the differential.
#include "yrpp/ObjectClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "map_runtime.hpp"
#include <cstddef>
static_assert(offsetof(ObjectClass,NeedsRedraw)==0x80);
static_assert(offsetof(ObjectClass,InLimbo)==0x81);
static_assert(offsetof(TechnoClass,IsTether)==0x418);
static_assert(offsetof(TechnoClass,UnloadTimer)==0x350);
static_assert(offsetof(BuildingClass,IsFogged)==0x6E7);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x00887324);
bool TacticalClass::CoordsToClient(const CoordStruct*,Point2D*) const { JMP_THIS(0x006D2140); }
namespace game {
const MapRuntimeServices& map_runtime() noexcept {
    static const MapRuntimeServices runtime=[] {
        MapRuntimeServices r;
        r.view_bounds=reinterpret_cast<const RectangleStruct*>(0x00B0CE28);
        r.drawing_bounds=reinterpret_cast<const RectangleStruct*>(0x00886FA0);
        r.debug_map=reinterpret_cast<const bool*>(0x00A8ED6B);
        r.has_window=[]() noexcept {return *reinterpret_cast<void**>(0x00B73550)!=nullptr;};
        return r;
    }();
    return runtime;
}
}
extern "C" __declspec(dllexport) bool YRPP_FASTCALL ObjectVisible(const ObjectClass* p,void*,RectangleStruct* b,bool force,DWORD extra) {return p->ObjectClass::DrawIfVisible(b,force,extra);}
extern "C" __declspec(dllexport) bool YRPP_FASTCALL BuildingVisible(const BuildingClass* p,void*,RectangleStruct* b,bool force,DWORD extra) {return p->BuildingClass::DrawIfVisible(b,force,extra);}
extern "C" __declspec(dllexport) bool YRPP_FASTCALL UnitVisible(const UnitClass* p,void*,RectangleStruct* b,bool force,DWORD extra) {return p->UnitClass::DrawIfVisible(b,force,extra);}
extern "C" __declspec(dllexport) RectangleStruct* YRPP_FASTCALL RectangleIntersection(RectangleStruct* out,const RectangleStruct* a,const RectangleStruct* b,int* dx,int* dy) {return Drawing::Intersect(out,*a,*b,dx,dy);}
