#include <YRPP.h>

int TechnoClass::GetIonCannonValue(AIDifficulty const difficulty) const {
	const auto& rules = *RulesClass::Instance;

	const TypeList<int>* pValues = nullptr;
	int value = 1;

	if(auto pUnit = abstract_cast<const UnitClass*>(this)) {
		auto pType = pUnit->Type;

		if(pType->Harvester) {
			pValues = &rules.AIIonCannonHarvesterValue;

		} else if(rules.BuildConst.FindItemIndex(pType->DeploysInto) != -1) {
			pValues = &rules.AIIonCannonMCVValue;

		} else if(pType->Passengers > 0) {
			pValues = &rules.AIIonCannonAPCValue;

		} else {
			value = 2;
		}

	} else if(auto pBuilding = abstract_cast<const BuildingClass*>(this)) {
		auto pType = pBuilding->Type;

		if(pType->Factory == AbstractType::BuildingType) {
			pValues = &rules.AIIonCannonConYardValue;

		} else if(pType->Factory == AbstractType::UnitType && !pType->Naval) {
			pValues = &rules.AIIonCannonWarFactoryValue;

		} else if(pType->PowerBonus > pType->PowerDrain) {
			pValues = &rules.AIIonCannonPowerValue;

		} else if(pType->IsBaseDefense) {
			pValues = &rules.AIIonCannonBaseDefenseValue;

		} else if(pType->IsPlug) {
			pValues = &rules.AIIonCannonPlugValue;

		} else if(pType->IsTemple) {
			pValues = &rules.AIIonCannonTempleValue;

		} else if(pType->HoverPad) {
			pValues = &rules.AIIonCannonHelipadValue;

		} else if(rules.BuildConst.FindItemIndex(pType) != -1) {
			pValues = &rules.AIIonCannonTechCenterValue;

		} else {
			value = 4;
		}

	} else if(auto pInfantry = abstract_cast<const InfantryClass*>(this)) {
		auto pType = pInfantry->Type;

		if(pType->Engineer) {
			pValues = &rules.AIIonCannonEngineerValue;

		} else if(pType->VehicleThief) {
			pValues = &rules.AIIonCannonThiefValue;

		} else {
			value = 2;
		}
	}

	if(pValues) {
		value = pValues->GetItemOrDefault(static_cast<int>(difficulty), value);
	}

	return value;
}
