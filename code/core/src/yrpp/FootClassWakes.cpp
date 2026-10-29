// Original FootClass wake effect, 0x629E90.
#include "yrpp/FootClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include "Matrix3DArithmetic.hpp"
#if !defined(RA2_YRPP_GAME)
void FootClass::CreateWakes(CoordStruct at) {
    const double angle = -matrix_multiply(int(short(PrimaryFacing.Current().Raw)) - 0x3FFF, -0.00009587672516830327);
    const double cosine = Math::cos(angle), sine = Math::sin(angle);
    auto& random = ScenarioClass::Instance->Random;
    for (int i = 0; i < 3; ++i) {
        const double along = random.RandomRanged(-180, 180);
        const double across = random.RandomRanged(-64, 64);
        const int x = rule_integer(matrix_add(matrix_multiply(along, cosine), -matrix_multiply(across, sine)));
        const int y = rule_integer(matrix_add(matrix_multiply(along, sine), matrix_multiply(across, cosine)));
        const CoordStruct where{std::bit_cast<int>(unsigned(at.X) + unsigned(x)),
                                std::bit_cast<int>(unsigned(at.Y) + unsigned(y)), at.Z};
        if (void* memory = YRMemory::Allocate(sizeof(AnimClass)))
            ::new (memory) AnimClass(RulesClass::Instance->Wake, where, 0, 1, 0x600, 0, false);
    }
}
#endif
