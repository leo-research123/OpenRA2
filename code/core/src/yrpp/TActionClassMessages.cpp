// TActionClass 0x006E0D60 and Execute's inline case at 0x006DE0A8.
#include "yrpp/TActionClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/ColorScheme.h"
#include "x87_integer.hpp"

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

#if !defined(RA2_YRPP_GAME)
bool TActionClass::PrintMessage(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    const int side = ScenarioClass::Instance->PlayerSideIndex;
    // 0x0069A310 maps a player color slot through the original nine-byte LUT.
    const int color = ColorScheme::PlayerColorToColorSchemeLUT[side == 0 ? 2 : side == 1 ? 1 : 5];
    if (Text[0]) {
        // Multiply before _ftol changes the rounding mode; consume its low
        // 32 bits, including wrapped and invalid conversions, not a frame timer.
        const int duration = game::x87_integer(RulesClass::Instance->MessageDelay * 900.0);
        const auto* text = StringTable::LoadString(Text, nullptr, "D:\\ra2mdpost\\TAction.CPP", 1766);
        MessageListClass::Instance.AddMessage(nullptr, 0, text, color, TextPrintType(0x4046), duration, false);
    }
    // Empty keys and a rejected insertion are successful original actions.
    return true;
}
#endif
