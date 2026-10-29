#pragma once
#include "type_drawing_context.hpp"
namespace game {
// Device composition only. Original ChronoScreenEffect owns all gameplay
// phases, durations, pause nesting, input ownership and sound controllers.
enum class ChronoDeviceOperation : int { capture_old, warp_old, white, capture_new, warp_new, restore, discard };
void chrono_device(ChronoDeviceOperation, int step = 0, int duration = 0) noexcept;
void chrono_audio_begin() noexcept;
void chrono_audio_reserve(bool reserve) noexcept;
// Submits the current stage at its position in GScreenClass::Render.
void draw_chrono_screen() noexcept;
void draw_ion_blast(Point2D center, int lifetime, int depthBase) noexcept;
// 0x53B560 renders the new world with Status=0 into the snapshot without
// presenting it; GScreenClass::Render performs that pass between these marks.
bool chrono_capture_pending() noexcept;
void submit_chrono_capture(bool begin) noexcept;
void reset_chrono_screen_effect() noexcept;
}
