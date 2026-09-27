#pragma once
#include "yrpp/ArrayClasses.h"
#include "filesystem/bootstrap_stage.hpp"
#include "yrpp/MixFileClass.h"

namespace game {
// Storage is selected by the target: standalone state or original-game bindings.
extern int& disk_selection;
MixFileClass*& generic_mix(GenericMixSlot slot);
// Host deletion removes references, never deletes through the non-owning list.
void forget_mix(MixFileClass* mix) noexcept;
}
