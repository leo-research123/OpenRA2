#pragma once

#include "yrpp/Helpers/CompileTime.h"

class GetCDClass
{
public:
    /// Global VA: 0x00A8E8E8.
    DEFINE_REFERENCE(GetCDClass, Instance, 0xA8E8E8u)

    int Drives[26];
    int Count;
    int unknown_6C;
};
