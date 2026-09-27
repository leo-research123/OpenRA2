// Original Building overrides 456580/4565E0/456640 and mask lookup 656580.
#include "yrpp/BuildingClass.h"
#include "yrpp/RadarClass.h"
namespace {
const DynamicVectorClass<Point2D>* foundation(const BuildingClass& building) noexcept {
    if (!building.Type || unsigned(building.Type->Foundation)>=22) return nullptr;
    return &RadarClass::Instance.FoundationTypePixels[unsigned(building.Type->Foundation)];
}
}
void BuildingClass::RadarTrackingStart() {
    if (const auto* pixels=foundation(*this)) for (auto offset : *pixels)
        RadarClass::Instance.TrackObject(this,RadarPosition.X+offset.X,RadarPosition.Y+offset.Y);
    IsRadarTracked=true;
}
void BuildingClass::RadarTrackingStop() {
    if (const auto* pixels=foundation(*this)) for (auto offset : *pixels)
        RadarClass::Instance.UntrackObject(this,RadarPosition.X+offset.X,RadarPosition.Y+offset.Y);
    IsRadarTracked=false;
}
void BuildingClass::RadarTrackingFlash() {
    if (const auto* pixels=foundation(*this)) for (auto offset : *pixels) {
        Point2D point{RadarPosition.X+offset.X,RadarPosition.Y+offset.Y};
        RadarClass::Instance.RefreshCrd(&point);
    }
}
