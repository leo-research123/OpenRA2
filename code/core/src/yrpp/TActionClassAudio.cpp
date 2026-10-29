// TActionClass 0x6E1BB0 and Execute's inline case at 0x6DE904.
#include "yrpp/TActionClass.h"
#include "yrpp/VoxClass.h"

#if !defined(RA2_YRPP_GAME)
bool TActionClass::PlayEVA(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    VoxClass::PlayIndex(Value, static_cast<int>(VoxType::Interrupt), -1);
    return true;
}
#endif
