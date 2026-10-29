// YR 0x74CD60: the most recently registered monster at the exact cell.
#include "yrpp/VeinholeMonsterClass.h"
#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<VeinholeMonsterClass*> monsters;
}
DynamicVectorClass<VeinholeMonsterClass*>& VeinholeMonsterClass::Array = monsters;
VeinholeMonsterClass* YRPP_FASTCALL VeinholeMonsterClass::GetVeinholeMonsterFrom(CellStruct* cell) {
    for (int i = Array.Count - 1; i >= 0; --i)
        if (Array[i]->Position == *cell) return Array[i];
    return nullptr;
}
#endif
