// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9, display.cpp Active_Click.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
// YR 0x004AE750 removes TS group spreading and passes an explicit follow cell.
#include "yrpp/DisplayClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/Unsorted.h"

void DisplayClass::ActiveClick(ObjectClass* object,CellStruct cell,Action action) {
    if(Unsorted::ArmageddonMode)return;
    CellStruct clipped;
    cell=*MapClass::Instance.ClipToMap(&clipped,cell);
    int path=-1;BYTE waypoint=0;
    auto* point=HouseClass::CurrentPlayer->GetPlanningWaypointAt(&cell);
    if(point)HouseClass::CurrentPlayer->GetPlanningWaypointProperties(point,path,waypoint);
    auto& selected=ObjectClass::CurrentObjects;
    for(int i=0;i<selected.Count;++i) {
        selected[i]->AssignPlanningPath(path,static_cast<signed char>(waypoint));
        // Re-read after the virtual call, as in the original.
        auto* actor=selected[i];
        if(actor && (actor->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None)
            static_cast<TechnoClass*>(actor)->unknown_bool_430=false;
    }
    if(object) {
        for(int i=0;i<selected.Count;++i) {
            auto* actor=selected[i];
            actor->ObjectClickedAction(actor->MouseOverObject(object,false),object,false);
            Unsorted::MoveFeedback=false;
        }
    } else if(action==Action::NoMove) {
        // Only NoMove snapshots the selected count before dispatch.
        const int count=selected.Count;
        for(int i=0;i<count;++i) {
            auto* actor=selected[i];
            actor->CellClickedAction(actor->MouseOverCell(&cell,false,false),&cell,&cell,false);
            Unsorted::MoveFeedback=false;
        }
    } else {
        for(int i=0;i<selected.Count;++i) {
            auto* actor=selected[i];CellStruct no_follow{-1,-1};
            auto* follow=action==Action::Move || action==Action::PatrolWaypoint ? &cell : &no_follow;
            actor->CellClickedAction(actor->MouseOverCell(&cell,false,false),&cell,follow,false);
            Unsorted::MoveFeedback=false;
        }
    }
    Unsorted::MoveFeedback=true;
}
