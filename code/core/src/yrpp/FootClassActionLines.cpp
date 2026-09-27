// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Draw_Action_Line and actionline.cpp Action_Line_Coord.
// YR 0x004DC060: shared timer, attack priority, route tail, bridge and palette indexes.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "map_runtime.hpp"
#include "type_drawing.hpp"

void FootClass::DrawActionLines(bool force,DWORD dashed) {
    const bool attack=Target!=nullptr;
    if((!attack&&!Destination)||(!force&&ActionLineTimer.GetTimeLeft()<=0))return;
    CoordStruct from,to;
    if(attack){
        vt_entry_300(&from,0);
        PredictTargetCoords(&to);
    }else{
        from=Location;
        auto* target=unknown_abstract_array_588.Count?
            unknown_abstract_array_588[unknown_abstract_array_588.Count-1]:Destination;
        to=target->GetCoords();
        auto& map=MapClass::Instance;
        if(map.IsWithinUsableArea(to)&&
            (static_cast<unsigned>(map.GetCellAt(to)->Flags)&0x100u))
            to.Z=CellClass::BridgeHeight+map.GetCellFloorHeight(to);
    }
    const auto* palette=game::map_runtime().action_line_palette;
    if(!palette){game::record_type_drawing_result(game::DrawingStatus::unavailable);return;}
    DrawActionLine(from,to,palette->Entries[attack?8:3],attack?false:bool(dashed&0xFFu),false);
}
