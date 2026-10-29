// YRpp 9402d7da; constructor 00644BE0, paired destructor and primary vtable
// calibrated against supplied gamemd exports.
#include "yrpp/ParticleTypeClass.h"
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
#include <cstdlib>

namespace {
DynamicVectorClass<ParticleTypeClass*> types;
}
DynamicVectorClass<ParticleTypeClass*>& ParticleTypeClass::Array = types;
ParticleTypeClass* YRPP_FASTCALL ParticleTypeClass::Find(const char* id) { return game::find_type(Array, id); }
#if !defined(RA2_YRPP_GAME)
int YRPP_FASTCALL ParticleTypeClass::FindIndex(const char* id) {
    if (!id || !_strcmpi(id, "<none>")) return -1;
    const int index = game::find_type_index(Array, id);
    if (index >= 0) return index;
    try {
        // 0x645430 treats only <none> as a sentinel and creates unknown names.
        // A nullable allocation is distinct from a constructor/registry failure.
        void* storage = YRMemory::Allocate(sizeof(ParticleTypeClass));
        auto* type = storage ? ::new (storage) ParticleTypeClass(id) : nullptr;
        // The original scans Count directly, without the vector's initialization gate.
        for (int i = 0; i < Array.Count; ++i)
            if (Array[i] == type) return i;
        return -1;
    } catch (...) {
        // An internal construction failure cannot unwind across the game ABI.
        std::abort();
    }
}
#endif

ParticleTypeClass::ParticleTypeClass(const char* pID)
    : ObjectTypeClass(pID), NextParticleOffset{}, XVelocity{}, YVelocity{}, MinZVelocity{}, ZVelocityRange{},
      ColorSpeed{}, ColorList{}, StartColor1{}, StartColor2{}, MaxDC{}, MaxEC{}, Warhead{}, Damage{}, StartFrame{},
      NumLoopFrames{}, Translucency{}, WindEffect{}, Velocity{}, Deacc{}, Radius{}, DeleteOnStateLimit{}, EndStateAI{},
      StartStateAI{}, StateAIAdvance{}, FinalDamageState{}, Translucent25State{}, Translucent50State{}, Normalized{},
      NextParticle{}, BehavesLike{} {
    ObjectTypeClass::IsLogic = false;
    NextParticleOffset.X = 0;
    NextParticleOffset.Y = 0;
    NextParticleOffset.Z = 0;
    XVelocity = 1;
    YVelocity = 1;
    MinZVelocity = 0;
    ZVelocityRange = 1;
    ColorSpeed = 0.0;
    StartColor1.R = 0x0u;
    StartColor1.G = 0x0u;
    StartColor1.B = 0x0u;
    StartColor2.R = 0x0u;
    StartColor2.G = 0x0u;
    StartColor2.B = 0x0u;
    MaxDC = 0;
    MaxEC = 1;
    Warhead = nullptr;
    Damage = 0;
    StartFrame = 0;
    NumLoopFrames = 1;
    Translucency = 0;
    WindEffect = 0;
    Velocity = 0.0f;
    Deacc = 0.0f;
    Radius = 0;
    DeleteOnStateLimit = false;
    EndStateAI = 0x0u;
    StartStateAI = 0x0u;
    StateAIAdvance = 0x4u;
    FinalDamageState = 0x0u;
    Translucent25State = 0xffu;
    Translucent50State = 0xffu;
    Normalized = false;
    NextParticle = -1;
    BehavesLike = static_cast<ParticleBehavior>(0xffffffffu);
    Array.AddItem(this);
    TypeExpirationListeners.AddItem(this);
}

ParticleTypeClass::~ParticleTypeClass() {
    NotifyTypeExpired();
    TypeExpirationListeners.Remove(this);
    Array.Remove(this);
}
void ParticleTypeClass::PointerExpired(AbstractClass* object, bool removed) {
    (void)removed;
    if (Warhead == object) Warhead = nullptr;
}
