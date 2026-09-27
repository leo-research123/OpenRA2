// YRpp 9402d7da; supplied 6A4550/6A4710/6A4930.
#include "yrpp/SideClass.h"
#include "yrpp/CRC.h"
namespace { DynamicVectorClass<SideClass*> types; }
DynamicVectorClass<SideClass*>& SideClass::Array = types;
SideClass* YRPP_FASTCALL SideClass::Find(const char* id) {
    const int index = FindIndex(id);
    return index < 0 ? nullptr : Array[index];
}
int YRPP_FASTCALL SideClass::FindIndex(const char* id) {
    if (!id) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (_strcmpi(Array[i]->ID, id) == 0) return i;
    return -1;
}

SideClass::SideClass(const char* id) noexcept : AbstractTypeClass(id), HouseTypes() {
    Create_ID();
    Array.AddItem(this);
}
SideClass::~SideClass() { NotifyTypeExpired(); Array.Remove(this); }
void SideClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(HouseTypes.Count);
}
