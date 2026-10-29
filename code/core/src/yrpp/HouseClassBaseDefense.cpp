// Special base defense nodes and their original scoring dependencies.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/IndexClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <memory>
namespace {
int weight(const BuildingTypeClass* type, int category) {
    return category == 0 ? type->AntiAirValue : category == 1 ? type->AntiArmorValue : type->AntiInfantryValue;
}
void defenses(HouseClass& house, DynamicVectorClass<BuildingTypeClass*>& output,
              const DynamicVectorClass<BuildingTypeClass*>& owned, int category) {
    output.Clear();
    const unsigned owners = 1u << (unsigned(house.Type->FindParentCountryIndex()) & 31u);
    const auto& rules = *RulesClass::Instance;
    const auto& types = house.Type->SideIndex == 0   ? rules.AlliedBaseDefenses
                        : house.Type->SideIndex == 1 ? rules.SovietBaseDefenses
                                                     : rules.ThirdBaseDefenses;
    for (int i = types.Count - 1; i >= 0; --i) {
        auto* type = types[i];
        if ((type->OwnerFlags & owners) && type->TechLevel <= house.TechLevel && weight(type, category) > 0 &&
            house.AllPrerequisitesAvailable(type, owned, owned.Count))
            output.AddItem(type);
    }
}
unsigned facing(int x, int y) {
    return static_cast<unsigned short>(game::x87_integer((Math::atan2(y, x) - 1.570796326794897) * -10430.06004058427));
}
}
DynamicVectorClass<BuildingTypeClass*>* HouseClass::GetAntiAirDefenses(
    DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) noexcept {
    try {
        defenses(*this, output, owned, 0);
    } catch (...) {
    }
    return &output;
}
DynamicVectorClass<BuildingTypeClass*>* HouseClass::GetAntiArmorDefenses(
    DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) noexcept {
    try {
        defenses(*this, output, owned, 1);
    } catch (...) {
    }
    return &output;
}
DynamicVectorClass<BuildingTypeClass*>* HouseClass::GetAntiInfantryDefenses(
    DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) noexcept {
    try {
        defenses(*this, output, owned, 2);
    } catch (...) {
    }
    return &output;
}
void HouseClass::PredictEnemyForces() noexcept {
    if (EnemyHouseIndex != -1) {
        const auto* enemy = Array[EnemyHouseIndex];
        int values[]{enemy->TotalOwnedAircraftCost + 3000, enemy->TotalOwnedVehicleCost + 3000,
                     enemy->TotalOwnedInfantryCost + 3000};
        for (auto& value : values) {
            const int range = game::x87_integer(
                double(RulesClass::Instance->AIForcePredictionFudge[int(AIDifficulty)]) * double(0.01f) * value);
            value += ScenarioClass::Instance->Random.RandomRanged(-range, range);
        }
        const int total = values[0] + values[1] + values[2];
        if (total > 0) {
            PredictionEnemyArmor = game::x87_double_to_float(double(values[1]) / total);
            PredictionEnemyAir = game::x87_double_to_float(double(values[0]) / total);
            PredictionEnemyInfantry = game::x87_double_to_float(double(values[2]) / total);
            return;
        }
    }
    PredictionEnemyArmor = PredictionEnemyAir = PredictionEnemyInfantry = 0.33f;
}
void YRPP_STDCALL HouseClass::AccumulateBaseDefense(const BuildingClass* building, int value, int* scores,
                                                    const RectangleStruct* bounds) noexcept {
    value = std::max(value, 1);
    const int bx = short(building->Location.X / 256), by = short(building->Location.Y / 256);
    for (int y = std::max(by - 6, bounds->Y); y < std::min(by + 6, bounds->Y + bounds->Height); ++y)
        for (int x = std::max(bx - 6, bounds->X); x < std::min(bx + 6, bounds->X + bounds->Width); ++x) {
            const double radius = Math::sqrt(double((x - bx) * (x - bx) + (y - by) * (y - by)));
            if (radius >= 6.0) continue;
            auto& score = scores[x - bounds->X + (y - bounds->Y) * bounds->Width];
            score = game::x87_integer(double(value) / ((std::max(radius, 1.0) - 1.0) * 0.1 + 1.0) + score);
        }
}
int YRPP_FASTCALL HouseClass::ScoreBaseDefenseCell(const HouseClass* house, const CellStruct* cell, int index,
                                                   int zone) {
    const auto& bounds = house->Base.SpacingBounds;
    const int value = house->BaseDefenseScores[cell->X - bounds.X + bounds.Width * (cell->Y - bounds.Y)];
    int angle = 0;
    if (zone != -1) {
        const unsigned direction =
            (((facing(cell->X - house->Base.Center.X, house->Base.Center.Y - cell->Y) >> 7) + 1) >> 1) & 0xFF;
        angle = std::abs(32 * zone - int(direction));
        if (angle >= 128) angle = 255 - angle;
    }
    return index + 1000 * (angle + (value << 7));
}
bool HouseClass::ExpandBaseDefense(int index, DynamicVectorClass<CellStruct>* wallCells) noexcept {
    try {
        const auto& bounds = Base.SpacingBounds;
        if (bounds.Width <= 0 || bounds.Height <= 0) return false;
        const auto count = size_t(bounds.Width) * size_t(bounds.Height);
        struct Free {
            void operator()(int* p) const { YRMemory::Deallocate(p); }
        };
        std::unique_ptr<int, Free> maps[3];
        for (auto& map : maps) {
            map.reset(static_cast<int*>(YRMemory::Allocate(count * sizeof(int))));
            if (!map) return false;
            std::fill_n(map.get(), count, 0);
        }
        struct Reset {
            HouseClass* house;
            ~Reset() { house->BaseDefenseScores = nullptr; }
        } reset{this};
        DynamicVectorClass<CellStruct> zones[4];
        if (wallCells)
            for (auto cell : *wallCells) {
                const int x = short(cell.X - Base.Center.X), y = short(cell.Y - Base.Center.Y);
                zones[(((facing(x, -y) >> 13) + 1) >> 1) & 3].AddItem(cell);
            }
        int totals[3][4]{};
        const int buildingCount = Buildings.Count;
        for (int i = 0; i < buildingCount; ++i) {
            auto* building = Buildings[i];
            const auto cell = building->GetMapCoords();
            const int x = short(cell.X - Base.Center.X), y = short(cell.Y - Base.Center.Y);
            if (!x && !y) continue;
            const auto zone = (((facing(x, -y) >> 13) + 1) >> 1) & 3;
            const int values[]{building->GetAntiAirValue(), building->GetAntiArmorValue(),
                               building->GetAntiInfantryValue()};
            if (values[0] + values[1] + values[2] <= 0) continue;
            for (int n = 0; n < 3; ++n) {
                totals[n][zone] += values[n];
                AccumulateBaseDefense(building, values[n], maps[n].get(), &bounds);
            }
        }
        int minimum = 999999, zone = -1;
        for (int n = 0; n < 4; ++n) {
            const int sum = totals[0][n] + totals[1][n] + totals[2][n];
            if (sum < minimum && (!wallCells || zones[n].Count > 0)) {
                minimum = sum;
                zone = n;
            }
        }
        // The original assumes at least one usable quadrant.
        if (zone < 0) return false;
        PredictEnemyForces();
        DynamicVectorClass<BuildingTypeClass*> owned;
        for (auto* building : Buildings)
            if (!building->Type->IsBaseDefense && building->Type != RulesClass::Instance->WallTower)
                owned.AddItem(building->Type);
        const float total = game::x87_double_to_float(minimum);
        const float air =
            game::x87_double_to_float(PredictionEnemyAir - (minimum ? double(totals[0][zone]) / total : 0.0));
        const float armor =
            game::x87_double_to_float(PredictionEnemyArmor - (minimum ? double(totals[1][zone]) / total : 0.0));
        const float infantry =
            game::x87_double_to_float(PredictionEnemyInfantry - (minimum ? double(totals[2][zone]) / total : 0.0));
        DynamicVectorClass<BuildingTypeClass*> candidates[3];
        GetAntiAirDefenses(candidates[0], owned);
        GetAntiArmorDefenses(candidates[1], owned);
        GetAntiInfantryDefenses(candidates[2], owned);
        int category = air > armor && air > infantry ? 0 : armor > infantry ? 1 : 2;
        if (!candidates[category].Count) {
            category = candidates[1].Count ? 1 : candidates[2].Count ? 2 : 0;
            if (!candidates[category].Count) return false;
        }
        BaseDefenseScores = maps[category].get();
        auto& choices = candidates[category];
        BuildingTypeClass* chosen = nullptr;
        if (choices.Count == 1)
            chosen = choices[0];
        else {
            int sum = 0;
            for (auto* type : choices)
                sum += weight(type, category);
            if (sum) {
                const auto roll = unsigned(ScenarioClass::Instance->Random.RandomRanged(1, sum));
                unsigned cumulative = 0;
                for (auto* type : choices) {
                    cumulative += unsigned(weight(type, category));
                    if (cumulative >= roll) {
                        chosen = type;
                        break;
                    }
                }
            }
        }
        auto* type = Base.BaseNodes[index].BuildingTypeIndex < 0
                         ? chosen
                         : BuildingTypeClass::Array[Base.BaseNodes[index].BuildingTypeIndex];
        CellStruct cell{};
        if (wallCells) {
            IndexClass<int, CellStruct> ranked;
            for (int i = 0; i < zones[zone].Count; ++i) {
                const auto at = zones[zone][i];
                ranked.AddIndex(ScoreBaseDefenseCell(this, &at, i, -1), at);
            }
            ranked.Sort();
            if (!ranked.Count()) return false;
            cell = ranked.begin()->Data;
            wallCells->RemoveItem(wallCells->FindItemIndex(cell));
        } else
            FindBaseBuildingSite(&cell, type, ScoreBaseDefenseCell, 2 * zone);
        if (cell == CellStruct::Empty || !type) return false;
        if (Base.BaseNodes[index].BuildingTypeIndex < 0) {
            Base.BaseNodes[index].BuildingTypeIndex = type->ArrayIndex;
            Base.BaseNodes[index].MapCoords = cell;
        } else {
            Base.BaseNodes[index].MapCoords = cell;
            // WallTower is followed by the defense placeholder in the original plan.
            if (chosen && index + 1 < Base.BaseNodes.Count && Base.BaseNodes[index + 1].BuildingTypeIndex == -1) {
                Base.BaseNodes[index + 1].BuildingTypeIndex = chosen->ArrayIndex;
                Base.BaseNodes[index + 1].MapCoords = cell;
                if (wallCells) {
                    auto* wall = FirstBuildableFromArray(RulesClass::Instance->ConcreteWalls);
                    for (int i = Base.BaseNodes.Count - 1; wall && i >= 0; --i)
                        if (Base.BaseNodes[i].MapCoords == cell &&
                            Base.BaseNodes[i].BuildingTypeIndex == wall->ArrayIndex) {
                            Base.BaseNodes.RemoveItem(i);
                            break;
                        }
                }
            }
        }
        return true;
    } catch (...) {
        BaseDefenseScores = nullptr;
        return false;
    }
}
