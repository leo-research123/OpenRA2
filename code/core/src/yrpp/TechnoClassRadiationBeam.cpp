// Original beam creation at YR 0x6FD620; temporal shares the radiation geometry.
#include "yrpp/TechnoClass.h"
#include "yrpp/RadBeam.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/Randomizer.h"
#include <bit>
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
void TechnoClass::FireRadiationBeam(AbstractClass* target, bool temporal) try {
    auto* beam = RadBeam::Allocate(RadBeamType::RadBeam);
    CoordStruct source, render;
    source = *GetFLH(&source, 0, CoordStruct::Empty);
    render = *GetRenderCoords(&render);
    int depth = 0;
    if (source.Y != render.Y) {
        Point2D a, b;
        TacticalClass::Instance->CoordsToScreen(&a, &source);
        TacticalClass::Instance->CoordsToScreen(&b, &render);
        depth = a.Y - b.Y;
    }
    CoordStruct end;
    if ((target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None)
        end = *static_cast<ObjectClass*>(target)->GetTargetCoords(&end);
    else
        end = *target->GetCenterCoords(&end);
    beam->SetCoordsSource(source);
    beam->SetCoordsTarget(end);
    beam->Period = 15;
    beam->Amplitude = 40.0;
    beam->SetColor(temporal ? RulesClass::Instance->ChronoBeamColor : RulesClass::Instance->RadColor);
    beam->unknown_C = static_cast<DWORD>(depth);
} catch (...) {
    std::abort();
}

void TechnoClass::FireRadiationEruption(short spread) try {
    CoordStruct buffer;
    const auto source = *GetCoords(&buffer);
    const CellStruct base{short(source.X / 256), short(source.Y / 256)};
    const auto color = RulesClass::Instance->RadColor;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0) continue;
            const CellStruct cell{short(base.X + spread * x), short(base.Y + spread * y)};
            auto target = MapClass::Instance.GetCellAt(cell)->GetCellCoords();
            const int dx = Randomizer::Global.RandomRanged(-128, 128);
            const int dy = Randomizer::Global.RandomRanged(-128, 128);
            target.X = std::bit_cast<int>(unsigned(target.X) + unsigned(dx));
            target.Y = std::bit_cast<int>(unsigned(target.Y) + unsigned(dy));
            auto* beam = RadBeam::Allocate(RadBeamType::Eruption);
            beam->SetColor(color);
            const int period = Randomizer::Global.RandomRanged(5, 20);
            const int amplitude = Randomizer::Global.RandomRanged(100, 500);
            const int extension = Randomizer::Global.RandomRanged(0, 20);
            beam->SetCoordsSource(source);
            beam->SetCoordsTarget(target);
            beam->Period = unsigned(period);
            beam->Amplitude = double(amplitude);
            beam->unknown_48 = 0.0;
            beam->unknown_50 = unsigned(extension);
        }
} catch (...) {
    std::abort();
}
#endif
