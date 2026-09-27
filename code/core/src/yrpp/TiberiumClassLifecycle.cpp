// YRpp 9402d7da; constructor 007216C0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/TiberiumClass.h"
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

namespace { DynamicVectorClass<TiberiumClass*> types; }
DynamicVectorClass<TiberiumClass*>& TiberiumClass::Array = types;
TiberiumClass* YRPP_FASTCALL TiberiumClass::Find(const char* id) { return game::find_type(Array, id); }
int YRPP_FASTCALL TiberiumClass::FindIndex(const char* id) { return game::find_type_index(Array, id); }

TiberiumClass::TiberiumClass(const char* pID)
    : AbstractTypeClass(pID),
      ArrayIndex{},
      Spread{},
      SpreadPercentage{},
      Growth{},
      GrowthPercentage{},
      Value{},
      Power{},
      Color{},
      Debris{},
      Image{},
      NumFrames{},
      NumImages{},
      NumSlopes{},
      SpreadLogic{},
      GrowthLogic{} {
    ArrayIndex = -1;
    Spread = 0;
    SpreadPercentage = 0.1;
    Growth = 0;
    GrowthPercentage = 0.1;
    Value = 0;
    Power = 0;
    Color = 0;
    Image = nullptr;
    NumFrames = 0;
    NumImages = 0;
    NumSlopes = 0;
    SpreadLogic.Timer.Start(0);
    GrowthLogic.Timer.Start(0);
    ArrayIndex = Array.Count;
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
}

TiberiumClass::~TiberiumClass() {
    NotifyTypeExpired();
    Array.Remove(this);
    TypeExpirationListeners.Remove(this);
    SpreadLogic.Destruct(); GrowthLogic.Destruct();
    SpreadLogic.Count = 0; GrowthLogic.Count = 0;
    Debris.~TypeList();
}
int TiberiumClass::GetArrayIndex() const { return ArrayIndex; }
void TiberiumClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    for (int i = Debris.Count - 1; i >= 0; --i) if (Debris[i] == object) Debris.RemoveItem(i);
    if (Image == object) Image = nullptr;
}
