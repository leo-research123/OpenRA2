// TabClass 6D0A20 and original SHP-derived rectangles from 72FC60.
#include "yrpp/MouseClass.h"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cwchar>
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
void TabClass::Draw(DWORD force) {
    using namespace game;
    auto* frame=game_ui_frame();
    if (!frame) return;
    const auto& assets=frame->resources;
    if(HouseClass::CurrentPlayer)TabData.LastValue=static_cast<int>(HouseClass::CurrentPlayer->Available_Money());
    draw_ui_shape(UiImage::credits,{DSurface::SidebarBounds.X,0});
    // CreditsClass 4A2370: original LastValue, centered at sidebar width/2,
    // y=2, %ld. Identity is still terrain-browser startup until House exists.
    if (auto* font=assets.font()) {
        wchar_t text[32]; std::swprintf(text,32,L"%d",TabData.LastValue);
        int width=0,height=0; font->GetTextDimension(text,&width,&height,0);
        int position=DSurface::SidebarBounds.X+(168-width)/2;
        // 72A940/72A960/72A980 -> 72F440 original tooltip/credits RGB.
        const bool other=ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex;
        const WORD color=other ? 0xffe0 : WORD(((164>>3)<<11)|((210>>2)<<5)|(255>>3));
        for (const auto* character=text;*character;++character) {
            int next=position;
            record_ui_drawing(font->SubmitGlyph(frame->drawing.types,*character,position,2,
                DSurface::WindowBounds,color,next));
            position=next;
        }
    }
    const auto bar=GetCommandBarBounds();
    const int end_width=assets.image(UiImage::rightcap)->Width;
    const int start_width=assets.image(UiImage::leftcap)->Width;
    const int pitch=assets.image(UiImage::button_background)->Width;
    const int end_x=bar.Width-end_width;
    const int count=std::max(0,(bar.Width-start_width-end_width)/pitch);
    const int begin=end_x-count*pitch;
    const int spacer_width=assets.image(UiImage::spacer)->Width;
    for (int x=0;x<bar.Width;x+=spacer_width) draw_ui_shape(UiImage::spacer,{x,bar.Y});
    if (ThumbActive) {
        for (int i=0;i<count;++i) draw_ui_shape(UiImage::button_background,{begin+i*pitch,bar.Y});
        for (auto& button : CommandButtons) if (button.ShapeData) button.Draw(true);
        CollapseButton.Draw(true);
    } else ExpandButton.Draw(true);
    draw_ui_shape(UiImage::rightcap,{end_x,bar.Y});
    SidebarClass::Draw(force);
}
