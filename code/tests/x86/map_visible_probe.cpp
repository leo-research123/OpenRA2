// Full production Map::SetVisibleRect clamp and re-entry dispatch, x86.
// IsWithinUsableArea delegates to the independently calibrated original
// routine; virtual object calls and GScreen redraw are controlled boundaries.
#include "yrpp/MapClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include <cstddef>
static_assert(offsetof(TechnoClass,IsInPlayfield)==0x3D5);
static_assert(offsetof(TechnoClass,Owner)==0x21C);
static_assert(offsetof(ObjectClass,IsAlive)==0x90);
static_assert(offsetof(ObjectClass,InLimbo)==0x81);
static_assert(offsetof(MapClass,VisibleRect)==0xFC);
GScreenClass& GScreenClass::Instance=*reinterpret_cast<GScreenClass*>(0x87F7E8);
DynamicVectorClass<TechnoClass*>& TechnoClass::Array=*reinterpret_cast<DynamicVectorClass<TechnoClass*>*>(0xA8EC78);
bool MapClass::IsWithinUsableArea(const CellStruct& cell,bool height) const {
    return reinterpret_cast<bool(__thiscall*)(const MapClass*,const CellStruct*,bool)>(0x578460)(this,&cell,height);
}
bool HouseClass::IsControlledByCurrentPlayer() const {
    return reinterpret_cast<bool(__thiscall*)(const HouseClass*)>(0x50B6F0)(this);
}
extern "C" {
__declspec(dllexport) void __fastcall MapVisibleRect(MapClass* self,void*,const RectangleStruct* bounds){self->MapClass::SetVisibleRect(*bounds);}
void* __cdecl memset(void* p,int value,std::size_t count){auto* b=static_cast<volatile byte*>(p);while(count--)*b++=byte(value);return p;}
}
