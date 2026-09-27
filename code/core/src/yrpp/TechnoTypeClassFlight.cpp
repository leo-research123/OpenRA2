// Original YR 0x717800: per-type override or Rules flight altitude.
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/RulesClass.h"
int TechnoTypeClass::GetFlightLevel() const {return FlightLevel==-1?RulesClass::Instance->FlightLevel:FlightLevel;}
