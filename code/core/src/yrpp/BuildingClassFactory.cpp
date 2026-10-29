// Factory controller recovered from YR 0x004500F0.
#include "yrpp/BuildingClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "x87_integer.hpp"
#include <new>
void BuildingClass::FactoryAI() {
    if (Factory && Factory->IsDone() && !FactoryRetryTimer.GetTimeLeft()) {
        auto* product = Factory->Object;
        switch (KickOutUnit(product, CellStruct::Empty)) {
            case KickOutResult::Failed:
                if (product->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(product)->Type->Naval)
                    product->Owner->NodeLogic_1F0 = false;
                Factory->AbandonProduction();
                delete Factory;
                Factory = nullptr;
                break;
            case KickOutResult::Busy:
                FactoryRetryTimer.Start(game::x87_integer(RulesClass::Instance->PlacementDelay * 900.0));
                break;
            case KickOutResult::Succeeded:
                if (Type->CreateUnitSound != -1) VocClass::PlayIndexAtPos(Type->CreateUnitSound, Location, 0);
                Owner->JustBuilt(product);
                Factory->CompletedProduction();
                delete Factory;
                Factory = nullptr;
                break;
        }
    }
    if (!Owner->Production || GetCurrentMission() == Mission::Construction || GetCurrentMission() == Mission::Selling ||
        Type->Factory == AbstractType::None)
        return;
    if (Factory) {
        if (!FactoryRetryTimer.GetTimeLeft() && (!Factory->Production.Rate || Factory->IsSuspended)) {
            Factory->AbandonProduction();
            delete Factory;
            Factory = nullptr;
        }
        return;
    }
    if (Owner->Available_Money() <= 10) return;
    auto* type = Owner->GetProductionType(Type->Factory, false);
    if (!type || (type->WhatAmI() == AbstractType::UnitType && type->Naval != Type->Naval)) return;
    Factory = new (std::nothrow) FactoryClass;
    if (!Factory) return;
    if (Factory->DemandProduction(type, Owner, false)) {
        Factory->Unsuspend(false);
        return;
    }
    delete Factory;
    Factory = nullptr;
}
