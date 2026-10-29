#include "ebolt_lifetime.hpp"
#include "yrpp/EBolt.h"
#include <bit>
namespace game {
void ebolt_follow_owners() {
    for (int i = EBolt::Array.Count - 1; i >= 0; --i) {
        auto* bolt = EBolt::Array[i];
        if (bolt->Owner) {
            const auto source = bolt->GetSourceCoords();
            if (source != CoordStruct(-1, -1, -1)) bolt->Point1 = source;
        }
    }
}
void ebolt_advance() {
    for (int i = EBolt::Array.Count - 1; i >= 0; --i) {
        auto* bolt = EBolt::Array[i];
        if (bolt->Lifetime) bolt->Random = std::bit_cast<int>(unsigned(bolt->Random) + 1u);
        bolt->Lifetime >>= 1;
        if (!bolt->Lifetime) {
            EBolt::Array.Remove(bolt);
            GameDelete(bolt);
        }
    }
}
}
