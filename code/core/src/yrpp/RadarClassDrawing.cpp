// RadarClass chrome and terrain/viewport drawing, calibrated to 656EC0.
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "game_ui_runtime.hpp"
#include "yrpp/OwnerDraw.h"
#include <algorithm>
#include <array>
#include <cstring>

#include <cstdlib>


bool RadarClass::UpdateRadarImage() noexcept {
    const auto& rect = unknown_rect_149C;
    auto* background = unknown_1220;
    if (!background || rect.Width < 1 || rect.Height < 1 || background->GetWidth() != rect.Width ||
        background->GetHeight() != rect.Height || background->GetBytesPerPixel() != 2)
        return false;
    auto*& foreground = unknown_121C;
    if (!foreground || foreground->GetWidth() != rect.Width || foreground->GetHeight() != rect.Height ||
        foreground->GetBytesPerPixel() != 2) {
        GameDelete(foreground);
        foreground = nullptr;
        try {
            foreground = BSurface::Create(rect.Width, rect.Height, 2);
        } catch (...) {
            return false;
        }
        if (!foreground) return false;
        unknown_1248 = unknown_124C = 0;
        unknown_1250 = DWORD(rect.Width);
        unknown_1254 = DWORD(rect.Height);
    }
    const int target_pitch = foreground->GetPitch() / 2, source_pitch = background->GetPitch() / 2;
    if (target_pitch < rect.Width || source_pitch < rect.Width) return false;
    auto* target = static_cast<WORD*>(foreground->Lock(0, 0));
    if (!target) {
        foreground->Unlock();
        return false;
    }
    const auto* source = static_cast<const WORD*>(background->Lock(0, 0));
    if (!source) {
        background->Unlock();
        foreground->Unlock();
        return false;
    }
    // 0x00655C50 restarts each pixel from the background, then applies the
    // gap/shroud and tracked-object branches.
    const auto plot = [&](const RectangleStruct& area) {
        for (int y = area.Y; y < area.Y + area.Height; ++y)
            std::memcpy(target + y * target_pitch + area.X, source + y * source_pitch + area.X,
                        std::size_t(area.Width) * 2);
        return ApplyTerrainVisibility(target, target_pitch, area) && ApplyTrackedObjects(target, target_pitch, area);
    };
    const auto pending = Drawing::Intersect(
        {int(unknown_1248), int(unknown_124C), int(unknown_1250), int(unknown_1254)}, {0, 0, rect.Width, rect.Height});
    bool result = pending.Width <= 0 || pending.Height <= 0 || plot(pending);
    if (result) unknown_1248 = unknown_124C = unknown_1250 = unknown_1254 = 0;
    // Points retained across a rebuild may fall outside it; 0x00655C50
    // ignores them. The caller clears the queue after presentation.
    for (int i = unknown_points_125C.Count - 1; result && i >= 0; --i) {
        const auto point = unknown_points_125C[i];
        if (point.X >= 0 && point.Y >= 0 && point.X < rect.Width && point.Y < rect.Height)
            result = plot({point.X, point.Y, 1, 1});
    }
    background->Unlock();
    foreground->Unlock();
    return result;
}

void RadarClass::Draw(DWORD) { std::abort(); }
