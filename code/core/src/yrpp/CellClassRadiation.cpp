// Original YR cell radiation operations, 0x00487C90..0x00487D41.
#include "yrpp/CellClass.h"

bool CellClass::IsRadiated() const { return RadLevel > 0.0; }
void CellClass::RadLevel_Increase(double amount) { RadLevel += amount; }
void CellClass::RadLevel_Decrease(double amount) {
    RadLevel -= amount;
    if (RadLevel < 0.0) RadLevel = 0.0;
}
