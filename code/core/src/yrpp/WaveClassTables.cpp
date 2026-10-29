// WaveClass shared lookup tables, YR 0x0075F020.
#include "yrpp/WaveClass.h"
#include "yrpp/YRMath.h"
#include "x87_double.hpp"
#include "x87_integer.hpp"

namespace {
unsigned short distance_table[90000];
short sonic_amplitudes[500];
short magnetron_amplitudes[496];
int strengths[13];
bool tables_initialized = false;
}

unsigned short (&WaveClass::DistanceTable)[90000] = distance_table;
short (&WaveClass::SonicAmplitudeTable)[500] = sonic_amplitudes;
short (&WaveClass::MagnetronAmplitudeTable)[496] = magnetron_amplitudes;
int (&WaveClass::StrengthTable)[13] = strengths;
bool& WaveClass::TablesInitialized = tables_initialized;

void WaveClass::InitializeTables() {
    TablesInitialized = true;
    // The first conversion through 0x007C5F00 leaves the 0x0E7F control word active, so the amplitude
    // arithmetic below rounds toward zero. The 16-bit stores keep the low word of each conversion.
    for (int y = 0; y < 300; ++y)
        for (int x = 0; x < 300; ++x)
            DistanceTable[y * 300 + x] =
                static_cast<unsigned short>(game::x87_integer(Math::sqrt(static_cast<double>(y * y + x * x))));
    const auto amplitude = [](int index, double step) {
        const double sine = Math::sin(static_cast<double>(index) * step);
        return static_cast<short>(game::x87_integer(game::x87_add(game::x87_multiply(sine, 12.0), 0.49)));
    };
    for (int i = 0; i < 500; ++i)
        SonicAmplitudeTable[i] = amplitude(i, 0.125);
    for (int i = 0; i < 496; ++i)
        MagnetronAmplitudeTable[i] = amplitude(i, 0.1);
    for (int i = 0; i < 13; ++i)
        StrengthTable[i] = 110 + 8 * i;
}
