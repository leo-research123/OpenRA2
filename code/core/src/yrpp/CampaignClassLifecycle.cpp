// YRpp 9402d7da; constructor 0046CB60, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/CampaignClass.h"
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
#include <cwchar>
#include <iterator>
#include <stdexcept>

namespace { DynamicVectorClass<CampaignClass*> types; }
DynamicVectorClass<CampaignClass*>& CampaignClass::Array = types;
CampaignClass* YRPP_FASTCALL CampaignClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL CampaignClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

CampaignClass::CampaignClass(const char *name, const wchar_t* initialDescription)
    : AbstractTypeClass(name),
      idxCD{},
      Scenario{},
      FinalMovie{},
      Description{} {
    idxCD = -1;
    Scenario[0] = static_cast<char>(0);
    FinalMovie = -1;
    if (!initialDescription) throw std::invalid_argument("Campaign initial description is required");
    std::wcsncpy(Description, initialDescription, std::size(Description) - 1);
    Description[std::size(Description) - 1] = 0;
    Array.AddItem(this);
}

CampaignClass::~CampaignClass() {
    Array.Remove(this);
}
