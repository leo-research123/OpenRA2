// Existing DisplayClass: layout/state portion of 4A8960 and 6D5F60.
// Device targets are recreated by the drawing backend, never by this class.
#include "yrpp/MouseClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/CCToolTip.h"
#include "yrpp/MessageListClass.h"
#if !defined(RA2_YRPP_GAME)
void DisplayClass::InitGUI() {
    // 0x004AE6B0 closes and clears the legacy HWND at 0x00B73554.
    // The standalone host has no legacy Win32 dialog to close.
}
void DisplayClass::Set_View_Dimensions(const RectangleStruct& rect) {
    if (rect.Width <= 0 || rect.Height <= 0) return;
    const auto& previous=DSurface::ViewBounds;
    const bool resized=previous.X!=rect.X || previous.Y!=rect.Y ||
        previous.Width!=rect.Width || previous.Height!=rect.Height;
    DSurface::ViewBounds = rect;
    TacticalClass::ViewBounds = rect;
    if (auto* tactical = TacticalClass::Instance) {
        auto center = tactical->TacticalCoord1;
        Point2D minimum, maximum;
        TacticalClass::CameraCenterBounds(MapRect.Width, VisibleRect, rect, minimum, maximum);
        if (minimum.X > maximum.X) minimum.X = maximum.X = (minimum.X + maximum.X) / 2;
        if (minimum.Y > maximum.Y) minimum.Y = maximum.Y = (minimum.Y + maximum.Y) / 2;
        TacticalClass::ClampCameraCenter(center, minimum, maximum);
        tactical->SetViewCenter(center, rect);
    }
    SidebarClass::Instance.UpdateLayout();
    RadarClass::Instance.InitializeLayout();
    // YR 0x004A8B18..0x004A8B7F registers the tactical tooltip region.
    if(auto* tips=CCToolTip::Instance){
        // The native bridge checks layout every frame. Only a real resize may
        // retire the region; replacing it each frame would dismiss an active tip.
        ToolTip region;
        const bool same=tips->Find(500,region) && region.Bounds.X==rect.X && region.Bounds.Y==rect.Y &&
            region.Bounds.Width==rect.Width && region.Bounds.Height==rect.Height;
        if(!same){region.GadgetID=500;region.Bounds=rect;region.Text=nullptr;region.field_18=false;
            tips->Remove(500);tips->Add(region);}
    }
    if(resized){
        // Message configuration tail of 0x004A8960. This entry is polled by the host; original
        // message reinitialization belongs to an actual view-size transition.
        auto& messages=MessageListClass::Instance;
        messages.Init(rect.X+3,rect.Y,6,98,14,-1,-1,0,20,98,rect.Width-6);
        messages.SetWidth(rect.Width);
    }
}

#endif
