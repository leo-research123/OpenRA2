#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "x87_integer.hpp"

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

int TechnoClass::TimeToBuild() const {
    int time=GetType()->GetBuildSpeed();
    time=game::x87_integer(Owner->GetBuildTimeMultiplier(GetTechnoType())*time);
    time=game::x87_integer(double(time)*GetTechnoType()->BuildTimeMultiplier);
    const float power=static_cast<float>(Owner->GetPowerPercentage());
    const auto& rules=*RulesClass::Instance;
    double speed=1.0-(1.0-double(power))*rules.LowPowerPenaltyModifier;
    if(speed<=rules.MinLowPowerProductionSpeed)speed=rules.MinLowPowerProductionSpeed;
    if(power<1.0f&&speed>=rules.MaxLowPowerProductionSpeed)speed=rules.MaxLowPowerProductionSpeed;
    if(speed==0.0)speed=double(0.01f);
    time=game::x87_integer(double(time)/speed);
    bool naval=false;
    if(WhatAmI()==AbstractType::Unit)if(auto* type=GetTechnoType())naval=type->Naval;
    const int factories=Owner->CountFactories(WhatAmI(),naval);
    if(rules.MultipleFactory>0.0f&&factories-1>0)
        for(int i=factories-1;i>0;--i)time=game::x87_integer(double(time)*rules.MultipleFactory);
    if(WhatAmI()==AbstractType::Building&&static_cast<const BuildingClass*>(this)->Type->Wall)
        return game::x87_integer(double(time)*rules.WallBuildSpeedCoefficient);
    return time;
}
