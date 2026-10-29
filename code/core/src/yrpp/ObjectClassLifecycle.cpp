#include "tactical_spatial.hpp"
#include "techno_spatial.hpp"
// Native map lifecycle using the original object fields.
// Original implementation: 0x005F3900 / 0x005F3B00.
#include "background_invalidation.hpp"
#include "yrpp/ObjectClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/LineTrail.h"
#include "map_world.hpp"
#include "debug/debug_state.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <bit>
#include <cstdlib>
#include <cstring>
#include "yrpp/SwizzleManagerClass.h"
#include "yrpp/Unsorted.h"
#include <new>
namespace {
DynamicVectorClass<ObjectClass*> selected, objects;
}
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects = selected;
DynamicVectorClass<ObjectClass*>& ObjectClass::Array = objects;
ObjectClass::ObjectClass() noexcept
    : AbstractClass(), unknown_24{}, unknown_28{}, FallRate{}, NextObject{}, AttachedTag{}, AttachedBomb{},
      AmbientSoundController{}, CustomSoundController{}, CustomSound{}, BombVisible{}, align_69{}, Health{},
      EstimatedHealth{}, IsOnMap{}, align_75{}, unknown_78{}, unknown_7C{}, NeedsRedraw{}, InLimbo{},
      InOpenToppedTransport{}, IsSelected{}, HasParachute{}, align_85{}, Parachute{}, OnBridge{}, IsFallingDown{},
      WasFallingDown{}, IsABomb{}, IsAlive{}, align_91{}, LastLayer{}, IsInLogic{}, IsVisible{}, align_99{}, Location{},
      LineTrailer{} {
    AbstractFlags |= ::AbstractFlags::Object;
    Health = EstimatedHealth = 255;
    IsAlive = true;
    InLimbo = true;
    NeedsRedraw = false;
    LastLayer = Layer::None;
    CustomSound = -1;
    // Concrete constructors own Create_ID; 0x5F3900 leaves the base ID unset.
    if (!ObjectClass::Array.AddItem(this)) std::abort();
    if (!TagExpirationListeners.AddItem(this)) std::abort();
    if (!AbstractClass::Array.AddItem(this) || !TypeExpirationListeners.AddItem(this)) std::abort();
}
ObjectClass::~ObjectClass() {
    game::tactical_spatial_forget(this);
    game::forget_debug_object(this);
    LogicClass::Instance.RemoveObject(this);
    CurrentObjects.Remove(this);
    game::map_object_changed();
    if (LineTrailer) LineTrailer->Detach();
    PendingDeletes.Remove(this);
    ObjectClass::Array.Remove(this);
    TagExpirationListeners.Remove(this);
    AbstractClass::Array.Remove(this);
    TypeExpirationListeners.Remove(this);
    AmbientSoundController.~AudioController();
    CustomSoundController.~AudioController();
}
CoordStruct* ObjectClass::GetCoords(CoordStruct* out) const {
    if (out) *out = Location;
    return out;
}
DirStruct* ObjectClass::GetDirectionTo(DirStruct* out, AbstractClass* target) const {
    const auto destination = target->GetCoords();
    const auto origin = GetCoords();
    const double angle = Math::atan2(double(origin.Y) - destination.Y, double(destination.X) - origin.X);
    const auto direction = static_cast<unsigned short>(int((angle - 1.5707963267948966) * -10430.060040584269));
    // 0x5F3DB0 copies all four bytes: the upper half retains destination.X.
    const std::uint32_t packed = (static_cast<std::uint32_t>(destination.X) & 0xFFFF0000u) | direction;
    std::memcpy(out, &packed, sizeof(packed));
    return out;
}
CoordStruct* ObjectClass::GetCenterCoords(CoordStruct* out) const { return GetCoords(out); }
CoordStruct* ObjectClass::GetRenderCoords(CoordStruct* out) const { return GetCenterCoords(out); }
int ObjectClass::GetYSort() const {
    const auto position = GetRenderCoords();
    // 0x005F6BD0 adds world X/Y with the original 32-bit wrap, before projection.
    return std::bit_cast<std::int32_t>(std::uint32_t(position.X) + std::uint32_t(position.Y));
}
bool ObjectClass::IsDead() const { return !IsAlive; }
bool ObjectClass::IsActive() const { return false; }
SHPStruct* ObjectClass::GetImage() const {
    auto* t = GetType();
    return t ? t->GetImage() : nullptr;
}
const wchar_t* ObjectClass::GetUIName() const { return L"No name"; }
bool ObjectClass::IsSelectable() const { return false; }

bool ObjectClass::IsOnBridge(TechnoClass* docker) const {
    const bool bridge = OnBridge;
    const auto destination = GetDestination(docker);
    auto& map = MapClass::Instance;
    const int targetFloor = map.GetCellFloorHeight(destination);
    const int ownFloor = map.GetCellFloorHeight(Location);
    if (!OnBridge && std::bit_cast<int>(unsigned(ownFloor) - unsigned(targetFloor)) > 3 * Unsorted::LevelHeight &&
        (unsigned(map.GetCellAt(destination)->Flags) & 0x100u)) return true;
    if (OnBridge && std::bit_cast<int>(unsigned(targetFloor) - unsigned(ownFloor)) > 3 * Unsorted::LevelHeight) return false;
    return bridge;
}

void ObjectClass::MoveToDirection(FacingType facing) {
    Mark(MarkType::Up);
    const auto delta = Unsorted::AdjacentCoord[unsigned(facing) & 7u];
    const CoordStruct at{std::bit_cast<int>(unsigned(Location.X) + unsigned(delta.X)),
                          std::bit_cast<int>(unsigned(Location.Y) + unsigned(delta.Y)), Location.Z};
    if (IsCellOccupied(MapClass::Instance.GetCellAt(at), facing, -1, nullptr, true) == Move::OK) SetLocation(at);
    Mark(MarkType::Down);
}

CellStruct* ObjectClass::GetMapCoords(CellStruct* out) const {
    if (out) *out = {short(Location.X / 256), short(Location.Y / 256)};
    return out;
}
CellStruct* ObjectClass::GetMapCoordsAgain(CellStruct* out) const {
    const auto at = GetDestination();
    *out = {short(at.X / 256), short(at.Y / 256)};
    return out;
}
CellClass* ObjectClass::GetCell() const {
    MapClass::Instance.GetCellAt(Location);
    return MapClass::Instance.GetCellAt(Location);
}
CellClass* ObjectClass::GetCellAgain() const {
    const auto at = GetDestination();
    return MapClass::Instance.GetCellAt(CellStruct{short(at.X / 256), short(at.Y / 256)});
}
int ObjectClass::GetHeight() const {
    return Location.Z - MapClass::Instance.GetCellFloorHeight(Location) - (OnBridge ? CellClass::BridgeHeight : 0);
}
int ObjectClass::GetCellLevel() const { return static_cast<signed char>(GetCell()->Level) + (OnBridge ? 4 : 0); }
void ObjectClass::SetHeight(DWORD height) {
    game::techno_spatial_dirty(this);
    game::tactical_spatial_dirty(this);
    if (WhatAmI() == AbstractType::Terrain) game::invalidate_background_object(*this);
    const bool down = IsOnMap;
    if (down) Mark(MarkType::Up);
    Location.Z = std::bit_cast<int>(height) + MapClass::Instance.GetCellFloorHeight(Location) +
                 (OnBridge ? CellClass::BridgeHeight : 0);
    game::techno_spatial_dirty(this);
    if (down) Mark(MarkType::Down);
}
void ObjectClass::SetLocation(const CoordStruct& v) {
    Location = v;
    game::techno_spatial_dirty(this);
    game::tactical_spatial_dirty(this);
    if (WhatAmI() == AbstractType::Terrain) game::invalidate_background_object(*this);
}
/// VA: 0x005F5C80
void ObjectClass::SetHealthPercentage(double percentage) {
    if (WhatAmI() == AbstractType::Terrain) game::invalidate_background_object(*this);
    if (!(percentage > 0.0)) {
        Health = 0;
        game::techno_spatial_dirty(this);
        return;
    }
    auto* type = GetType();
    if (!type) return;
    // The original conversion keeps the low 32 bits of its signed 64-bit result, and the
    // product before it is formed under the truncating control word the helper leaves behind.
    // It only updates Health; redraw and estimated health belong to the caller.
    Health = game::x87_integer(game::x87_truncated_product(double(type->Strength), percentage));
    if (!Health) Health = 1;
    game::techno_spatial_dirty(this);
}
bool ObjectClass::IsRedHP() const {
    auto* t = GetType();
    auto* r = RulesClass::Instance;
    return t && t->Strength > 0 && Health > 0 && double(Health) / t->Strength <= (r ? r->ConditionRed : 0.25);
}
bool ObjectClass::IsYellowHP() const {
    auto* t = GetType();
    auto* r = RulesClass::Instance;
    double q = t && t->Strength > 0 ? double(Health) / t->Strength : 0;
    return Health > 0 && q > (r ? r->ConditionRed : 0.25) && q <= (r ? r->ConditionYellow : 0.5);
}
bool ObjectClass::IsGreenHP() const {
    auto* t = GetType();
    auto* r = RulesClass::Instance;
    return t && t->Strength > 0 && Health > 0 && double(Health) / t->Strength > (r ? r->ConditionYellow : 0.5);
}
HealthState ObjectClass::GetHealthStatus() const {
    return IsRedHP() ? HealthState::Red : IsYellowHP() ? HealthState::Yellow : HealthState::Green;
}

BuildingClass* ObjectClass::FindFactory(bool allowOccupied, bool requirePower) const {
    return GetType()->FindFactory(allowOccupied, requirePower, false, GetOwningHouse());
}

#if !defined(RA2_YRPP_GAME)
// The parent reads the full concrete record. Only Object-owned runtime tails
// are repaired here; the concrete loader restores its own vtables afterwards.
HRESULT YRPP_STDCALL ObjectClass::Load(IStream* stream) {
    const auto result = AbstractClass::Load(stream);
    if (result < 0) return result;
    auto& swizzle = SwizzleManagerClass::Instance;
    swizzle.Swizzle(reinterpret_cast<void**>(&NextObject));
    swizzle.Swizzle(reinterpret_cast<void**>(&AttachedTag));
    swizzle.Swizzle(reinterpret_cast<void**>(&AttachedBomb));
    swizzle.Swizzle(reinterpret_cast<void**>(&unknown_18));
    swizzle.Swizzle(reinterpret_cast<void**>(&Parachute));
    ::new (&AmbientSoundController) AudioController();
    ::new (&CustomSoundController) AudioController();
    LineTrailer = nullptr;
    return 0;
}
#endif
