// Original hospital/machine-shop recovery amounts, 0x0050D9E0 / 0x0050D9F0.
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include <bit>

int HouseClass::GetInfSelfHealStep() const noexcept {
    return std::bit_cast<int>(unsigned(InfantrySelfHeal) * unsigned(RulesClass::Instance->SelfHealInfantryAmount));
}
int HouseClass::GetUnitSelfHealStep() const noexcept {
    return std::bit_cast<int>(unsigned(UnitsSelfHeal) * unsigned(RulesClass::Instance->SelfHealUnitAmount));
}
