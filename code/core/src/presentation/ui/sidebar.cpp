#include "presentation/ui/widgets.hpp"
// SidebarClass background/buttons: geometry and order from 0x6A6C30/0x6ABD30.
// Production cameos use the original CAMEO.PAL and GCLOCK2.SHP resources.
#include "yrpp/SidebarClass.h"
#include "bitfont_drawing.hpp"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cwchar>
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/StringTable.h"
#include "yrpp/SuperClass.h"

namespace {
// Strip::Draw 0x6A9540: text at cameo Y+1, a 2x1 margin, tooltip ink,
// and a black 0xAF background through 0x621B80 before the glyphs.
void production_text(const wchar_t* text, int anchor, int y, unsigned alignment) {
    auto* frame = game::game_ui_frame();
    auto* font = frame ? frame->resources.font() : nullptr;
    if (!font || !text) return;
    const int width = font->GetTextWidth(text);
    int x = anchor;
    if (alignment & 0x100)
        x -= width / 2;
    else if (alignment & 0x200)
        x -= width;
    game::RasterDrawingRequest background;
    background.target = frame->drawing.types.target;
    background.position = {x - 2, y - 1};
    background.clip = DSurface::WindowBounds;
    background.width = width + 4;
    background.height = font->field_1C + 2;
    // Original black blend retains (255 - 0xAF)/256 of each RGB565 channel.
    // The existing darkening operation retains (256 - strength)/256.
    background.blend_mode = game::RasterBlendMode::spotlight;
    background.spotlight_flags = 1;
    background.color = 0xB0;
    game::record_ui_drawing(game::submit_type_raster(frame->drawing.types, background));
    const WORD color = WORD(Drawing::TooltipColor.ToInt());
    for (const auto* c = text; *c; ++c) {
        int next = x;
        game::record_ui_drawing(
            game::submit_bitfont_glyph(*font, frame->drawing.types, *c, x, y, DSurface::WindowBounds, color, next));
        x = next;
    }
}
}

#include "presentation/ui/chrome.hpp"
#include "sidebar_layout.hpp"
namespace game {
void build_sidebar(GameUiFrame& context, const SidebarClass& self, unsigned force) {
    using namespace game;
    auto* frame = &context;
    const int x = DSurface::SidebarBounds.X;
    const auto& assets = frame->resources;
    draw_ui_shape(UiImage::side1, {x, DSurface::SidebarBounds.Y});
    int y = self.CameoPosition.Y;
    const int repeats = sidebar_usable_cameos() / 2, side_height = assets.image(UiImage::side2)->Height;
    draw_ui_shape_strip(UiImage::side2, {x, y}, {0, side_height}, repeats);
    y += repeats * side_height;
    draw_ui_shape(UiImage::side3, {x, y});
    y += assets.image(UiImage::side3)->Height;
    const int addon_height = assets.image(UiImage::addon)->Height;
    // Original addon plus repetitions to cover modern canvases beyond 60 slots.
    draw_ui_shape_strip(UiImage::addon, {x, y}, {0, addon_height},
                        std::max(0, (DSurface::WindowBounds.Height - y + addon_height - 1) / addon_height));
    build_shape_button(context, self.ToggleRepairButton, true);
    build_shape_button(context, self.ToggleSellButton, true);
    for (auto& button : self.TabButtons)
        build_shape_button(context, button, true);
    draw_ui_shape(UiImage::down, self.ScrollPosition, std::min(2, int(assets.image(UiImage::down)->Frames) - 1));
    draw_ui_shape(UiImage::up, {self.ScrollPosition.X + self.ScrollPitch.X, self.ScrollPosition.Y},
                  std::min(2, int(assets.image(UiImage::up)->Frames) - 1));
    const DrawingPaletteHandle* cameoPalette = nullptr;
    const auto prepared =
        frame->drawing.plain_palette(frame->drawing.types.backend_context, assets.cameo_palette(), cameoPalette);
    record_ui_drawing(prepared);
    if (prepared == DrawingStatus::drawn) {
        const auto* previous = frame->palette;
        frame->palette = cameoPalette;
        const auto& strip = self.Tabs[self.ActiveTabIndex];
        auto* owner = HouseClass::CurrentPlayer;
        for (int slot = 0; slot < sidebar_usable_cameos() && slot + 2 * strip.TopRowIndex < strip.CameoCount; ++slot) {
            const auto& cameo = strip.Cameos[slot + 2 * strip.TopRowIndex];
            auto* type = TechnoTypeClass::GetByTypeAndIndex(cameo.ItemType, cameo.ItemIndex);
            auto* super = cameo.ItemType == AbstractType::Special && owner &&
                                  unsigned(cameo.ItemIndex) < unsigned(owner->Supers.Count)
                              ? owner->Supers[cameo.ItemIndex]
                              : nullptr;
            if (!type && !super) continue;
            const Point2D at{self.CameoPosition.X + (slot % 2) * self.CameoPitch.X,
                             self.CameoPosition.Y + (slot / 2) * self.CameoPitch.Y + 1};
            draw_ui_shape(super ? super->Type->SidebarImage : type->GetCameo(), at);
            if (super) {
                production_text(super->NameReadiness(), at.X + 30, at.Y + 1, 0x100);
                if (super->ShouldDrawProgress()) {
                    ShapeDrawingRequest clock;
                    clock.target = frame->drawing.types.target;
                    clock.palette = previous;
                    clock.image = assets.image(UiImage::clock);
                    clock.frame = std::clamp(super->AnimStage(), 0, 54) + 1;
                    clock.position = at;
                    clock.clip = DSurface::WindowBounds;
                    clock.flags = 0x404;
                    record_ui_drawing(submit_type_shape(frame->drawing.types, clock));
                }
                continue;
            }
            // Original Strip::Draw flashes with DARKEN.SHP on frames 9..15.
            if (cameo.FlashEndFrame > Unsorted::CurrentFrame && Unsorted::CurrentFrame % 16 > 8) {
                ShapeDrawingRequest flash;
                flash.target = frame->drawing.types.target;
                flash.palette = previous;
                flash.image = assets.image(UiImage::darken);
                flash.position = at;
                flash.clip = DSurface::WindowBounds;
                flash.flags = 0x404;
                record_ui_drawing(submit_type_shape(frame->drawing.types, flash));
            }
            auto* factory = owner ? owner->GetPrimaryFactory(cameo.ItemType, type->Naval,
                                                             type->WhatAmI() == AbstractType::BuildingType
                                                                 ? static_cast<BuildingTypeClass*>(type)->BuildCat
                                                                 : BuildCat(0))
                                  : nullptr;
            const int queued = factory ? factory->CountTotal(type) : 0;
            const bool showCount =
                queued > 1 || (queued > 0 && factory && factory->Object && factory->Object->GetTechnoType() != type);
            if (showCount) {
                wchar_t number[16];
                std::swprintf(number, 16, L"%d", queued);
                production_text(number, at.X + 60, at.Y + 1, 0x200);
            }
            if (factory && factory->Object && factory->Object->GetTechnoType() == type) {
                const int progress = std::clamp(factory->GetProgress(), 0, 54);
                if (factory->IsDone()) {
                    production_text(StringTable::LoadString("TXT_READY"), at.X + 30, at.Y + 1, 0x100);
                } else {
                    ShapeDrawingRequest clock;
                    clock.target = frame->drawing.types.target;
                    clock.palette = previous;
                    clock.image = assets.image(UiImage::clock);
                    clock.frame = progress + 1;
                    clock.position = at;
                    clock.clip = DSurface::WindowBounds;
                    clock.flags = 0x4;
                    record_ui_drawing(submit_type_shape(frame->drawing.types, clock));
                    if (!factory->Production.Rate || factory->IsSuspended)
                        production_text(StringTable::LoadString("TXT_HOLD"), at.X + (showCount ? 2 : 30), at.Y + 1,
                                        showCount ? 0 : 0x100);
                }
            }
        }
        frame->palette = previous;
    }

}

}
