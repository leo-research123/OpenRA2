#include "yrpp/InfantryClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "x87_integer.hpp"
#include <algorithm>

#if !defined(RA2_YRPP_GAME)
int InfantryClass::Mission_Harvest() {
    auto* cell = Type->Storage ? GetCell() : nullptr;
    if (!Type->Storage || cell->LandType != LandType::Tiberium || GetStoragePercentage() == 1.0) {
        PlayAnim(Sequence::Ready, false, false);
        QueueMission(Mission::Guard, false);
        return 1;
    }
    if (SequenceAnim != Sequence::Shovel) PlayAnim(Sequence::Shovel, false, false);
    const int index = cell->GetContainedTiberiumIndex();
    const float capacity = game::x87_multiply_to_float(Type->Storage, 1.0);
    const int amount = game::x87_integer(std::min(1.0, double(capacity) - Tiberium.GetTotalAmount()));
    const int reduced = cell->ReduceTiberium(amount);
    if (reduced > 0) Tiberium.AddAmount(float(reduced), index);
    return Type->HarvestRate;
}

void InfantryClass::DeliverTiberium(TechnoClass* destination) {
    bool delivered = false;
    for (int index = 0; index < 4; ++index) {
        const float amount = Tiberium.GetAmount(index);
        if (!(amount > 0)) continue;
        auto* house = destination->Owner;
        int purifiers = house->NumOrePurifiers;
        if (!house->IsHumanPlayer && SessionClass::Instance.GameMode != GameMode::Campaign)
            purifiers += RulesClass::Instance->AIVirtualPurifiers[house->GetAIDifficultyIndex()];
        const float bonus =
            game::x87_multiply_to_float(double(purifiers) * RulesClass::Instance->PurifierBonus, amount);
        const float removed = Tiberium.RemoveAmount(amount, index);
        if (removed > 0) {
            delivered = true;
            house->GiveTiberium(removed, index);
            if (bonus > 0) house->GiveTiberium(bonus, index);
        }
    }
    if (delivered) destination->UpdateRefinerySmokeSystems();
}
#endif
