#pragma once

#include "yrpp/GeneralStructures.h"
#include <cstddef>

class AbstractTypeClass;
class TagClass;
class TagTypeClass;
class AbstractClass;
class TechnoClass;
class ObjectClass;
class FootClass;
class TriggerClass;
class HouseClass;
class TechnoTypeClass;
class TriggerTypeClass;
class TeamTypeClass;
class TerrainClass;
class BulletClass;
class AnimClass;
class TeamClass;
class InfantryClass;
class UnitClass;
class BuildingClass;
class AircraftClass;
class CellClass;

// Original wire target: signed ID followed by a one-byte kind, not a pointer.
#pragma pack(push, 1)
class TargetClass
{
public:
    explicit TargetClass() noexcept : m_ID{0}, m_RTTI{0} {}
    /// VA: 0x006E6AB0
    explicit TargetClass(AbstractClass* item) noexcept;
    /// VA: 0x006E6B20
    explicit TargetClass(const CellStruct& cell);
    /// VA: 0x006E6B70
    explicit TargetClass(const CoordStruct& coord);

    template<typename T> T* As() = delete;
    /// VA: 0x006E6BB0
    AbstractTypeClass* As_AbstractType();
    operator AbstractTypeClass*() { return As_AbstractType(); }
    /// VA: 0x006E6C80
    TagClass* As_Tag();
    operator TagClass*() { return As_Tag(); }
    /// VA: 0x006E6D50
    TagTypeClass* As_TagType();
    operator TagTypeClass*() { return As_TagType(); }
    /// VA: 0x006E6E20
    AbstractClass* As_Abstract();
    operator AbstractClass*() { return As_Abstract(); }
    /// VA: 0x006E6F20
    TechnoClass* As_Techno();
    operator TechnoClass*() { return As_Techno(); }
    /// VA: 0x006E6FF0
    ObjectClass* As_Object();
    operator ObjectClass*() { return As_Object(); }
    /// VA: 0x006E70C0
    FootClass* As_Foot();
    operator FootClass*() { return As_Foot(); }
    /// VA: 0x006E7190
    TriggerClass* As_Trigger();
    operator TriggerClass*() { return As_Trigger(); }
    /// VA: 0x006E7260
    HouseClass* As_House();
    operator HouseClass*() { return As_House(); }
    /// VA: 0x006E7330
    TechnoTypeClass* As_TechnoType();
    operator TechnoTypeClass*() { return As_TechnoType(); }
    /// VA: 0x006E7400
    TriggerTypeClass* As_TriggerType();
    operator TriggerTypeClass*() { return As_TriggerType(); }
    /// VA: 0x006E74D0
    TeamTypeClass* As_TeamType();
    operator TeamTypeClass*() { return As_TeamType(); }
    /// VA: 0x006E75A0
    TerrainClass* As_Terrain();
    operator TerrainClass*() { return As_Terrain(); }
    /// VA: 0x006E7670
    BulletClass* As_Bullet();
    operator BulletClass*() { return As_Bullet(); }
    /// VA: 0x006E7740
    AnimClass* As_Anim();
    operator AnimClass*() { return As_Anim(); }
    /// VA: 0x006E7810
    TeamClass* As_Team();
    operator TeamClass*() { return As_Team(); }
    /// VA: 0x006E78E0
    InfantryClass* As_Infantry();
    operator InfantryClass*() { return As_Infantry(); }
    /// VA: 0x006E79B0
    UnitClass* As_Unit();
    operator UnitClass*() { return As_Unit(); }
    /// VA: 0x006E7A80
    BuildingClass* As_Building();
    operator BuildingClass*() { return As_Building(); }
    /// VA: 0x006E7B50
    AircraftClass* As_Aircraft();
    operator AircraftClass*() { return As_Aircraft(); }
    /// VA: 0x006E7C20
    CellClass* As_Cell();
    operator CellClass*() { return As_Cell(); }

    int m_ID;
    unsigned char m_RTTI;
};
#pragma pack(pop)

// Namespace-scope specializations work with both Microsoft and native C++ ABIs.
template<> inline AbstractTypeClass* TargetClass::As<AbstractTypeClass>() { return As_AbstractType(); }
template<> inline TagClass* TargetClass::As<TagClass>() { return As_Tag(); }
template<> inline TagTypeClass* TargetClass::As<TagTypeClass>() { return As_TagType(); }
template<> inline AbstractClass* TargetClass::As<AbstractClass>() { return As_Abstract(); }
template<> inline TechnoClass* TargetClass::As<TechnoClass>() { return As_Techno(); }
template<> inline ObjectClass* TargetClass::As<ObjectClass>() { return As_Object(); }
template<> inline FootClass* TargetClass::As<FootClass>() { return As_Foot(); }
template<> inline TriggerClass* TargetClass::As<TriggerClass>() { return As_Trigger(); }
template<> inline HouseClass* TargetClass::As<HouseClass>() { return As_House(); }
template<> inline TechnoTypeClass* TargetClass::As<TechnoTypeClass>() { return As_TechnoType(); }
template<> inline TriggerTypeClass* TargetClass::As<TriggerTypeClass>() { return As_TriggerType(); }
template<> inline TeamTypeClass* TargetClass::As<TeamTypeClass>() { return As_TeamType(); }
template<> inline TerrainClass* TargetClass::As<TerrainClass>() { return As_Terrain(); }
template<> inline BulletClass* TargetClass::As<BulletClass>() { return As_Bullet(); }
template<> inline AnimClass* TargetClass::As<AnimClass>() { return As_Anim(); }
template<> inline TeamClass* TargetClass::As<TeamClass>() { return As_Team(); }
template<> inline InfantryClass* TargetClass::As<InfantryClass>() { return As_Infantry(); }
template<> inline UnitClass* TargetClass::As<UnitClass>() { return As_Unit(); }
template<> inline BuildingClass* TargetClass::As<BuildingClass>() { return As_Building(); }
template<> inline AircraftClass* TargetClass::As<AircraftClass>() { return As_Aircraft(); }
template<> inline CellClass* TargetClass::As<CellClass>() { return As_Cell(); }
static_assert(sizeof(TargetClass) == 5 && alignof(TargetClass) == 1);
static_assert(offsetof(TargetClass, m_ID) == 0 && offsetof(TargetClass, m_RTTI) == 4);
