// YR 0x00442C40: building defaults and owner/secret-lab registration.
#include "yrpp/BuildingClass.h"
#include "techno_spatial.hpp"
#include "yrpp/HouseClass.h"
#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<BuildingClass*> secret_labs;
}
DynamicVectorClass<BuildingClass*>& BuildingClass::SecretLabArray = secret_labs;
void BuildingClass::Init() {
    TechnoClass::Init();
    if (Owner) {
        OwnerCountryIndex = Owner->Type->ArrayIndex2;
        Owner->AddTracking(this);
    }
    if (Type) {
        Health = EstimatedHealth = Type->Strength;
        game::techno_spatial_dirty(this);
        Ammo = Type->InitialAmmo == -1 ? Type->Ammo : Type->InitialAmmo;
        PrimaryFacing.SetROT(Type->ROT);
        if (Type->LoadBuildup()) {
            Type->ClearBuildUp();
            HasBuildUp = true;
        } else
            AI_Sellable = false;
        if (Owner && Owner->WarFactoryInfiltrated && !Type->Naval && Type->Trainable && Type->UndeploysInto)
            Veterancy.SetVeteran();
        if (Type->SecretLab) SecretLabArray.AddItem(this);
    }
}
#endif
