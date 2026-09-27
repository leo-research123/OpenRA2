// YRpp 9402d7da; only exact trivial bodies from the supplied function exports.
#include "yrpp/ObjectTypeClass.h"

// 0x00428E40: confirmed constant return, not an unimplemented stub.
DWORD ObjectTypeClass::GetOwners() const { return -1; }

// 0x005F75B0: confirmed constant return, not an unimplemented stub.
int ObjectTypeClass::GetPipMax() const { return 0; }

// 0x005F7610: confirmed constant return, not an unimplemented stub.
int ObjectTypeClass::GetActualCost(HouseClass* pHouse) const { return 0; }

// 0x005F7620: confirmed constant return, not an unimplemented stub.
int ObjectTypeClass::GetBuildSpeed() const { return 0; }

// 0x005F7630: confirmed constant return, not an unimplemented stub.
SHPStruct* ObjectTypeClass::GetCameo() const { return nullptr; }
