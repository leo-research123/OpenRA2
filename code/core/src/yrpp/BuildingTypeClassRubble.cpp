// YR additions to the OpenTS BuildingTypeClass model, calibrated at
// 0x0045F160 / 0x0045F1D0. The dedicated rubble shadow writes its output
// arguments but returns false; that original behavior is intentional.
#include "yrpp/BuildingTypeClass.h"

bool BuildingTypeClass::GetRubbleShape(SHPStruct** image, int* frame) const {
    if (!LeaveRubble || !image) return false;
    if (Rubble) { *image = Rubble; *frame = 0; return true; }
    *image = GetImage();
    if (*image && (*image)->Frames / 2 > 3) { *frame = 3; return true; }
    return false;
}

bool BuildingTypeClass::GetRubbleShadowShape(SHPStruct** image, int* frame) const {
    if (!LeaveRubble || !image) return false;
    if (Rubble) { *image = Rubble; *frame = Rubble->Frames / 2; }
    else {
        *image = GetImage();
        if (*image && (*image)->Frames / 2 > 3) {
            *frame = (*image)->Frames / 2 + 3;
            return true;
        }
    }
    return false;
}
