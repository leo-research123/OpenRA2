// YR 0x0054FE60..0x0055025E: original laser records and logic lifetime.
#include "yrpp/LaserDrawClass.h"
#include "yrpp/ArrayClasses.h"
#include <new>
#include <bit>

#if !defined(RA2_YRPP_GAME)

namespace {
DynamicVectorClass<LaserDrawClass*> lasers;
}
DynamicVectorClass<LaserDrawClass*>& LaserDrawClass::Array = lasers;

LaserDrawClass::LaserDrawClass(CoordStruct source, CoordStruct target, int zAdjust, BYTE unknown,
                               ColorStruct innerColor, ColorStruct outerColor, ColorStruct outerSpread, int duration,
                               bool blinks, bool fades, float startIntensity, float endIntensity)
    : Progress{}, Thickness(1), IsHouseColor(false), IsSupported(false), Source(source), Target(target),
      ZAdjust(zAdjust), field_40(static_cast<char>(unknown)), InnerColor(innerColor), OuterColor(outerColor),
      OuterSpread(outerSpread), Duration(duration), Blinks(blinks), BlinkState(false), Fades(fades),
      StartIntensity(startIntensity), EndIntensity(endIntensity) {
    Progress.Value = 0;
    Progress.Start(1, 1);
    try {
        Array.AddItem(this);
    } catch (const std::bad_alloc&) {
        // Original SetCapacity returns false; no exception may cross this boundary.
        Array.IsInitialized = true;
    }
}
LaserDrawClass::~LaserDrawClass() { Array.Remove(this); }
void LaserDrawClass::Update() {
    const auto& timer = Progress.Timer;
    const int elapsed = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(timer.StartTime));
    const int left = timer.StartTime == -1       ? timer.TimeLeft
                     : elapsed >= timer.TimeLeft ? 0
                                                 : std::bit_cast<int>(unsigned(timer.TimeLeft) - unsigned(elapsed));
    Progress.HasChanged = false;
    if (!left && Progress.Rate) {
        Progress.Value = std::bit_cast<int>(unsigned(Progress.Value) + unsigned(Progress.Step));
        Progress.HasChanged = true;
        Progress.Timer.Start(Progress.Rate);
    }
    if (Blinks) BlinkState = !BlinkState;
    if (Progress.Value >= Duration) GameDelete(this);
}
void YRPP_CDECL LaserDrawClass::UpdateAll() {
    for (int i = Array.Count - 1; i >= 0; --i)
        Array[i]->Update();
}

void YRPP_CDECL LaserDrawClass::DeleteAll() {
    while (Array.Count)
        GameDelete(Array[0]);
}

#endif
