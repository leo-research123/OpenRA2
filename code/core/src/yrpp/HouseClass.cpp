#include <YRPP.h>

int HouseClass::CountOwnedNow(const TechnoTypeClass* const pItem) const {
	switch(pItem->WhatAmI()) {
	case AbstractType::BuildingType:
		return this->CountOwnedNow(
			static_cast<BuildingTypeClass const*>(pItem));

	case AbstractType::UnitType:
		return this->CountOwnedNow(
			static_cast<UnitTypeClass const*>(pItem));

	case AbstractType::InfantryType:
		return this->CountOwnedNow(
			static_cast<InfantryTypeClass const*>(pItem));

	case AbstractType::AircraftType:
		return this->CountOwnedNow(
			static_cast<AircraftTypeClass const*>(pItem));

	default:
		__assume(0);
	}
}

int HouseClass::CountOwnedAndPresent(const TechnoTypeClass* const pItem) const {
	switch(pItem->WhatAmI()) {
	case AbstractType::BuildingType:
		return this->CountOwnedAndPresent(
			static_cast<BuildingTypeClass const*>(pItem));

	case AbstractType::UnitType:
		return this->CountOwnedAndPresent(
			static_cast<UnitTypeClass const*>(pItem));

	case AbstractType::InfantryType:
		return this->CountOwnedAndPresent(
			static_cast<InfantryTypeClass const*>(pItem));

	case AbstractType::AircraftType:
		return this->CountOwnedAndPresent(
			static_cast<AircraftTypeClass const*>(pItem));

	default:
		__assume(0);
	}
}

int HouseClass::CountOwnedEver(TechnoTypeClass const* const pItem) const {
	switch(pItem->WhatAmI()) {
	case AbstractType::BuildingType:
		return this->CountOwnedEver(
			static_cast<BuildingTypeClass const*>(pItem));

	case AbstractType::UnitType:
		return this->CountOwnedEver(
			static_cast<UnitTypeClass const*>(pItem));

	case AbstractType::InfantryType:
		return this->CountOwnedEver(
			static_cast<InfantryTypeClass const*>(pItem));

	case AbstractType::AircraftType:
		return this->CountOwnedEver(
			static_cast<AircraftTypeClass const*>(pItem));

	default:
		__assume(0);
	}
}

bool HouseClass::CanExpectToBuild(const TechnoTypeClass* const pItem) const {
	auto const parentOwnerMask = this->Type->FindParentCountryIndex();
	return this->CanExpectToBuild(pItem, parentOwnerMask);
}

bool HouseClass::CanExpectToBuild(const TechnoTypeClass* const pItem, int const idxParent) const {
	auto const parentOwnerMask = 1u << idxParent;
	if(pItem->InOwners(parentOwnerMask)) {
		if(this->InRequiredHouses(pItem)) {
			if(!this->InForbiddenHouses(pItem)) {
				auto const BaseSide = pItem->AIBasePlanningSide;
				if(BaseSide == -1 || BaseSide == this->Type->SideIndex) {
					return true;
				}
			}
		}
	}
	return false;
}

int HouseClass::FindSuperWeaponIndex(SuperWeaponType const type) const {
	for(int i = 0; i < this->Supers.Count; ++i) {
		if(this->Supers.Items[i]->Type->Type == type) {
			return i;
		}
	}
	return -1;
}

SuperClass* HouseClass::FindSuperWeapon(SuperWeaponType const type) const {
	auto index = this->FindSuperWeaponIndex(type);
	return this->Supers.GetItemOrDefault(index);
}

bool HouseClass::IsIonCannonEligibleTarget(const TechnoClass* const pTechno) const {
	if(pTechno->InWhichLayer() == Layer::Ground && pTechno->IsAlive && !pTechno->InLimbo) {
		return true;
	}

	// hard difficulty shoots the tank in the factory
	if(this->AIDifficulty == AIDifficulty::Hard) {
		for(const auto* pFactory : FactoryClass::Array) {
			if(pFactory->Object == pTechno
				&& pFactory->Production.Rate
				&& !pFactory->IsSuspended)
			{
				return true;
			}
		}
	}

	return false;
}
