#include "yrpp/AnimTypeClass.h"

namespace {
DynamicVectorClass<AnimTypeClass*> animations;
}
DynamicVectorClass<AnimTypeClass*>& AnimTypeClass::Array = animations;
