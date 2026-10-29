// UnitTrackerClass counters owned by HouseClass statistics.
#include "yrpp/HouseClass.h"
#if !defined(RA2_YRPP_GAME)
void UnitTrackerClass::PopulateUnitCount(int nCount) noexcept {
    // 0x749060 clears the full table before publishing the populated range.
    UnitCount = 0x200;
    for (int i = 0; i < UnitCount; ++i)
        UnitTotals[i] = 0;
    UnitCount = nCount;
}
#endif
