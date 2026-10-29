// Original NodeNameType operations, YR 696F90 / 696F50.
#include "yrpp/SessionClass.h"
#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<NodeNameType*> players;
}
DynamicVectorClass<NodeNameType*>& NodeNameType::Array = players;
#endif
void NodeNameType::SetCountry(int country) {
    Country = country == -2 ? 0 : country;
    InitialCountry = country == -2 ? -2 : -1;
}
int NodeNameType::GetStartPoint() const { return InitialStartPoint == -2 && StartPoint == -1 ? -2 : StartPoint; }
