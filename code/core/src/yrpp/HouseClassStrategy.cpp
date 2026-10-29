// Strategic state and economic phases, YR 0x4FD500.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
#include <limits>

int HouseClass::AI_StrategicUpdate() {
    if (!Unknown_Timer_5640.GetTimeLeft() && EnemyHouseIndex == -1 &&
        SessionClass::Instance.GameMode != GameMode::Campaign && !Type->MultiplayPassive) {
        const auto center = GetBaseCenter();
        if (center != CellStruct::Empty) {
            const auto origin = MapClass::Instance.GetCellAt(center)->GetCoords();
            if (origin != CoordStruct::Empty) {
                int nearest = std::numeric_limits<int>::max();
                HouseClass* selected = nullptr;
                for (int i = 0; i < Array.Count; ++i) {
                    auto* house = Array[i];
                    if (house == this || house->Type->MultiplayPassive || house->Defeated) continue;
                    // 0x4FD635 deliberately reads THIS house again, not the
                    // candidate's center. Preserve the original selection bug.
                    const auto cell = GetBaseCenter();
                    const auto at = cell == CellStruct::Empty ? CoordStruct::Empty
                                                             : MapClass::Instance.GetCellAt(cell)->GetCoords();
                    const auto delta = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
                    const double x = delta(at.X, origin.X), y = delta(at.Y, origin.Y), z = delta(at.Z, origin.Z);
                    const int distance = game::x87_integer(Math::sqrt(x * x + z * z + y * y));
                    if (distance < nearest) {
                        nearest = distance;
                        selected = house;
                    }
                }
                if (selected) RegisterDamage(1, selected);
            }
        }
    }
    if (EnemyHouseIndex != -1) {
        auto* enemy = Array[EnemyHouseIndex];
        if (enemy->Defeated) {
            for (int i = 0; i < AngerNodes.Count; ++i)
                if (AngerNodes[i].House == enemy) {
                    RegisterDamage(std::bit_cast<int>(0u - unsigned(AngerNodes[i].AngerLevel)), enemy);
                    break;
                }
            // RegisterDamage can select a different enemy; the EXE still clears it.
            EnemyHouseIndex = -1;
        }
    }
    if (SessionClass::Instance.GameMode != GameMode::Campaign || IQLevel2 >= RulesClass::Instance->SuperWeapons)
        AI_TryFireSW();
    if (AIMode == ::AIMode::SellAll) {
        Fire_Sale();
        All_To_Hunt();
    } else {
        if (AIMode == ::AIMode::General && Available_Money() < 25) AIMode = ::AIMode::LowOnCash;
        if (AIMode == ::AIMode::LowOnCash && Available_Money() >= 25) AIMode = ::AIMode::General;
        const int until = std::bit_cast<int>(unsigned(LATime) + 900u);
        if (AIMode == ::AIMode::BuildBase && until < Unsorted::CurrentFrame) AIMode = ::AIMode::General;
        if (AIMode != ::AIMode::BuildBase && until > Unsorted::CurrentFrame) AIMode = ::AIMode::BuildBase;
    }
    if (SessionClass::Instance.GameMode != GameMode::Campaign) {
        int productionPriority = 4;
        if (AIMode == ::AIMode::BuildBase) {
            productionPriority = 0;
        } else {
            for (auto* building : Buildings)
                if (building && building->IsAlive && !building->InLimbo && building->Type->Factory != AbstractType::None) {
                    productionPriority = 0;
                    break;
                }
        }
        // Both priorities are evaluated before any sale callback changes the world.
        const int economyPriority = AI_EvaluateEconomy();
        for (int priority = 4; priority > 0; --priority) {
            if (productionPriority == priority && priority == 4) {
                Fire_Sale();
                All_To_Hunt();
            }
            if (economyPriority == priority) AI_RecoverEconomy(priority);
        }
    }
    return ScenarioClass::Instance->Random.RandomRanged(1, 7) + 105;
}
