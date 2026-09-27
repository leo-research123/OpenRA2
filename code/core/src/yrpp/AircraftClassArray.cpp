// Native storage for the existing original AircraftClass array at 0xA8E390.
// Ground-unit threat scans also consult this list for AA-capable weapons.
#include "yrpp/AircraftClass.h"
namespace {DynamicVectorClass<AircraftClass*> aircraft;}
DynamicVectorClass<AircraftClass*>& AircraftClass::Array=aircraft;
