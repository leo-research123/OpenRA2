// YR RailgunAI 0x0062C3A0.
#include "yrpp/ParticleClass.h"
#include "yrpp/ScenarioClass.h"
#include "x87_integer.hpp"

void ParticleClass::RailgunAI() {
#if defined(RA2_YRPP_GAME)
    JMP_THIS(0x0062C3A0);
#else
    // Arithmetic follows the original's steady-state x87 control word, 0x0E7F.
    // Movement uses the old speed and float deltas before speed jitter.
    const Vector3D<float> delta{game::x87_double_to_float(game::x87_truncated_product(MovementDirection.X, Speed)),
                                game::x87_double_to_float(game::x87_truncated_product(MovementDirection.Y, Speed)),
                                game::x87_double_to_float(game::x87_truncated_product(MovementDirection.Z, Speed))};
    auto& random = ScenarioClass::Instance->Random;
    const double jitter =
        game::x87_truncated_product(double(random.RandomRanged(0, 0x7FFFFFFE)), 4.656612877414201e-10);
    Speed = game::x87_double_to_float(
        game::x87_truncated_sum(game::x87_truncated_product(game::x87_truncated_difference(jitter, 0.5), 0.1), Speed));
    PrecisePosition.X = game::x87_double_to_float(game::x87_truncated_sum(delta.X, PrecisePosition.X));
    PrecisePosition.Y = game::x87_double_to_float(game::x87_truncated_sum(delta.Y, PrecisePosition.Y));
    PrecisePosition.Z = game::x87_double_to_float(game::x87_truncated_sum(delta.Z, PrecisePosition.Z));
    SetLocation({game::x87_integer(PrecisePosition.X), game::x87_integer(PrecisePosition.Y),
                 game::x87_integer(PrecisePosition.Z)});

    const double color = game::x87_truncated_product(double(random.RandomRanged(0, 0x7FFFFFFE)), 4.656612877414201e-10);
    ColorAccum = game::x87_truncated_sum(
        game::x87_truncated_sum(game::x87_truncated_product(color, 0.05), Type->ColorSpeed), ColorAccum);
    if (ColorAccum > 1.0) {
        if (ColorIndex < Type->ColorList.Count - 2) {
            ++ColorIndex;
            ColorAccum = 0.0;
        } else {
            ColorAccum = 1.0;
        }
    }
#endif
}
