// Original mobile iron-curtain application, 0x004DEAE0.
#include "yrpp/FootClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ParasiteClass.h"
DamageState FootClass::IronCurtain(int duration, HouseClass* source, bool forceShield) {
    auto* type = GetTechnoType();
    if (type->Organic) {
        int damage = GetTechnoType()->Strength;
        return ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, false, false, nullptr);
    }
    if (ParasiteEatingMe && ParasiteEatingMe->ParasiteImUsing) {
        ParasiteEatingMe->ParasiteImUsing->SuppressionTimer.Start(50);
        ParasiteEatingMe->ParasiteImUsing->ExitUnit();
    }
    ParalysisTimer.Start(0);
    return TechnoClass::IronCurtain(duration, source, forceShield);
}
