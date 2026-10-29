// Protection rings and perimeter wall/gate planning, YR 0x50C340 / 0x5082C0.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
namespace {
void insert_after(DynamicVectorClass<BaseNodeClass>& nodes, int index, int type, CellStruct cell) {
    if (!nodes.AddItem({type, cell, false, 0})) return;
    for (int i = nodes.Count - 1; i > index + 1; --i)
        nodes[i] = nodes[i - 1];
    nodes[index + 1] = {type, cell, false, 0};
}
CellStruct add(CellStruct a, CellStruct b) { return {short(a.X + b.X), short(a.Y + b.Y)}; }
bool forbidden(LandType land) { return land == LandType(2) || land == LandType(3) || land == LandType(8); }
}
bool HouseClass::AddBaseProtectionWalls(int index) noexcept {
    try {
        int wall = -1;
        for (auto* type : RulesClass::Instance->ConcreteWalls)
            if (type->AIBasePlanningSide == Type->SideIndex || type->AIBasePlanningSide == -1) {
                wall = type->ArrayIndex;
                break;
            }
        BuildingClass* building = nullptr;
        int previous = index - 1;
        for (; previous >= 0; --previous) {
            building = Base.GetNodeBuilding(previous);
            // The original tests the following node, not the protected building's node.
            if (building && building->Type->ProtectWithWall && Base.BaseNodes[previous + 1].BuildingTypeIndex != wall)
                break;
        }
        if (previous < 0 || !building || !building->Type->ProtectWithWall) return false;
        const int width = building->Type->GetFoundationWidth(), height = building->Type->GetFoundationHeight(false);
        const CellStruct start{short(building->Location.X / 256 - 1), short(building->Location.Y / 256 - 1)};
        auto& nodes = Base.BaseNodes;
        for (int x = 1; x <= width; ++x) {
            insert_after(nodes, previous, wall, add(start, {short(x), 0}));
            insert_after(nodes, previous, wall, add(start, {short(x), short(height + 1)}));
        }
        for (int y = 1; y <= height; ++y) {
            insert_after(nodes, previous, wall, add(start, {0, short(y)}));
            insert_after(nodes, previous, wall, add(start, {short(width + 1), short(y)}));
        }
        insert_after(nodes, previous, wall, start);
        insert_after(nodes, previous, wall, add(start, {short(width + 1), 0}));
        insert_after(nodes, previous, wall, add(start, {0, short(height + 1)}));
        insert_after(nodes, previous, wall, add(start, {short(width + 1), short(height + 1)}));
        return true;
    } catch (...) {
        return false;
    }
}
void HouseClass::AddBasePerimeterWalls() noexcept {
    try {
        const auto& old = Base.SpacingBounds;
        Base.WallBounds = {old.X - 1, old.Y - 1, old.Width + 2, old.Height + 2};
        const auto area = Base.WallBounds;
        DynamicVectorClass<BaseNodeClass> walls, gates;
        const CellStruct starts[]{{short(area.X), short(area.Y)},
                                  {short(area.X), short(area.Y + area.Height - 1)},
                                  {short(area.X), short(area.Y + 1)},
                                  {short(area.X + area.Width - 1), short(area.Y + 1)}};
        const CellStruct ends[]{{short(area.X + area.Width), short(area.Y)},
                                {short(area.X + area.Width), short(area.Y + area.Height - 1)},
                                {short(area.X), short(area.Y + area.Height - 1)},
                                {short(area.X + area.Width - 1), short(area.Y + area.Height - 1)}};
        const CellStruct steps[]{{1, 0}, {1, 0}, {0, 1}, {0, 1}};
        const CellStruct outside[]{{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        auto* wall = FirstBuildableFromArray(RulesClass::Instance->ConcreteWalls);
        auto* eastWest = FirstBuildableFromArray(RulesClass::Instance->EWGates);
        auto* northSouth = FirstBuildableFromArray(RulesClass::Instance->NSGates);
        // The EXE assumes all three rule lists contain an eligible type.
        if (!wall || !eastWest || !northSouth || area.Width <= 0 || area.Height <= 1) return;
        const int level = MapClass::Instance.GetCellAt(Base.Center)->Level;
        CellStruct start{};
        for (int side = 0; side < 4; ++side) {
            int length = 0, obstructions = 0;
            bool active = false;
            const auto step = steps[side];
            auto flush = [&](bool gate) {
                auto at = start;
                for (int n = 0; n < length;) {
                    if (gate && n == length / 2 - 1) {
                        gates.AddItem({(side < 2 ? eastWest : northSouth)->ArrayIndex, at, false, 0});
                        n += 3;
                        at = add(add(add(at, step), step), step);
                    } else {
                        walls.AddItem({wall->ArrayIndex, at, false, 0});
                        ++n;
                        at = add(at, step);
                    }
                }
            };
            for (auto at = starts[side]; at != ends[side]; at = add(at, step)) {
                auto* cell = MapClass::Instance.GetCellAt(at);
                auto* neighbour = MapClass::Instance.GetCellAt(add(at, outside[side]));
                const bool height = std::abs(int(cell->Level) - level) <= 2;
                const bool land = !forbidden(cell->LandType) && !forbidden(neighbour->LandType);
                const bool overlay = cell->OverlayTypeIndex == -1;
                const bool building = !cell->GetBuilding() && !neighbour->GetBuilding();
                const bool terrain = !cell->GetTerrain(false) && !neighbour->GetTerrain(false);
                const bool slope = cell->SlopeIndex == 0;
                const bool usable = MapClass::Instance.IsWithinUsableArea(at, true);
                if (height && land && overlay && building && terrain && slope && usable) {
                    if (!active) {
                        active = true;
                        length = 1;
                        start = at;
                    } else
                        ++length;
                    continue;
                }
                bool gate;
                if (height && land && building && terrain && usable) {
                    gate = false;
                    ++obstructions;
                } else {
                    gate = true;
                    if (obstructions > 0) {
                        gate = false;
                        obstructions = 0;
                    }
                }
                if (length >= 5 || !gate) flush(gate);
                length = 0;
                active = false;
            }
            if (length >= 5) flush(true);
        }
        for (const auto& node : walls)
            Base.BaseNodes.AddItem(node);
        for (const auto& node : gates)
            Base.BaseNodes.AddItem(node);
        Base.SpacingBounds = Base.WallBounds;
    } catch (...) {
    }
}
