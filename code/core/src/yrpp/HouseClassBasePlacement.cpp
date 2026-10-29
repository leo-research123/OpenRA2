// Base perimeter search, YR 0x005060B0, 0x00506B90 and 0x0050B760.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/IndexClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <cstdlib>
namespace {
constexpr CellStruct neighbours[8]{{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
constexpr CellStruct first[8]{{-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}};
constexpr CellStruct step[8]{{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
CellStruct add(CellStruct a, CellStruct b) { return {short(a.X + b.X), short(a.Y + b.Y)}; }
int distance(const CoordStruct& a, const CoordStruct& b) {
    const double x = double(a.X) - b.X, y = double(a.Y) - b.Y, z = double(a.Z) - b.Z;
    return game::x87_integer(Math::sqrt(x * x + z * z + y * y));
}
}
int YRPP_FASTCALL HouseClass::ScoreBaseCell(const HouseClass* house, const CellStruct* cell, int index, int) {
    return index + 1000 * std::max(std::abs(int(cell->X) - house->Base.Center.X),
                                   std::abs(int(cell->Y) - house->Base.Center.Y));
}
bool HouseClass::IsNearBase(BuildingTypeClass* type, const CellStruct& cell) const {
    if (SessionClass::Instance.GameMode == GameMode::Campaign) return true;
    const int spacing = RulesClass::Instance->AIBaseSpacing;
    const int width = 2 * spacing + type->GetFoundationWidth();
    const int height = 2 * spacing + type->GetFoundationHeight(false);
    const unsigned mask = 1u << (unsigned(ArrayIndex) & 31u);
    for (int x = cell.X - spacing - 1; x < cell.X + width + 1; ++x)
        for (int y = cell.Y - spacing - 1; y < cell.Y + height + 1; ++y)
            if (MapClass::Instance.GetCellAt(CellStruct{short(x), short(y)})->BaseSpacerOfHouses & mask) return true;
    return false;
}
CellStruct* HouseClass::FindBaseUpgradeSite(CellStruct* output, BuildingTypeClass* type) {
    auto* parent = BuildingTypeClass::Find(type->PowersUpBuilding);
    BuildingClass* chosen = nullptr;
    if (parent) {
        int level = parent->Upgrades + 10;
        for (auto* building : BuildingClass::Array) {
            if (building->Owner != this || building->Type != parent || !building->CanUpgrade(type, this)) continue;
            if (type->PowersUpToLevel == -1 ? level > building->UpgradeLevel : building->UpgradeLevel == 0) {
                level = building->UpgradeLevel;
                chosen = building;
            }
        }
    }
    *output = chosen ? CellStruct{short(chosen->Location.X / 256), short(chosen->Location.Y / 256)} : CellStruct::Empty;
    return output;
}
CellStruct* HouseClass::FindBaseBuildingSite(CellStruct* output, BuildingTypeClass* type, BaseCellScore score,
                                             int zone) {
    auto& map = MapClass::Instance;
    const auto& rules = *RulesClass::Instance;
    if (!type) {
        *output = CellStruct::Empty;
        return output;
    }
    if (type->Naval) {
        const int width = FirstBuildableFromArray(rules.Shipyard)->GetFoundationWidth() + 2;
        const int height = FirstBuildableFromArray(rules.Shipyard)->GetFoundationHeight(false) + 2;
        const CellStruct closeTo{};
        map.NearByLocation(*output, GetBaseCenter(), SpeedType(5), -1, MovementZone(0), false, width, height, false,
                           false, false, true, closeTo, false, false);
        if (*output != CellStruct::Empty && ConYards[0] &&
            distance(map.GetCellAt(*output)->GetCoords(), ConYards[0]->GetCoords()) > rules.AINavalYardAdjacency * 256)
            *output = CellStruct::Empty;
        return output;
    }
    if (Base.Center == CellStruct::Empty) {
        *output = GetBaseCenter();
        return output;
    }
    IndexClass<int, CellStruct> ranked;
    for (int i = 0; i < Base.Cells_24.Count; ++i) {
        const auto cell = Base.Cells_24[i];
        int value;
        if (type->CloakGenerator) {
            CoordStruct center;
            GetBaseCenterCoords(&center);
            value = i + 1000 * distance(center, {cell.X * 256 + 128, cell.Y * 256 + 128, 0});
        } else
            value = score(this, &cell, i, zone);
        // 0x510860 uses nullable allocation and skips only the failed append.
        // IndexClass's general allocator is checked, so preserve this caller's
        // original failure contract before using its calibrated sort.
        if (ranked.IndexCount == ranked.IndexSize) {
            const int capacity = ranked.IndexSize ? 2 * ranked.IndexSize : 10;
            using Entry = IndexClass<int, CellStruct>::NodeElement;
            auto* items = static_cast<Entry*>(YRMemory::Allocate(size_t(capacity) * sizeof(Entry)));
            if (!items) continue;
            for (int n = 0; n < ranked.IndexCount; ++n)
                ::new (items + n) Entry(ranked.IndexTable[n]);
            YRMemory::Deallocate(ranked.IndexTable);
            ranked.IndexTable = items;
            ranked.IndexSize = capacity;
            ranked.Archive = nullptr;
        }
        ::new (ranked.IndexTable + ranked.IndexCount) IndexClass<int, CellStruct>::NodeElement{value, cell};
        ++ranked.IndexCount;
    }
    ranked.Sort();
    DynamicVectorClass<CellStruct> candidates;
    for (const auto& entry : ranked) candidates.AddItem(entry.Data);
    const int level = map.GetCellAt(Base.Center)->Level;
    const unsigned mask = 1u << (unsigned(ArrayIndex) & 31u);
    for (int pass = 0; pass < 2; ++pass) {
        for (const auto cell : candidates) {
            CellStruct sum{};
            for (const auto delta : neighbours)
                if (map.GetCellAt(add(cell, delta))->BaseSpacerOfHouses & mask) sum = add(sum, delta);
            if (sum == CellStruct::Empty) continue;
            const double angle = Math::atan2(sum.Y, -int(sum.X));
            const auto facing =
                static_cast<unsigned short>(game::x87_integer((angle - 1.570796326794897) * -10430.06004058427));
            const int direction = (((unsigned(facing) >> 12) + 1) >> 1) & 7;
            const int width = type->GetFoundationWidth(), height = type->GetFoundationHeight(false);
            int spacing = rules.AIBaseSpacing + ((type->ProtectWithWall || type->WantsExtraSpace) ? 1 : 0);
            CellStruct offset{};
            if (direction >= 1 && direction <= 3)
                offset.X = short(spacing);
            else if (direction >= 5)
                offset.X = short(1 - width - spacing);
            if (direction >= 3 && direction <= 5)
                offset.Y = short(spacing + 1);
            else if (direction >= 7 || direction <= 1)
                offset.Y = short(1 - height - spacing);
            for (int margin = 0; margin < 2; ++margin) {
                auto at = add(add(cell, offset), first[direction]);
                if (margin == 1) {
                    at = add(at, neighbours[(direction - 4) & 7]);
                    spacing -= rules.AIBaseSpacing;
                }
                for (int attempt = 0; attempt < 3; ++attempt) {
                    RectangleStruct area{at.X - spacing, at.Y - spacing, width + 2 * spacing, height + 2 * spacing};
                    if (map.IsAreaFree(&area, ArrayIndex) && type->CanCreateHere(at, this) &&
                        std::abs(level - int(map.GetCellAt(at)->Level)) < 3 && IsNearBase(type, at)) {
                        *output = at;
                        return output;
                    }
                    at = add(at, step[direction]);
                }
            }
        }
    }
    *output = CellStruct::Empty;
    return output;
}
