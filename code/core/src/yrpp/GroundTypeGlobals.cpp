#include "yrpp/MapClass.h"

namespace {
// The EXE's zero-initialized ground table is populated by Rules readers.
GroundType ground[12]{};
}
GroundType (&GroundType::Array)[12] = ground;
