// Original dominator ripple lifetime, 0x0053CB10 / 0x0053CBE0 / 0x0053D310.
#include "yrpp/IonBlastClass.h"
#include "presentation/builders/ion_blast.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/DrawingBuffers.h"
#include "screen_effect.hpp"
#include <cstdlib>
namespace game {
void build_ion_blasts(PresentationContext&) {
    if (GameOptionsClass::Instance.DetailLevel != 2 || !TacticalClass::Instance) return;
    for (int i = IonBlastClass::Array.Count - 1; i >= 0; --i) {
        auto* blast = IonBlastClass::Array[i];
        Point2D center;
        if (TacticalClass::Instance->CoordsToClient(&blast->Location, &center))
            game::draw_ion_blast(center, blast->Lifetime,
                                 (ZBuffer::Instance ? ZBuffer::Instance->MaxValue : 0x8000) -
                                     TacticalClass::AdjustForZ(blast->Location.Z) - 3);
    }
}
}
