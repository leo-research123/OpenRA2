// Full production RTactical caller, with declared original callee boundaries.
#include "yrpp/RadarClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/DrawingBuffers.h"
#include "game_ui_runtime.hpp"
#include <cstddef>
static_assert(offsetof(DisplayClass,CurrentSWTypeIndex)==0x11B8);
static_assert(offsetof(RadarClass,unknown_rect_149C)==0x149C);
RadarClass& RadarClass::Instance=*reinterpret_cast<RadarClass*>(0x87F7E8);
MouseClass& MouseClass::Instance=*reinterpret_cast<MouseClass*>(0x87F7E8);
GScreenClass& GScreenClass::Instance=*reinterpret_cast<GScreenClass*>(0x87F7E8);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x887324);
RectangleStruct& TacticalClass::ViewBounds=*reinterpret_cast<RectangleStruct*>(0x886FA0);
ZBuffer*& ZBuffer::Instance=*reinterpret_cast<ZBuffer**>(0x887644);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
bool& PlanningNodeClass::PlanningModeActive=*reinterpret_cast<bool*>(0xAC4CF4);
bool& Unsorted::DragSelectAborted=*reinterpret_cast<bool*>(0xA8ED9D);
GadgetClass*& GadgetClass::StuckOn=*reinterpret_cast<GadgetClass**>(0x01009000);
namespace game {
const GameUiInput* game_ui_input() noexcept {
    return reinterpret_cast<GameUiInput*>(0x01009100);
}
}
RectangleStruct RadarClass::GetPanelBounds() const noexcept {
    return {(*reinterpret_cast<bool*>(0xA8EB7C)?TacticalClass::ViewBounds.Width:0)+int(unknown_11F0),
        int(unknown_11F4),int(unknown_1200),int(unknown_1204)};
}
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
int MapClass::GetCellFloorHeight(const CoordStruct& point) const {
    return reinterpret_cast<int(__thiscall*)(const MapClass*,const CoordStruct*)>(0x578080)(this,&point);
}
bool MapClass::IsLocationShrouded(const CoordStruct& point) const {
    return reinterpret_cast<bool(__thiscall*)(const MapClass*,const CoordStruct*)>(0x586360)(this,&point);
}
ObjectClass* __fastcall Unsorted::BestSelectedObject(const CellStruct* cell,ObjectClass* target) noexcept {
    return reinterpret_cast<ObjectClass*(__fastcall*)(const CellStruct*,ObjectClass*)>(0x5353D0)(cell,target);
}
int Game::PlanningManager_UnsupportedType() noexcept { return reinterpret_cast<int(__cdecl*)()>(0x639DA0)(); }
bool DisplayClass::ConvertAction(const CellStruct& cell,bool fog,ObjectClass* target,::Action action,bool mini) {
    return reinterpret_cast<bool(__thiscall*)(DisplayClass*,const CellStruct*,bool,ObjectClass*,::Action,bool)>(0x4AAE90)(this,&cell,fog,target,action,mini);
}
void DisplayClass::LeftMouseButtonUp(const CoordStruct& world,const CellStruct& cell,ObjectClass* target,::Action action,DWORD mini) noexcept {
    reinterpret_cast<void(__thiscall*)(DisplayClass*,const CoordStruct*,const CellStruct*,ObjectClass*,::Action,DWORD)>(0x4AB9B0)(this,&world,&cell,target,action,mini);
}
bool MouseClass::SetCursor(MouseCursorType cursor,bool mini) {
    return reinterpret_cast<bool(__thiscall*)(MouseClass*,MouseCursorType,bool)>(0x5BDA80)(this,cursor,mini);
}
bool MouseClass::UpdateCursor(MouseCursorType cursor,bool mini) {
    return reinterpret_cast<bool(__thiscall*)(MouseClass*,MouseCursorType,bool)>(0x5BDC80)(this,cursor,mini);
}
bool GadgetClass::Action(GadgetFlag flags,DWORD* key,KeyModifier modifier) {
    return reinterpret_cast<bool(__thiscall*)(GadgetClass*,GadgetFlag,DWORD*,KeyModifier)>(0x4E1530)(this,flags,key,modifier);
}
extern "C" {
__declspec(dllexport) bool __fastcall RadarAction(RadarClass::RTacticalClass* self,void*,GadgetFlag flags,DWORD* key,KeyModifier modifier) {
    return self->RadarClass::RTacticalClass::Action(flags,key,modifier);
}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
