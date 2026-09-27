// Original Techno planning interface, YR 0x0070F070. Delegates through both
// original virtual slots; capability belongs to the TechnoType instance.
#include "yrpp/TechnoClass.h"
#include "yrpp/TechnoTypeClass.h"
bool TechnoClass::CanUseWaypoint() const {return GetTechnoType()->CanUseWaypoint();}
