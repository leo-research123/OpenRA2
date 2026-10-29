// HouseClass::AssignHandicap, YR 0x004F6EC0.
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "x87_double.hpp"

int HouseClass::TeamDelayFor(int difficulty) {
    const auto& delays = RulesClass::Instance->TeamDelays;
    return difficulty >= 0 && difficulty < delays.Count ? delays.Items[difficulty] : 0;
}

// The difficulty indexes the three consecutive Rules difficulty records. The
// products run under the persistent 0x0E7F control word, so each one rounds
// toward zero. The original also stores an uninitialized word into the team
// delay timer's clock; the timer is only started here.
void HouseClass::AssignHandicap(int difficulty) {
    AIDifficulty = static_cast<::AIDifficulty>(difficulty);
    auto& rules = *RulesClass::Instance;
    const auto& d = (&rules.Easy)[difficulty];
    const double bias = rules.GameSpeedBias;
    const auto mul = [](double a, double b) { return game::x87_multiply(a, b); };
    if (SessionClass::Instance.GameMode != GameMode::Campaign) {
        const auto& t = *Type;
        FirepowerMultiplier = mul(d.Firepower, t.FirepowerMult);
        GroundspeedMultiplier = mul(mul(d.GroundSpeed, bias), t.GroundspeedMult);
        AirspeedMultiplier = mul(mul(d.AirSpeed, bias), t.AirspeedMult);
        ArmorMultiplier = mul(d.Armor, t.ArmorMult);
        ROFMultiplier = mul(d.ROF, t.ROFMult);
        CostMultiplier = mul(d.Cost, t.CostMult);
        RepairDelay = d.RepairDelay;
        BuildDelay = d.BuildDelay;
        BuildTimeMultiplier = mul(mul(d.BuildTime, bias), t.BuildtimeMult);
    } else {
        FirepowerMultiplier = d.Firepower;
        GroundspeedMultiplier = mul(d.GroundSpeed, bias);
        AirspeedMultiplier = mul(d.AirSpeed, bias);
        ArmorMultiplier = d.Armor;
        ROFMultiplier = d.ROF;
        CostMultiplier = d.Cost;
        RepairDelay = d.RepairDelay;
        BuildDelay = d.BuildDelay;
        BuildTimeMultiplier = mul(d.BuildTime, bias);
    }
    TeamDelayTimer.Start(175 * ArrayIndex + TeamDelayFor(difficulty));
}
