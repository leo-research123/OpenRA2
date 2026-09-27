// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/display.cpp Help_Text; YR 0x004AE4F0 calibration.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DisplayClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"
#include "tooltip_platform.hpp"
#include "map_runtime.hpp"
#include <cwchar>

const wchar_t* DisplayClass::GetToolTip(UINT id) {
    // Original virtual 0x00693060: dragging, input lock, or camera motion.
    if(id<500 || id>900 || MouseClass::Instance.unknown_byte_554A || Unsorted::UserInputLocked ||
       (TacticalClass::Instance && TacticalClass::Instance->field_D8!=0))return nullptr;
    Point2D point;CellStruct cell;CoordStruct coords;ObjectClass* object=nullptr;
    if(!game::tooltip_pointer(point) || !game::tooltip_pick(point,cell,coords,object))return nullptr;
    const auto& runtime=game::map_runtime();
    if(runtime.debug_map && *runtime.debug_map){
        static wchar_t coordinate_text[32];
        std::swprintf(coordinate_text,32,L"(%d,%d)",coords.X/256,coords.Y/256);
        return coordinate_text;
    }
    auto* tile=MapClass::Instance.TryGetCellAt(cell);
    if(!tile)return nullptr;
    if(!(tile->AltFlags&AltCellFlags::Clear) && runtime.has_window && runtime.has_window())
        return StringTable::LoadString("TXT_SHADOW");
    if(!object)return nullptr;
    if((object->AbstractFlags&AbstractFlags::Techno)==AbstractFlags::None)return tile->GetUIName();
    const auto* techno=static_cast<TechnoClass*>(object);
    if(!techno->IsOwnedByCurrentPlayer){
        if(techno->CloakState==CloakState::Cloaked){
            const auto* player=HouseClass::CurrentPlayer;
            const auto* occupied=MapClass::Instance.TryGetCellAt(techno->GetCoords());
            if(!player || !occupied || !occupied->Sensors_InclHouse(player->ArrayIndex))return nullptr;
        }
        const auto* type=techno->GetTechnoType();
        if(!type || type->Invisible)return nullptr;
    }
    return object->GetUIName();
}
