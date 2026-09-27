// YRpp 9402d7da; supplied 6E7E80/6E8750/6E87F0.
#include "yrpp/TaskForceClass.h"
#include "yrpp/CRC.h"
namespace { DynamicVectorClass<TaskForceClass*> types; }
DynamicVectorClass<TaskForceClass*>& TaskForceClass::Array = types;
TaskForceClass* YRPP_FASTCALL TaskForceClass::Find(const char* id) {
    const int index = FindIndex(id);
    return index < 0 ? nullptr : Array[index];
}
int YRPP_FASTCALL TaskForceClass::FindIndex(const char* id) {
    if (!id) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (_strcmpi(Array[i]->ID, id) == 0) return i;
    return -1;
}

TaskForceClass::TaskForceClass(const char* id) noexcept : AbstractTypeClass(id),
    Group(-1), CountEntries(0), IsGlobal(false), Entries{} { Array.AddItem(this); }
TaskForceClass::~TaskForceClass() { NotifyTypeExpired(); Array.Remove(this); }
void TaskForceClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(Group);
    crc(CountEntries);
}
