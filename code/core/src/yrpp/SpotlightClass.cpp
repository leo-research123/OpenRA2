// Original spotlight lifetime and registry.
#include "yrpp/SpotlightClass.h"
namespace { DynamicVectorClass<SpotlightClass*> lights; }
DynamicVectorClass<SpotlightClass*>& SpotlightClass::Array = lights;
SpotlightClass::SpotlightClass(CoordStruct coords, int size)
    : Coords(coords), MovementRadius(0), Size(size), DisableFlags(SpotlightFlags::None) {
    Array.AddItem(this);
}
SpotlightClass::~SpotlightClass() { Array.Remove(this); }
void SpotlightClass::Update() {
    MovementRadius += 8;
    if (MovementRadius >= 80) delete this;
}
