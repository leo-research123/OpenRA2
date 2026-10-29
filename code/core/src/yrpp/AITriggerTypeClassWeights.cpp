// Team outcome weighting, YR 0x41FD60/0x41FE20.
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/RulesClass.h"
#include <bit>

void AITriggerTypeClass::RegisterSuccess() {
    double adjustment = 0.0;
    if (TimesCompleted > 0) {
        adjustment = double(TimesCompleted) * (double(TimesExecuted) / double(TimesCompleted) - 0.5);
        if (adjustment < 0.0) adjustment = 0.0;
    }
    Weight_Current = adjustment + RulesClass::Instance->AITriggerSuccessWeightDelta + Weight_Current;
    if (Weight_Current < Weight_Minimum) Weight_Current = Weight_Minimum;
    if (Weight_Current > Weight_Maximum) Weight_Current = Weight_Maximum;
    TimesExecuted = std::bit_cast<int>(unsigned(TimesExecuted) + 1u);
    TimesCompleted = std::bit_cast<int>(unsigned(TimesCompleted) + 1u);
}
void AITriggerTypeClass::RegisterFailure() {
    double adjustment = 0.0;
    if (TimesCompleted > 0) {
        adjustment = double(TimesCompleted) * ((double(TimesExecuted) / double(TimesCompleted) - 0.5) *
                                               RulesClass::Instance->AITriggerTrackRecordCoefficient);
        if (adjustment > 0.0) adjustment = 0.0;
    }
    Weight_Current = adjustment + RulesClass::Instance->AITriggerFailureWeightDelta + Weight_Current;
    if (Weight_Current < Weight_Minimum) Weight_Current = Weight_Minimum;
    if (Weight_Current > Weight_Maximum) Weight_Current = Weight_Maximum;
    TimesCompleted = std::bit_cast<int>(unsigned(TimesCompleted) + 1u);
}
