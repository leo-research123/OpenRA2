// Tactical sound projection and positioned controller list, 0x007509E0..0x00750F35.
#include "yrpp/VocClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CellClass.h"
#include <algorithm>
#include <array>
#include <cmath>

#if !defined(RA2_YRPP_GAME)
namespace {
struct Positioned {
    bool used = false;
    CoordStruct at{};
    int argument = 0;
    AudioController controller;
};
std::array<Positioned, 200> positioned;
float spatial(const VocClass& type, const CoordStruct& at, unsigned& pan) {
    pan = 0;
    if (!TacticalClass::Instance || TacticalClass::ViewBounds.Width <= 0 || TacticalClass::ViewBounds.Height <= 0)
        return 0;
    const unsigned flags = unsigned(type.Type);
    if (flags & 0x800) {
        if (at.X / 256 == 0 && at.Y / 256 == 0) return 0;
        const auto* cell = MapClass::Instance.TryGetCellAt(at);
        if (!cell || !(unsigned(cell->AltFlags) & 0x18)) return 0;
    }
    Point2D point{};
    TacticalClass::Instance->CoordsToClient(&at, &point);
    const float half_x = TacticalClass::ViewBounds.Width * 0.5f;
    const float half_y = TacticalClass::ViewBounds.Height * 0.5f;
    const float offset = point.X - half_x;
    float x = float(std::abs(int(offset))), y = float(std::abs(int(point.Y - half_y)));
    if (!(flags & 0x40)) {
        x = std::max(0.0f, x - half_x);
        y = std::max(0.0f, y - half_y);
    }
    const float range = float(type.Range) * 60;
    float volume = range > 0 && x < range && y * 2 < range ? (range - std::max(x, y * 2)) / range : 0;
    if (flags & 0x10) volume = std::max(volume, type.MinVolume);
    if (volume < 0.05f) return 0;
    const float width = half_x * 2;
    pan = unsigned(std::clamp(offset, -width, width) * 8192.0f / width + 8192.0f);
    return volume;
}
}
void YRPP_FASTCALL VocClass::PlayAt(int index, const CoordStruct& at, AudioController* controller) {
    if (!VoicesEnabled) return;
    auto* type = Get(index);
    unsigned pan = 0;
    const float volume = type ? spatial(*type, at, pan) : 0;
    if (volume <= 0) {
        if (controller) {
            controller->Stop();
            controller->SetEvent(nullptr, type);
        }
        return;
    }
    PlayGlobal(index, int(pan), volume, controller);
}
void* YRPP_FASTCALL VocClass::UpdatePosition(const CoordStruct* at, AudioController* controller) {
    if (!at || !controller) return nullptr;
    auto* type = controller->GetEventType();
    auto* event = controller->GetEvent();
    if (type) {
        unsigned pan = 0;
        const float volume = spatial(*type, *at, pan);
        if (volume <= 0) {
            controller->Stop();
            controller->SetEvent(nullptr, type);
            return nullptr;
        }
        if (!event && (unsigned(type->Control) & 1) && !type->Loop) {
            PlayAt(FindIndex(type->Name), *at, controller);
            event = controller->GetEvent();
        }
        if (event) {
            AudioController::AdjustAudioEventVolume(event, unsigned(volume * 0x4000));
            AudioController::AdjustAudioEventPanning(event, pan);
        }
    }
    return event;
}
void YRPP_FASTCALL VocClass::PlayIndexAtPos(int index, const CoordStruct& at, int argument) {
    if (index < 0) return;
    for (auto& entry : positioned)
        if (!entry.used) {
            entry.used = true;
            entry.at = at;
            entry.argument = argument;
            PlayAt(index, at, &entry.controller);
            return;
        }
}
void VocClass::UpdatePositionedSounds() noexcept {
    for (auto& entry : positioned)
        if (entry.used) {
            UpdatePosition(&entry.at, &entry.controller);
            if (!entry.controller.GetEvent()) {
                const auto* type = entry.controller.GetEventType();
                if (!type || !(unsigned(type->Control) & 1) || type->Loop) {
                    entry.controller.Stop();
                    entry.used = false;
                }
            }
        }
}
void VocClass::ClearPositionedSounds() noexcept {
    for (auto& entry : positioned)
        if (entry.used) {
            entry.controller.Stop();
            entry.used = false;
        }
}
void YRPP_FASTCALL VocClass::StopIndexAtPos(const CoordStruct& at, int mask) {
    for (auto& entry : positioned)
        if (entry.used && (entry.argument & mask) && entry.at == at) entry.controller.Stop();
}
#endif
