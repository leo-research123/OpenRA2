// Original NodeNameType operations, YR 696F90 / 696F50.
#include "yrpp/SessionClass.h"
void NodeNameType::SetCountry(int country) {
    Country = country == -2 ? 0 : country;
    InitialCountry = country == -2 ? -2 : -1;
}
int NodeNameType::GetStartPoint() const {
    return InitialStartPoint == -2 && StartPoint == -1 ? -2 : StartPoint;
}
