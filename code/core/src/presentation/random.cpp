#include "presentation/random.hpp"
#include "yrpp/Randomizer.h"
namespace game {
Randomizer& presentation_random() noexcept {
    // Never destroyed: the stream may be used while the process shuts down.
    static auto* stream = new Randomizer(0x5052454Eu);
    return *stream;
}
}
