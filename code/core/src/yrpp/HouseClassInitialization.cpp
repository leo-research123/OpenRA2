// Original House initialization helper 4FCE00. Country is an unused argument.
#include "yrpp/HouseClass.h"
void HouseClass::InitializeForMultiplayer(int color, int, int credits) {
    StartingCredits = credits;
    Balance = credits;
    Type->ColorSchemeIndex = color;
    ColorSchemeIndex = color;
}
