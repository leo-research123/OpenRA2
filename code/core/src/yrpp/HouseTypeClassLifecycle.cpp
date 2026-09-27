// YRpp 9402d7da; constructor 005113F0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/HouseTypeClass.h"
#include "type_registry.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TActionClass.h"
#include <cstring>
#include <new>

namespace { DynamicVectorClass<HouseTypeClass*> types; }
DynamicVectorClass<HouseTypeClass*>& HouseTypeClass::Array = types;
HouseTypeClass* YRPP_FASTCALL HouseTypeClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL HouseTypeClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

HouseTypeClass::HouseTypeClass(const char* pID)
    : AbstractTypeClass(pID),
      ParentCountry{},
      align_B1{},
      ArrayIndex{},
      ArrayIndex2{},
      SideIndex{},
      ColorSchemeIndex{},
      align_C4{},
      FirepowerMult{},
      GroundspeedMult{},
      AirspeedMult{},
      ArmorMult{},
      ROFMult{},
      CostMult{},
      BuildtimeMult{},
      ArmorInfantryMult{},
      ArmorUnitsMult{},
      ArmorAircraftMult{},
      ArmorBuildingsMult{},
      ArmorDefensesMult{},
      CostInfantryMult{},
      CostUnitsMult{},
      CostAircraftMult{},
      CostBuildingsMult{},
      CostDefensesMult{},
      SpeedInfantryMult{},
      SpeedUnitsMult{},
      SpeedAircraftMult{},
      BuildtimeInfantryMult{},
      BuildtimeUnitsMult{},
      BuildtimeAircraftMult{},
      BuildtimeBuildingsMult{},
      BuildtimeDefensesMult{},
      IncomeMult{},
      VeteranInfantry{},
      VeteranUnits{},
      VeteranAircraft{},
      Suffix{},
      Prefix{},
      Multiplay{},
      MultiplayPassive{},
      WallOwner{},
      SmartAI{},
      padding_1A9{} {
    ArrayIndex = -1;
    ArrayIndex2 = -1;
    SideIndex = -1;
    ColorSchemeIndex = 0;
    FirepowerMult = 1.0;
    GroundspeedMult = 1.0;
    AirspeedMult = 1.0;
    ArmorMult = 1.0;
    ROFMult = 1.0;
    CostMult = 1.0;
    BuildtimeMult = 1.0;
    ArmorInfantryMult = 1.0f;
    ArmorUnitsMult = 1.0f;
    ArmorAircraftMult = 1.0f;
    ArmorBuildingsMult = 1.0f;
    ArmorDefensesMult = 1.0f;
    CostInfantryMult = 1.0f;
    CostUnitsMult = 1.0f;
    CostAircraftMult = 1.0f;
    CostBuildingsMult = 1.0f;
    CostDefensesMult = 1.0f;
    SpeedInfantryMult = 1.0f;
    SpeedUnitsMult = 1.0f;
    SpeedAircraftMult = 1.0f;
    BuildtimeInfantryMult = 1.0f;
    BuildtimeUnitsMult = 1.0f;
    BuildtimeAircraftMult = 1.0f;
    BuildtimeBuildingsMult = 1.0f;
    BuildtimeDefensesMult = 1.0f;
    IncomeMult = 1.0f;
    Suffix[0] = static_cast<char>(0);
    Prefix = static_cast<char>(65);
    Multiplay = false;
    MultiplayPassive = false;
    WallOwner = true;
    SmartAI = false;
    Create_ID();
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
    ArrayIndex2 = ArrayIndex;
}

HouseTypeClass::~HouseTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
}
int HouseTypeClass::GetArrayIndex() const { return ArrayIndex; }

// 005117D0: display Name (not the wchar_t UIName pointer) or ID, Random=-2.
int YRPP_FASTCALL HouseTypeClass::FindIndexOfName(const char* name) {
    if (!name) return -1;
    if (!_strcmpi(name, "Random")) return -2;
    for (int i = 0; i < Array.Count; ++i) {
        const auto* type = Array[i];
        if (type && (!_strcmpi(type->Name, name) || !_strcmpi(type->ID, name)))
            return type->ArrayIndex2;
    }
    return -1;
}
