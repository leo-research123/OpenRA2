// Campaign handoff, YR 0x004C6140 / 0x004C6180 / 0x004C6210.
#include "yrpp/EvadeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstring>

#if !defined(RA2_YRPP_GAME)
namespace {
int inherited_money(int money, double ratio, int cap) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // Keep the product in ST(0) through the cap test and original int64 conversion.
    const double ceiling = cap;
    const unsigned short control = 0x0E7F;
    unsigned long long result;
    // clang-format off
    __asm fild money
    __asm fmul ratio
    __asm cmp cap, -1
    __asm je convert_value
    __asm fcom ceiling
    __asm fnstsw ax
    __asm test ah, 1
    __asm jnz convert_value
    __asm fstp st(0)
    __asm fld ceiling
    __asm convert_value:
    __asm fldcw control
    __asm fistp result
    // clang-format on
    return std::bit_cast<int>(static_cast<unsigned>(result));
#else
    double amount = double(money) * ratio;
    if (cap != -1 && amount >= cap) amount = cap;
    return game::x87_integer(amount);
#endif
}
}

void EvadeClass::Reset() noexcept {
    std::memset(CarryOverGlobals, 0, sizeof(CarryOverGlobals));
    CarryOverMoney = CarryOverTimer = 0;
    CarryOverDifficulty = 1;
    CarryOverStage = 0;
}
void EvadeClass::Save() {
    auto& scenario = *ScenarioClass::Instance;
    for (int i = 0; i < 50; ++i) CarryOverGlobals[i] = scenario.GlobalVariables[i].Value;
    CarryOverMoney = HouseClass::CurrentPlayer->Available_Money();
    CarryOverTimer = scenario.MissionTimer.GetTimeLeft();
    CarryOverDifficulty = static_cast<int>(HouseClass::CurrentPlayer->AIDifficulty);
    CarryOverStage = scenario.Stage;
}
void EvadeClass::Do() {
    auto& scenario = *ScenarioClass::Instance;
    for (int i = 0; i < 50; ++i) scenario.SetGlobal(i, CarryOverGlobals[i]);
    const int money = inherited_money(CarryOverMoney, scenario.CarryOverMoney, scenario.CarryOverCap);
    auto* house = HouseClass::CurrentPlayer;
    house->GiveMoney(money);
    house->StartingCredits = std::bit_cast<int>(static_cast<unsigned>(house->StartingCredits) +
                                               static_cast<unsigned>(money));
    house->AssignHandicap(CarryOverDifficulty);
    if (scenario.TimerInherit && CarryOverTimer > 0) scenario.MissionTimer.Start(CarryOverTimer);
    scenario.Stage = CarryOverStage;
}
#endif
