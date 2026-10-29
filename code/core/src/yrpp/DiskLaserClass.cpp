// Native callee failures are fatal; exceptions do not cross the weapon boundary.
// Original YR disk module, 0x4A71A0..0x4A7D10. The ring advances in Logic.
#include "yrpp/DiskLaserClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <cstdlib>
#include <bit>
#include <new>
#include <cstring>

#if !defined(RA2_YRPP_GAME)
namespace {
Point2D ring[16];
struct RingInit {
    RingInit() {
        // Preserve the original integer-degree table, including 235 and 257.
        constexpr int degrees[16] = {270, 292, 315, 337, 0, 22, 45, 67, 90, 112, 135, 157, 180, 202, 235, 257};
        for (int i = 0; i < 16; ++i) {
            const double angle = 0.017453292519943295 * degrees[i];
            ring[i] = {game::x87_integer(Math::cos(angle) * 240.0), game::x87_integer(Math::sin(angle) * 240.0)};
        }
    }
} ringInit;
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
void retire(DiskLaserClass* disk) {
    disk->unknown_30 = 0xFFFFFFFFu;
    try {
        AbstractClass::PendingDeletes.AddItem(disk);
    } catch (const std::bad_alloc&) {
        AbstractClass::PendingDeletes.IsInitialized = true;
    }
}
void laser(CoordStruct a, CoordStruct b, ColorStruct inner, ColorStruct outer, ColorStruct spread, int duration,
           float end) {
    if (auto* memory = YRMemory::Allocate(sizeof(LaserDrawClass)))
        ::new (memory) LaserDrawClass(a, b, 0, 1, inner, outer, spread, duration, false, true, 1.0f, end);
}
}
Point2D (&DiskLaserClass::DrawCoords)[16] = ring;
DiskLaserClass::DiskLaserClass() noexcept
    : AbstractClass(), Owner(nullptr), Target(nullptr), Weapon(nullptr), unknown_30(0), unknown_34(0), unknown_38(0),
      Damage(0) {
    try {
        Array.AddItem(this);
    } catch (const std::bad_alloc&) {
        Array.IsInitialized = true;
    }
}
DiskLaserClass::~DiskLaserClass() {
    Array.Remove(this);
    while (PendingDeletes.Remove(this)) {
    }
}
void DiskLaserClass::Fire(TechnoClass* owner, TechnoClass* target, WeaponTypeClass* weapon, int damage) try {
    if (!owner || !target || !weapon) {
        retire(this);
        return;
    }
    Owner = owner;
    Target = target;
    Weapon = weapon;
    Damage = damage;
    CoordStruct fromBuffer, toBuffer;
    const auto* to = target->GetCoords(&toBuffer);
    const auto* from = owner->GetCoords(&fromBuffer);
    const double angle = Math::atan2(double(from->Y) - to->Y, double(to->X) - from->X);
    const auto raw = static_cast<unsigned short>(game::x87_integer(
        game::x87_truncated_product(game::x87_truncated_difference(angle, 1.5707963267948966), -10430.060040584269)));
    unknown_34 = ((((unsigned(raw) >> 11) + 1) >> 1) & 15) + 8;
    unknown_34 %= 16;
    unknown_30 = unknown_38 = 0;
} catch (...) {
    std::abort();
}
void DiskLaserClass::PointerGotInvalid(AbstractClass* invalid) try {
    if (Owner == invalid) retire(this);
    if (Target == invalid) retire(this);
} catch (...) {
    std::abort();
}
void DiskLaserClass::Update() try {
    const int delay = std::bit_cast<int>(unknown_30);
    if (delay < 0) {
        retire(this);
        return;
    }
    if (delay > 0) {
        --unknown_30;
        return;
    }
    CoordStruct buffer;
    auto from = *Owner->GetCoords(&buffer);
    if (Owner->IsInAir()) from.Z = Target->GetCoords(&buffer)->Z;
    const auto targetCoords = *Target->GetCoords(&buffer);
    const double dx = std::bit_cast<int>(unsigned(from.X) - unsigned(targetCoords.X));
    const double dy = std::bit_cast<int>(unsigned(from.Y) - unsigned(targetCoords.Y));
    const double dz = std::bit_cast<int>(unsigned(from.Z) - unsigned(targetCoords.Z));
    int distance = game::x87_integer(Math::sqrt(dx * dx + dy * dy + dz * dz));
    if (Target->WhatAmI() == AbstractType::Building) {
        auto* type = static_cast<BuildingClass*>(Target)->Type;
        distance = std::max(0, distance - 64 * (type->GetFoundationHeight(false) + type->GetFoundationWidth()));
    }
    if (distance > Weapon->Range || Owner->IsCrashing) {
        retire(this);
        return;
    }
    CoordStruct center;
    center = *Owner->GetFLH(&center, 0, CoordStruct::Empty);
    const int step = static_cast<int>(unknown_38), start = static_cast<int>(unknown_34);
    const int first = (start + step) % 16, second = (start - step + 16) % 16;
    ColorStruct inner = Weapon->LaserInnerColor, outer = Weapon->LaserOuterColor;
    if (Weapon->IsHouseColor) {
        inner = Owner->Owner->LaserColor;
        outer = {BYTE(inner.R >> 1), BYTE(inner.G >> 1), BYTE(inner.B >> 1)};
    }
    const auto at = [&](int i) {
        return CoordStruct{add(center.X, DrawCoords[i].X), add(center.Y, DrawCoords[i].Y), center.Z};
    };
    if (first == second && step) {
        CoordStruct end;
        end = *Target->GetTargetCoords(&end);
        laser(at(first), end, inner, outer, Weapon->LaserOuterSpread, static_cast<signed char>(Weapon->LaserDuration),
              0.0f);
        MapClass::DamageArea(end, Damage, Owner, Weapon->Warhead, true, nullptr);
        if (Weapon->Report.Count > 0)
            VocClass::PlayAt(
                Weapon->Report[static_cast<unsigned short>(Owner->unknown_short_3C8) % Weapon->Report.Count], at(first),
                nullptr);
        unknown_30 = 0xFFFFFFFFu;
    } else {
        if (!step && RulesClass::Instance->DiskLaserChargeUp != -1)
            VocClass::PlayAt(RulesClass::Instance->DiskLaserChargeUp, at(first), nullptr);
        laser(at(first), at((first + 1) % 16), inner, outer, Weapon->LaserOuterSpread, 8 - step, 0.5f);
        laser(at(second), at((start - step + 15) % 16), inner, outer, Weapon->LaserOuterSpread, 8 - step, 0.5f);
        unknown_30 = 1;
        ++unknown_38;
    }
} catch (...) {
    std::abort();
}

HRESULT YRPP_STDCALL DiskLaserClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x5230c9a8u, 0x47ec846au, 0x957ea2bdu, 0x491d5e44u};
    std::memcpy(output, words, sizeof(words));
    return 0;
}
#endif
