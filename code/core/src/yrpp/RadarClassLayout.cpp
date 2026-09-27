// Existing RadarClass; geometry-only portions of 652CF0/652E90/654320.
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Drawing.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/CCToolTip.h"
#include "yrpp/GameOptionsClass.h"
#include "game_ui_runtime.hpp"
#include "scenario_runtime.hpp"
void RadarClass::InitializeLayout() noexcept {
    const int dx=16-int(unknown_11F0),dy=49-int(unknown_11F4);
    if (unknown_rect_149C.Width>0 && unknown_rect_149C.Height>0) {
        unknown_rect_149C.X+=dx; unknown_rect_149C.Y+=dy;
        unknown_rect_14DC.X+=dx; unknown_rect_14DC.Y+=dy;
    }
    unknown_11E8 = 16; unknown_11EC = 48;
    unknown_11F0 = 16; unknown_11F4 = 49;
    unknown_11F8 = unknown_1200 = 140;
    unknown_11FC = unknown_1204 = 108;
    const auto panel=GetPanelBounds();
    RadarButton.SetPosition(panel.X,panel.Y);
    RadarButton.SetDimension(panel.Width,panel.Height);
    GScreenClass::Instance.AddButton(&RadarButton);
    unknown_bool_14DA = true;
    if (DiplomacyShape && OptionsShape) InitGUI();
}
void RadarClass::DisposeOfArt() {
    if (OwnsDiplomacyShape) {YRMemory::Deallocate(DiplomacyShape);OwnsDiplomacyShape=false;}
    DiplomacyShape=nullptr;
    if (OwnsOptionsShape) {YRMemory::Deallocate(OptionsShape);OwnsOptionsShape=false;}
    OptionsShape=nullptr;
}
void RadarClass::Init_For_House() {
    DisposeOfArt();
    const bool other=ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex;
    unknown_11F0=16;
    DiplomacyBounds.X=int(padding_11E4)+(other ? 14 : 11);
    OptionsBounds.X=int(padding_11E4)+(other ? 86 : 83);
    DiplomacyBounds.Y=OptionsBounds.Y=int(unknown_11E8)+(other ? 5 : 4);
    RadarButton.X=int(unknown_11F0);
    try {
        if (auto* frame=game::game_ui_frame()) {
            // UiResources owns these references; original owned flags stay false.
            DiplomacyShape=frame->resources.image(game::UiImage::briefing);
            OptionsShape=frame->resources.image(game::UiImage::options);
            RadarAnim=frame->resources.image(game::UiImage::radar);
        } else {
            DiplomacyShape=static_cast<SHPStruct*>(FileSystem::LoadWholeFileEx("DIPLOBTN.SHP",OwnsDiplomacyShape));
            OptionsShape=static_cast<SHPStruct*>(FileSystem::LoadWholeFileEx("OPTBTN.SHP",OwnsOptionsShape));
            RadarAnim=FileSystem::LoadSHPFile(other && ScenarioClass::Instance->PlayerSideIndex==2 ? "RADARY.SHP" : "RADAR.SHP");
        }
    } catch (...) {DisposeOfArt();RadarAnim=nullptr;}
    DiplomacyBounds.Width=DiplomacyShape ? DiplomacyShape->Width : 0;
    DiplomacyBounds.Height=DiplomacyShape ? DiplomacyShape->Height : 0;
    OptionsBounds.Width=OptionsShape ? OptionsShape->Width : 0;
    OptionsBounds.Height=OptionsShape ? OptionsShape->Height : 0;
    const auto color=Drawing::TooltipColor;
    unknown_1208=(unsigned(color.Red)>>Drawing::RedShiftRight)<<Drawing::RedShiftLeft |
        (unsigned(color.Green)>>Drawing::GreenShiftRight)<<Drawing::GreenShiftLeft |
        (unsigned(color.Blue)>>Drawing::BlueShiftRight)<<Drawing::BlueShiftLeft;
}
void RadarClass::Init_IO() {
    DisplayClass::Init_IO();
    if (Unsorted::ArmageddonMode) return;
    ShapeButtonClass* buttons[]{&DiplomacyButton,&OptionsButton};
    const RectangleStruct* bounds[]{&DiplomacyBounds,&OptionsBounds};
    SHPStruct* shapes[]{DiplomacyShape,OptionsShape};
    for(int i=0;i<2;++i) {
        auto& button=*buttons[i];
        button.ID=242+i;button.UseSidebarSurface=true;button.IsOn=false;
        button.ToggleType=0;button.Flags=static_cast<GadgetFlag>(0x5);
#if defined(RA2_YRPP_GAME)
        button.Drawer=FileSystem::SIDEBAR_PAL;
#else
        button.Drawer=nullptr; // Borrowed generic frame carries the converter.
#endif
        button.SetPosition(bounds[i]->X,bounds[i]->Y);
        button.SetShape(shapes[i],0,0);button.MarkRedraw();
    }
}
void RadarClass::SetRadarButtonsVisible(bool visible) {
    if (visible) {AddButton(&DiplomacyButton);AddButton(&OptionsButton);}
    else {RemoveButton(&DiplomacyButton);RemoveButton(&OptionsButton);}
}
void RadarClass::InitializeButtons() noexcept {
    auto* frame=game::game_ui_frame();
    if (!frame) return;
    if (DiplomacyShape!=frame->resources.image(game::UiImage::briefing) ||
        OptionsShape!=frame->resources.image(game::UiImage::options) ||
        RadarAnim!=frame->resources.image(game::UiImage::radar)) {
        Init_For_House();Init_IO();InitGUI();
    }
    const int x=GameOptionsClass::Instance.SidebarMode ? TacticalClass::ViewBounds.Width : 0;
    ToolTip tip;
    if (DiplomacyButton.X!=x+DiplomacyBounds.X || OptionsButton.X!=x+OptionsBounds.X ||
        (CCToolTip::Instance && (!CCToolTip::Instance->Find(242,tip) || !CCToolTip::Instance->Find(243,tip))))
        InitGUI();
    SetRadarButtonsVisible(SidebarClass::Instance.IsSidebarActive && !Unsorted::ArmageddonMode);
}
void RadarClass::InitGUI() {
    DisplayClass::InitGUI();
    const int x=GameOptionsClass::Instance.SidebarMode ? TacticalClass::ViewBounds.Width : 0;
    DiplomacyButton.DrawPosition.X=OptionsButton.DrawPosition.X=-x;
    DiplomacyButton.SetPosition(x+DiplomacyBounds.X,DiplomacyBounds.Y);
    OptionsButton.SetPosition(x+OptionsBounds.X,OptionsBounds.Y);
    RadarButton.SetPosition(x+int(unknown_11F0),int(unknown_11F4));
    DiplomacyButton.MarkRedraw();OptionsButton.MarkRedraw();RadarButton.MarkRedraw();
    if(auto* tips=CCToolTip::Instance){
#if defined(RA2_YRPP_GAME)
        const int mode=int(SessionClass::Instance.GameMode);
#else
        const auto& runtime=game::scenario_runtime();
        const int mode=runtime.session_mode ? runtime.session_mode(runtime.context) : -1;
#endif
        ToolTip tip;
        tip.GadgetID=242;tip.Text=mode==0 ? "Tip:BriefingButton" : "Tip:DiplomacyButton";
        tip.Bounds={DiplomacyButton.X,DiplomacyButton.Y,DiplomacyButton.Width,DiplomacyButton.Height};
        tips->Remove(242);tips->Add(tip);
        tip.GadgetID=243;tip.Text="Tip:OptionsButton";
        tip.Bounds={OptionsButton.X,OptionsButton.Y,OptionsButton.Width,OptionsButton.Height};
        tips->Remove(243);tips->Add(tip);
    }
    unknown_bool_14DA=true;
}
const wchar_t* RadarClass::GetToolTip(UINT id) {return DisplayClass::GetToolTip(id);}
RectangleStruct RadarClass::GetPanelBounds() const noexcept {
    // SidebarBounds starts at the production strip (Y=158); the original
    // radar surface uses window Y and only takes the sidebar's horizontal offset.
    return {DSurface::SidebarBounds.X + int(unknown_11F0), int(unknown_11F4),
        int(unknown_1200), int(unknown_1204)};
}
