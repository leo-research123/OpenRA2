// Original dominator ripple lifetime, 0x0053CB10 / 0x0053CBE0 / 0x0053D310.
#include "yrpp/IonBlastClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/DrawingBuffers.h"
#include <cstdlib>
namespace {
DynamicVectorClass<IonBlastClass*> blasts;
}
DynamicVectorClass<IonBlastClass*>& IonBlastClass::Array = blasts;
IonBlastClass::IonBlastClass(CoordStruct coords) : Location(coords), Lifetime(0), DisableIonBeam(false) {
    Array.AddItem(this);
}
IonBlastClass::~IonBlastClass() { Array.Remove(this); }
void IonBlastClass::Update() {
    if (Lifetime >= 79) {
        delete this;
        return;
    }
    // YR's dominator always disables the TS ion cannon beam and its deformation.
    // The unrelated TS beam path remains outside the native feature contract.
    if (DisableIonBeam != 1) std::abort();
    ++Lifetime;
}
void IonBlastClass::UpdateAll() {
    for (int i = Array.Count - 1; i >= 0; --i)
        Array[i]->Update();
}
