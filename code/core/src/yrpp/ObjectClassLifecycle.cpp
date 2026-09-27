// Native map lifecycle using the original object fields.
// Original implementation: 0x005F3900 / 0x005F3B00.
#include "yrpp/ObjectClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/LineTrail.h"
#include "map_world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <bit>
#include <cstdlib>
#include <cstring>
namespace { DynamicVectorClass<ObjectClass*> selected; }
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=selected;
ObjectClass::ObjectClass() noexcept
 : AbstractClass(),
 unknown_24{},
 unknown_28{},
 FallRate{},
 NextObject{},
 AttachedTag{},
 AttachedBomb{},
 AmbientSoundController{},
 CustomSoundController{},
 CustomSound{},
 BombVisible{},
 align_69{},
 Health{},
 EstimatedHealth{},
 IsOnMap{},
 align_75{},
 unknown_78{},
 unknown_7C{},
 NeedsRedraw{},
 InLimbo{},
 InOpenToppedTransport{},
 IsSelected{},
 HasParachute{},
 align_85{},
 Parachute{},
 OnBridge{},
 IsFallingDown{},
 WasFallingDown{},
 IsABomb{},
 IsAlive{},
 align_91{},
 LastLayer{},
 IsInLogic{},
 IsVisible{},
 align_99{},
 Location{},
 LineTrailer{} {
 AbstractFlags |= ::AbstractFlags::Object;
 Health=EstimatedHealth=1; IsAlive=true; InLimbo=true; NeedsRedraw=true;
 LastLayer=Layer::Ground; CustomSound=-1; Create_ID();
 if(!TagExpirationListeners.AddItem(this))std::abort();
 if(!AbstractClass::Array.AddItem(this)||!TypeExpirationListeners.AddItem(this))std::abort();
}
ObjectClass::~ObjectClass(){ LogicClass::Instance.RemoveObject(this);CurrentObjects.Remove(this); game::detach_map_object(*this,true);
 if(LineTrailer)LineTrailer->Detach();
 TagExpirationListeners.Remove(this);
 AbstractClass::Array.Remove(this);TypeExpirationListeners.Remove(this);
 AmbientSoundController.~AudioController();
 CustomSoundController.~AudioController();
}
CoordStruct* ObjectClass::GetCoords(CoordStruct* out) const {if(out)*out=Location;return out;}
DirStruct* ObjectClass::GetDirectionTo(DirStruct* out,AbstractClass* target) const {
 const auto destination=target->GetCoords();const auto origin=GetCoords();
 const double angle=Math::atan2(double(origin.Y)-destination.Y,double(destination.X)-origin.X);
 const auto direction=static_cast<unsigned short>(int((angle-1.5707963267948966)*-10430.060040584269));
 // 0x5F3DB0 copies all four bytes: the upper half retains destination.X.
 const std::uint32_t packed=(static_cast<std::uint32_t>(destination.X)&0xFFFF0000u)|direction;
 std::memcpy(out,&packed,sizeof(packed));return out;
}
CoordStruct* ObjectClass::GetCenterCoords(CoordStruct* out) const{return GetCoords(out);}
CoordStruct* ObjectClass::GetRenderCoords(CoordStruct* out) const{return GetCenterCoords(out);}
int ObjectClass::GetYSort() const {
 const auto position=GetRenderCoords();
 // 0x005F6BD0 adds world X/Y with the original 32-bit wrap, before projection.
 return std::bit_cast<std::int32_t>(std::uint32_t(position.X)+std::uint32_t(position.Y));
}
bool ObjectClass::IsDead() const{return !IsAlive||Health<=0;}
bool ObjectClass::IsActive() const{return IsAlive&&Health>0&&IsOnMap&&!InLimbo;}
SHPStruct* ObjectClass::GetImage() const{auto*t=GetType();return t?t->GetImage():nullptr;}
const wchar_t* ObjectClass::GetUIName() const{auto*t=GetType();return t&&t->UIName?t->UIName:L"";}
bool ObjectClass::IsSelectable() const{auto*t=GetType();return t&&t->Selectable;}
bool ObjectClass::CanBeSelected() const{return IsSelectable()&&IsAlive&&Health>0&&IsOnMap&&!InLimbo;}
bool ObjectClass::CanBeSelectedNow() const{return CanBeSelected();}
bool ObjectClass::Select(){if(!CanBeSelected())return false;if(!IsSelected&&!CurrentObjects.AddItem(this))return false;IsSelected=true;NeedsRedraw=true;game::map_object_changed();return true;}
void ObjectClass::Deselect(){if(IsSelected){IsSelected=false;CurrentObjects.Remove(this);NeedsRedraw=true;game::map_object_changed();}}
CellStruct* ObjectClass::GetMapCoords(CellStruct*out) const{if(out)*out={short(Location.X/256),short(Location.Y/256)};return out;}
CellStruct* ObjectClass::GetMapCoordsAgain(CellStruct*out) const{const auto at=GetDestination();*out={short(at.X/256),short(at.Y/256)};return out;}
CellClass* ObjectClass::GetCell() const{MapClass::Instance.GetCellAt(Location);return MapClass::Instance.GetCellAt(Location);}
CellClass* ObjectClass::GetCellAgain() const{const auto at=GetDestination();return MapClass::Instance.GetCellAt(CellStruct{short(at.X/256),short(at.Y/256)});}
int ObjectClass::GetHeight() const{return Location.Z-MapClass::Instance.GetCellFloorHeight(Location)-(OnBridge?CellClass::BridgeHeight:0);}
int ObjectClass::GetCellLevel() const{return static_cast<signed char>(GetCell()->Level)+(OnBridge?4:0);}
void ObjectClass::SetHeight(DWORD height){
 const bool down=IsOnMap;if(down)Mark(MarkType::Up);
 Location.Z=std::bit_cast<int>(height)+MapClass::Instance.GetCellFloorHeight(Location)+(OnBridge?CellClass::BridgeHeight:0);
 if(down)Mark(MarkType::Down);
}
void ObjectClass::SetLocation(const CoordStruct&v){Location=v;}
/// VA: 0x005F5C80
void ObjectClass::SetHealthPercentage(double p){auto*t=GetType();if(!t||!std::isfinite(p))return;double hp=std::trunc(double(t->Strength)*p);Health=p<=0?0:int(std::clamp(hp,1.0,double(INT32_MAX)));EstimatedHealth=Health;NeedsRedraw=true;game::map_object_changed();}
bool ObjectClass::IsRedHP() const{auto*t=GetType();auto*r=RulesClass::Instance;return t&&t->Strength>0&&Health>0&&double(Health)/t->Strength<=(r?r->ConditionRed:0.25);}
bool ObjectClass::IsYellowHP() const{auto*t=GetType();auto*r=RulesClass::Instance;double q=t&&t->Strength>0?double(Health)/t->Strength:0;return Health>0&&q>(r?r->ConditionRed:0.25)&&q<=(r?r->ConditionYellow:0.5);}
bool ObjectClass::IsGreenHP() const{auto*t=GetType();auto*r=RulesClass::Instance;return t&&t->Strength>0&&Health>0&&double(Health)/t->Strength>(r?r->ConditionYellow:0.5);}
HealthState ObjectClass::GetHealthStatus() const{return IsRedHP()?HealthState::Red:IsYellowHP()?HealthState::Yellow:HealthState::Green;}
