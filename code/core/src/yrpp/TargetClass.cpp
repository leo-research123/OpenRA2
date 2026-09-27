// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.
// Copyright 2026 OpenTS contributors
// Contains material derived from Electronic Arts source code.
// Modified for RedAlert2Open: YR class names, target index and map access.
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9, code/target.cpp.
// EA GPLv3 Section 7 terms apply; see third_party/opents/LICENSE.md.
#include "yrpp/TargetClass.h"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/AbstractClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"

namespace {
AbstractClass* tracked(const TargetClass& target) {
    AbstractClass* result = nullptr;
    if (target.m_RTTI == static_cast<unsigned char>(AbstractType::Abstract))
        AbstractClass::TargetIndex.TryGet(target.m_ID, result);
    return result;
}
}

TargetClass::TargetClass(AbstractClass* item) noexcept : m_ID{0}, m_RTTI{0} {
    if (!item) return;
    if (item->WhatAmI() == AbstractType::Cell) {
        const auto cell = static_cast<CellClass*>(item)->MapCoords;
        m_RTTI = static_cast<unsigned char>(AbstractType::Cell);
        m_ID = cell.X + 1000 * cell.Y;
    } else {
        m_RTTI = static_cast<unsigned char>(AbstractType::Abstract);
        m_ID = item->Fetch_ID();
    }
}

TargetClass::TargetClass(const CellStruct& cell) {
    // Startup 0x6E69E0 initializes 0xB0E830 to (0,0).
    // 0x6E6B20 leaves the ID bytes untouched for this no-cell value.
    m_RTTI = 0;
    if (cell != CellStruct::Empty) {
        m_RTTI = static_cast<unsigned char>(AbstractType::Cell);
        m_ID = cell.X + 1000 * cell.Y;
    }
}

TargetClass::TargetClass(const CoordStruct& coord)
    : m_ID{coord.X / 256 + 1000 * (coord.Y / 256)},
      m_RTTI{static_cast<unsigned char>(AbstractType::Cell)} {}

AbstractClass* TargetClass::As_Abstract() {
    if (m_RTTI == static_cast<unsigned char>(AbstractType::Cell)) return As_Cell();
    return tracked(*this);
}
CellClass* TargetClass::As_Cell() {
    if (m_RTTI != static_cast<unsigned char>(AbstractType::Cell)) return nullptr;
    return MapClass::Instance.GetCellAt(
        CellStruct{static_cast<short>(m_ID % 1000), static_cast<short>(m_ID / 1000)});
}
TechnoClass* TargetClass::As_Techno() {
    auto* item = tracked(*this);
    if (!item) return nullptr;
    // 0x40DD70 uses the game's type discriminator, not AbstractFlags or C++ RTTI.
    switch (item->WhatAmI()) {
    case AbstractType::Aircraft: case AbstractType::Unit:
    case AbstractType::Building: case AbstractType::Infantry:
        return static_cast<TechnoClass*>(item);
    default: return nullptr;
    }
}
AbstractTypeClass* TargetClass::As_AbstractType() {
    return dynamic_cast<AbstractTypeClass*>(tracked(*this));
}
TagClass* TargetClass::As_Tag() {
    return dynamic_cast<TagClass*>(tracked(*this));
}
TagTypeClass* TargetClass::As_TagType() {
    return dynamic_cast<TagTypeClass*>(tracked(*this));
}
ObjectClass* TargetClass::As_Object() {
    return dynamic_cast<ObjectClass*>(tracked(*this));
}
FootClass* TargetClass::As_Foot() {
    return dynamic_cast<FootClass*>(tracked(*this));
}
TriggerClass* TargetClass::As_Trigger() {
    return dynamic_cast<TriggerClass*>(tracked(*this));
}
HouseClass* TargetClass::As_House() {
    return dynamic_cast<HouseClass*>(tracked(*this));
}
TechnoTypeClass* TargetClass::As_TechnoType() {
    return dynamic_cast<TechnoTypeClass*>(tracked(*this));
}
TriggerTypeClass* TargetClass::As_TriggerType() {
    return dynamic_cast<TriggerTypeClass*>(tracked(*this));
}
TeamTypeClass* TargetClass::As_TeamType() {
    return dynamic_cast<TeamTypeClass*>(tracked(*this));
}
TerrainClass* TargetClass::As_Terrain() {
    return dynamic_cast<TerrainClass*>(tracked(*this));
}
BulletClass* TargetClass::As_Bullet() {
    return dynamic_cast<BulletClass*>(tracked(*this));
}
AnimClass* TargetClass::As_Anim() {
    return dynamic_cast<AnimClass*>(tracked(*this));
}
TeamClass* TargetClass::As_Team() {
    return dynamic_cast<TeamClass*>(tracked(*this));
}
InfantryClass* TargetClass::As_Infantry() {
    return dynamic_cast<InfantryClass*>(tracked(*this));
}
UnitClass* TargetClass::As_Unit() {
    return dynamic_cast<UnitClass*>(tracked(*this));
}
BuildingClass* TargetClass::As_Building() {
    return dynamic_cast<BuildingClass*>(tracked(*this));
}
AircraftClass* TargetClass::As_Aircraft() {
    return dynamic_cast<AircraftClass*>(tracked(*this));
}
