// Original 0x00471F90: discard all matching records without invoking FreeUnit.
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/TechnoClass.h"
bool CaptureManagerClass::UnlinkPointer(AbstractClass* object) {
    for (int i = ControlNodes.Count - 1; i >= 0; --i) {
        auto* node = ControlNodes[i];
        if (node->Unit == object) { GameDelete(node); ControlNodes.RemoveItem(i); }
    }
    return true;
}
