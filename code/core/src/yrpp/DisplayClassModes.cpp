// YR-only beacon mode extends the original OpenTS display command-mode design.
// Fixed original 0x004AC960: toggle starts from RepairMode, not PlaceBeaconMode.
#include "yrpp/DisplayClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/HouseClass.h"
#if !defined(RA2_YRPP_GAME)
// OpenTS 44fac744 display.cpp Sell_Mode_Control / Repair_Mode_Control;
// calibrated to YR 0x4AC660 / 0x4AC8C0. GPL-3.0-or-later, EA Section 7
// terms in code/third_party/opents/LICENSE.md.
void DisplayClass::SetSellMode(int mode) noexcept {
    const bool active=mode==-1 ? !SellMode : mode==0 ? false : mode==1 ? true : SellMode;
    if(active==SellMode || CurrentBuildingType)return;
    SetCursor(MouseCursorType::Default,false);
    RepairMode=PowerToggleMode=PlanningMode=PlaceBeaconMode=false;
    SellMode=active && HouseClass::CurrentPlayer && HouseClass::CurrentPlayer->OwnedBuildings>0;
    if(SellMode)MapClass::UnselectAll();else RestoreCursor();
}
void DisplayClass::SetRepairMode(int mode) noexcept {
    const bool active=mode==-1 ? !RepairMode : mode==0 ? false : mode==1 ? true : RepairMode;
    if(active==RepairMode || CurrentBuildingType)return;
    SellMode=PowerToggleMode=PlanningMode=PlaceBeaconMode=false;
    SetCursor(MouseCursorType::Default,false);
    RepairMode=active && HouseClass::CurrentPlayer && HouseClass::CurrentPlayer->OwnedBuildings>0;
    if(RepairMode)MapClass::UnselectAll();else RestoreCursor();
}

void DisplayClass::SetBeaconMode(int mode) noexcept {
    bool active=RepairMode;
    if(mode==-1)active=!active;
    else if(mode==0)active=false;
    else if(mode==1)active=true;
    if(active==PlaceBeaconMode || CurrentBuildingType)return;
    if(active && !BeaconManagerClass::Instance.CanPlaceBeacon(HouseClass::CurrentPlayer->ArrayIndex))active=false;
    SellMode=PowerToggleMode=PlanningMode=RepairMode=false;
    SetCursor(MouseCursorType::Default,false);
    PlaceBeaconMode=active;
    if(active)MapClass::UnselectAll();else RestoreCursor();
}
#endif
