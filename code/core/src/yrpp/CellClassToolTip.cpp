// YR 0x00484FF0; retain the original Tiberium UIName lookup, including null.
#include "yrpp/CellClass.h"
#include "yrpp/TiberiumClass.h"
const wchar_t* CellClass::GetUIName() const {
    const auto* type=TiberiumClass::Find(OverlayTypeIndex);
    return type?type->UIName:nullptr;
}
