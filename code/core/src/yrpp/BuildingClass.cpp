#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"

TechnoTypeClass* BuildingClass::GetSecretProduction() const {
    auto const pType = this->Type;

    if (pType->SecretInfantry) {
        return pType->SecretInfantry;
    }
    if (pType->SecretUnit) {
        return pType->SecretUnit;
    }
    if (pType->SecretBuilding) {
        return pType->SecretBuilding;
    }
    return this->SecretProduction;
}

#if !defined(RA2_YRPP_GAME)
// Persistence is not supported; the IPersistStream slots remain for ABI layout.
HRESULT YRPP_STDCALL BuildingClass::Save(IStream*, BOOL) { return static_cast<HRESULT>(0x80004001u); }
HRESULT YRPP_STDCALL BuildingClass::Load(IStream*) { return static_cast<HRESULT>(0x80004001u); }
#endif
