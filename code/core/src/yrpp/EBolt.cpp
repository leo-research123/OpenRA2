// YR 0x4C1E10..0x4C2C22: electric-bolt ownership and presentation lifetime.
#include "yrpp/EBolt.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Randomizer.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include <bit>

namespace {
DynamicVectorClass<EBolt*> bolts;
}
DynamicVectorClass<EBolt*>& EBolt::Array = bolts;

EBolt::EBolt()
    : Point1(0, 0, 0), Point2(0, 0, 0), unknown_18(0), Random(0), Owner(nullptr), WeaponSlot(0), Lifetime(0x10000),
      AlternateColor(false) {}
EBolt::~EBolt() { ClearOwner(); }
void EBolt::ClearOwner() {
    if (Owner) {
        static_cast<UnitClass*>(Owner)->ElectricBolt = nullptr;
        Owner = nullptr;
    }
}
void EBolt::SetOwner(UnitClass* owner, int weapon) {
    if (owner && owner->WhatAmI() == AbstractType::Unit && owner->IsAlive && !owner->InLimbo) {
        Owner = owner;
        WeaponSlot = weapon;
    }
}
CoordStruct* EBolt::GetSourceCoords(CoordStruct* output) const {
    if (Owner) return Owner->GetFLH(output, WeaponSlot, CoordStruct(0, 0, 0));
    *output = CoordStruct(-1, -1, -1);
    return output;
}
void EBolt::Fire(CoordStruct first, CoordStruct last, DWORD depth) {
    Point1 = first;
    Point2 = last;
    unknown_18 = depth;
    Random = Randomizer::Global.RandomRanged(0, 256);
    try {
        Array.AddItem(this);
    } catch (const std::bad_alloc&) {
        // Native vector allocation throws; original SetCapacity returned false.
        Array.IsInitialized = true;
    }
    // The original checks allocation before constructing the endpoint spark.
    void* memory = YRMemory::Allocate(sizeof(ParticleSystemClass));
    if (memory)
        ::new (memory) ParticleSystemClass(RulesClass::Instance->DefaultSparkSystem, last, nullptr, nullptr,
                                           CoordStruct(-1, -1, -1), nullptr);
}
void YRPP_CDECL EBolt::DeleteAll() {
    for (int i = 0; i < Array.Count; ++i)
        GameDelete(Array[i]);
    Array.Clear();
}
