#include "yrpp/MouseClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
void ScrollClass::ClearDragBand() {
    DisplayClass::ClearDragBand();
    unknown_byte_554A = 0;
}
#endif

// Normal tactical dispatch from 0x6930A0. Unlike the host adapter, this layer
// owns action classification and invokes the existing Display release protocol.
bool ScrollClass::ProcessMouseButton(unsigned button, bool pressed, Point2D point) noexcept {
    if (!Game::IsActive || !Unsorted::TacticalActive || Unsorted::UserInputLocked || !TacticalClass::Instance ||
        !HouseClass::CurrentPlayer)
        return false;
    CellStruct cell;
    CoordStruct coords;
    ObjectClass* object = nullptr;
    BYTE fogged = 0, shrouded = 0;
    if (button == 1) {
        if (pressed) {
            const bool picked = ProcessClickCoords(&point, &cell, &coords, &object, &fogged, &shrouded);
            if (!picked) return false;
            if (Game::PlanningManager_LeftDown(point)) return true;
            if (unknown_byte_554A) return false;
            const auto action = DecideAction(cell, object, 1);
            DisplayClass::Instance.DisplayClass::ConvertAction(cell, shrouded != 0, object, action, false);
            DisplayClass::Instance.DisplayClass::LeftMouseButtonClick(&point);
            unknown_byte_554A = 1;
        } else {
            if (Game::PlanningManager_LeftUp(point)) return true;
            if (!unknown_byte_554A) return false;
            ProcessClickCoords(&point, &cell, &coords, &object, &fogged, &shrouded);
            const auto action = DecideAction(cell, object, 0);
            DisplayClass::Instance.DisplayClass::LeftMouseButtonUp(coords, cell, object, action, 0);
            unknown_byte_554A = 0;
        }
        return true;
    }
    if (button == 2) {
        if (pressed) {
            Game::PlanningManager_RightDown(point);
            if (unknown_byte_554A || !ProcessClickCoords(&point, &cell, &coords, &object, &fogged, &shrouded))
                return false;
            ScrollClass::Instance.ScrollClass::RightMouseButtonClick(&point);
            unknown_byte_554A = 1;
        } else {
            Game::PlanningManager_WM_RBUTTONUP_63AB00(point);
            if (!unknown_byte_554A) return false;
            ScrollClass::Instance.ScrollClass::RightMouseButtonUp(0);
            DisplayClass::ClearDragBand();
            unknown_byte_554A = 0;
        }
        return true;
    }
    return false;
}
