// YR Wave lifecycle: 0x75E950, 0x75F610, 0x75F8B0, 0x75F980, 0x763200.
#include "yrpp/WaveClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>

namespace {
// 0x75F230 partitions screen space at the odd multiples of pi/8.
int direction(const Point2D& from, const Point2D& to) {
    const int x = to.X - from.X, y = from.Y - to.Y;
    const double slope = Math::sin(Math::Pi / 8) / Math::cos(Math::Pi / 8);
    if (x) {
        const double ratio = double(y) / x;
        if (ratio < 1 / slope && ratio >= slope) return x > 0 ? 1 : 5;
        if (ratio < slope && ratio >= -slope) return x > 0 ? 2 : 6;
        if (ratio < -slope && ratio >= -1 / slope) return x > 0 ? 3 : 7;
    }
    return y > 0 ? 0 : 4;
}
}

WaveClass::WaveClass(const CoordStruct& from, const CoordStruct& to, TechnoClass* owner, WaveType mode,
                     AbstractClass* target) noexcept
    : ObjectClass(), Target(target), Type(mode), IsTraveling(true), ShouldEnd(false), WaveEC(100), WaveCount(0),
      MatrixScale1(0), MatrixScale2(0), PointData_Counter(0), PointData_Pointer(nullptr), PointData2_X(0),
      PointData2_Y(0), PointData2_Pointer(nullptr), LaserEC(0), Owner(owner), Facing(), Cells(5) {
    try {
        const auto dx = unsigned(from.X) - unsigned(to.X), dy = unsigned(from.Y) - unsigned(to.Y);
        if (!(Math::sqrt(double(std::bit_cast<int>(dx * dx + dy * dy))) >= 240.0)) {
            ObjectClass::UnInit();
            return;
        }
        Create_ID();
        Array.AddItem(this);
        if (!TablesInitialized) InitializeTables();
        if (Type == WaveType::Magnetron)
            Draw_Magnetic(from, to);
        else
            Draw_NonMagnetic(from, to);
        FacingIndex = direction(WaveStartMiddle, WaveEndSide2);
        Unlimbo(LimboCoords, static_cast<DirType>(0));
        if (Type == WaveType::BigLaser || Type == WaveType::Laser) {
            MatrixScale1 = 1;
            LaserEC = 160;
            PointData_Pointer[0] = WaveEndSide1;
            PointData_Pointer[1] = WaveEndMiddle;
            PointData_Pointer[2] = WaveEndSide2;
        }
        Update_Wave();
    } catch (...) {
        std::abort();
    }
}
WaveClass::~WaveClass() {
    NotifyObjectExpired(true);
    Array.Remove(this);
    Cells.Clear();
}
HRESULT YRPP_STDCALL WaveClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    const unsigned words[]{237448653u, 298949647u, 2684357047u, 3517963556u};
    std::memcpy(id, words, sizeof(*id));
    return 0;
}
// Native save-game relocation is not available for this original class yet.
HRESULT YRPP_STDCALL WaveClass::Load(IStream*) { return static_cast<HRESULT>(0x80004001u); }
HRESULT YRPP_STDCALL WaveClass::Save(IStream*, BOOL) { return static_cast<HRESULT>(0x80004001u); }
AbstractType WaveClass::WhatAmI() const { return AbsID; }
int WaveClass::Size() const { return sizeof(*this); }
Layer WaveClass::InWhichLayer() const { return Layer::Air; }
ObjectTypeClass* WaveClass::GetType() const { return nullptr; }
void WaveClass::PointerExpired(AbstractClass* object, bool removed) {
    ObjectClass::PointerExpired(object, removed);
    if (Owner == object) Owner = nullptr;
    if (Target == object) Target = nullptr;
}
bool WaveClass::Unlimbo(const CoordStruct& at, DirType) {
    if (!Game::IsActive || !InLimbo || IsOnMap) return false;
    if (!Unsorted::ScenarioInit &&
        IsCellOccupied(MapClass::Instance.GetCellAt(at), FacingType::None, at.Z, nullptr, true) != Move::OK)
        return false;
    InLimbo = false;
    NeedsRedraw = false;
    SetLocation(at);
    if (!Mark(MarkType::Down)) return false;
    if (IsAlive) {
        if (InWhichLayer() != Layer::None) DisplayClass::Submit(this);
        LogicClass::Instance.AddObject(this, false);
    }
    return true;
}
bool WaveClass::Limbo() {
    if (!Game::IsActive || InLimbo) return false;
    Disappear(true);
    Mark(MarkType::Up);
    DisplayClass::Remove(this);
    LogicClass::Instance.RemoveObject(this);
    Undiscover();
    InLimbo = true;
    NeedsRedraw = false;
    return true;
}
