#include "yrpp/Randomizer.h"
namespace {
DWORD default_seed = 0;
Randomizer global_random(0);
}
DWORD& Randomizer::DefaultSeed = default_seed;
Randomizer& Randomizer::Global = global_random;
