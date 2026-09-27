// SidebarClass background/buttons: geometry and order from 6A6C30/6ABD30.
// Production cameos use the original CAMEO.PAL and GCLOCK2.SHP resources.
#include "yrpp/SidebarClass.h"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cwchar>
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"

void SidebarClass::Draw(DWORD) {
    using namespace game;
    auto* frame=game_ui_frame();
    if (!frame) return;
    const int x=DSurface::SidebarBounds.X;
    const auto& assets=frame->resources;
    draw_ui_shape(UiImage::side1,{x,DSurface::SidebarBounds.Y});
    int y=CameoPosition.Y;
    for (int i=0;i<GetUsableCameoCount()/2;++i) {
        draw_ui_shape(UiImage::side2,{x,y}); y+=assets.image(UiImage::side2)->Height;
    }
    draw_ui_shape(UiImage::side3,{x,y}); y+=assets.image(UiImage::side3)->Height;
    const int addon_height=assets.image(UiImage::addon)->Height;
    // Original addon plus repetitions to cover modern canvases beyond 60 slots.
    for (;y<DSurface::WindowBounds.Height;y+=addon_height) draw_ui_shape(UiImage::addon,{x,y});
    UpdateCommandButtons();
    ToggleRepairButton.Draw(true);ToggleSellButton.Draw(true);
    for (auto& button : TabButtons) button.Draw(true);
    draw_ui_shape(UiImage::down,ScrollPosition,std::min(2,int(assets.image(UiImage::down)->Frames)-1));
    draw_ui_shape(UiImage::up,{ScrollPosition.X+ScrollPitch.X,ScrollPosition.Y},std::min(2,int(assets.image(UiImage::up)->Frames)-1));
    const DrawingPaletteHandle* cameoPalette=nullptr;
    const auto prepared=frame->drawing.plain_palette(frame->drawing.types.backend_context,assets.cameo_palette(),cameoPalette);
    record_ui_drawing(prepared);
    if(prepared==DrawingStatus::drawn){
        const auto* previous=frame->palette;frame->palette=cameoPalette;
        const auto& strip=Tabs[ActiveTabIndex];auto* owner=HouseClass::CurrentPlayer;
        for(int slot=0;slot<GetUsableCameoCount()&&slot+2*strip.TopRowIndex<strip.CameoCount;++slot){
            const auto& cameo=strip.Cameos[slot+2*strip.TopRowIndex];
            auto* type=TechnoTypeClass::GetByTypeAndIndex(cameo.ItemType,cameo.ItemIndex);if(!type)continue;
            const Point2D at{CameoPosition.X+(slot%2)*CameoPitch.X,CameoPosition.Y+(slot/2)*CameoPitch.Y+1};
            draw_ui_shape(type->GetCameo(),at);
            auto* factory=owner?owner->GetPrimaryFactory(cameo.ItemType,type->Naval,BuildCat(0)):nullptr;
            const int queued=factory?factory->CountTotal(type):0;
            if(factory&&factory->Object&&factory->Object->GetTechnoType()==type){
                const int progress=std::clamp(factory->GetProgress(),0,54);
                if(progress<54){
                    ShapeDrawingRequest clock;clock.target=frame->drawing.types.target;clock.palette=previous;
                    clock.image=assets.image(UiImage::clock);clock.frame=progress+1;clock.position=at;clock.clip=DSurface::WindowBounds;clock.flags=0x4;
                    record_ui_drawing(submit_type_shape(frame->drawing.types,clock));
                }
            }
            if(queued>1||(queued>0&&factory&&factory->Object&&factory->Object->GetTechnoType()!=type)){
                wchar_t number[16];std::swprintf(number,16,L"%d",queued);int x=at.X+2;
                for(const auto* c=number;*c;++c){int next=x;record_ui_drawing(assets.font()->SubmitGlyph(frame->drawing.types,*c,x,at.Y+2,DSurface::WindowBounds,0xffff,next));x=next;}
            }
        }
        frame->palette=previous;
    }
    Tabs[ActiveTabIndex].NeedsRedraw=false;
    SidebarNeedsRedraw=SidebarBackgroundNeedsRedraw=false;
}
