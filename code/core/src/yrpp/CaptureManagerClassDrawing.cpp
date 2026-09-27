// YR-only CaptureManagerClass::DrawLinks, 0x00472160 (absent from pinned
// OpenTS 44fac744). Retain YRpp's existing manager, node and Techno hierarchy.
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"

void CaptureManagerClass::DrawLinks() {
    const bool selected=Owner->IsSelected;
    for(int index=ControlNodes.Count-1;index>=0;--index){
        const auto* node=ControlNodes[index];
        auto* unit=node->Unit;
        const bool show=node->LinkDrawTimer.GetTimeLeft()>0||unit->IsSelected;
        if(!Owner||!unit||(!selected&&!show))continue;
        auto to=unit->Location;
        to.Z+=unit->GetTechnoType()->LeptonMindControlOffset;
        // Every fifth node cycles back to AlternateFLH0. Do not use the
        // normal weapon muzzle or the unit center as the link's source.
        CoordStruct from;
        const auto color=Owner->Owner->LaserColor;
        Owner->GetFLH(&from,-1-index%5,CoordStruct::Empty);
        Owner->DrawMindControlLine(from,to,color);
    }
}
