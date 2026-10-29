#include "yrpp/DisplayClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
void DisplayClass::InitializeMultiplayerView() {
    CoordStruct center{};
    int count = 0;
    for (auto* object : TechnoClass::Array)
        if (!object->InLimbo && object->IsOwnedByCurrentPlayer) {
            center.X += object->Location.X; center.Y += object->Location.Y; ++count;
        }
    if (count) { center.X /= count; center.Y /= count; }
    else if (HouseClass::CurrentPlayer) HouseClass::CurrentPlayer->GetBaseCenterCoords(&center);
    center.Z = 0;
    auto& scenario = *ScenarioClass::Instance;
    scenario.View1 = scenario.View2 = scenario.View3 = scenario.View4 = {short(center.X / 256), short(center.Y / 256)};
    scenario.AltHomeCell = scenario.HomeCell;
    if (TacticalClass::Instance) TacticalClass::Instance->SetTacticalPosition(&center);
}
