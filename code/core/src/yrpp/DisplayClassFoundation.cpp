// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 display.cpp Set_Cursor_Shape / Cursor_Mark.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md. YR adds a separate pending-copy buffer.
#include "yrpp/DisplayClass.h"
#include "yrpp/CellClass.h"
#include <cstring>

#if !defined(RA2_YRPP_GAME)
namespace {
CellStruct active_foundation[120]{};
CellStruct pending_foundation[50]{};
void mark_foundation(DisplayClass& display,const CellStruct& base,
                     const CellStruct* offsets,AltCellFlags bit,bool mark) {
    // Original global 0x008A03F8 is initialized to (0,0), not (-1,-1).
    if(base==CellStruct{0,0})return;
    for(auto* offset=offsets;*offset!=CellStruct{0x7FFF,0x7FFF};++offset) {
        const CellStruct at{short(base.X+offset->X),short(base.Y+offset->Y)};
        if(display.CoordinatesLegal(at)) {
            auto& flags=display.GetCellAt(at)->AltFlags;
            if(mark)flags|=bit;else flags&=~bit;
        }
    }
}
CellStruct origin(CellStruct center,CellStruct offset) {
    return {short(center.X+offset.X),short(center.Y+offset.Y)};
}
CellStruct centered_offset(DisplayClass& display,const CellStruct* cells) {
    const auto bounds=display.FoundationBoundsSize(cells);
    // YR truncates to signed 16 bits before dividing, including extreme spans.
    return {short(short(1-bounds.X)/2),short(short(1-bounds.Y)/2)};
}
}
CellStruct (&DisplayClass::ActiveFoundationBuffer)[120]=active_foundation;
CellStruct (&DisplayClass::PendingFoundationBuffer)[50]=pending_foundation;

void DisplayClass::SetActiveFoundation(const CellStruct* cells) noexcept {
    if(CurrentFoundation_Data) {
        auto at=origin(CurrentFoundation_CenterCell,CurrentFoundation_TopLeftOffset);
        MarkFoundation(&at,false);
    }
    CurrentFoundation_TopLeftOffset={0,0};
    if(!cells){CurrentFoundation_Data=nullptr;return;}
    if(cells!=ActiveFoundationBuffer)std::memcpy(ActiveFoundationBuffer,cells,sizeof(ActiveFoundationBuffer));
    CurrentFoundation_Data=ActiveFoundationBuffer;
    CurrentFoundation_TopLeftOffset=centered_offset(*this,CurrentFoundation_Data);
    auto at=origin(CurrentFoundation_CenterCell,CurrentFoundation_TopLeftOffset);
    MarkFoundation(&at,true);
}
void DisplayClass::SetActiveFoundationCopy(const CellStruct* cells) noexcept {
    if(CurrentFoundationCopy_Data) {
        auto at=origin(CurrentFoundationCopy_CenterCell,CurrentFoundationCopy_TopLeftOffset);
        MarkFoundationCopy(&at,false);
    }
    CurrentFoundationCopy_TopLeftOffset={0,0};
    if(!cells){CurrentFoundationCopy_Data=nullptr;return;}
    if(cells!=PendingFoundationBuffer)std::memcpy(PendingFoundationBuffer,cells,sizeof(PendingFoundationBuffer));
    CurrentFoundationCopy_Data=PendingFoundationBuffer;
    CurrentFoundationCopy_TopLeftOffset=centered_offset(*this,CurrentFoundationCopy_Data);
    auto at=origin(CurrentFoundationCopy_CenterCell,CurrentFoundationCopy_TopLeftOffset);
    MarkFoundationCopy(&at,true);
}
void DisplayClass::MarkFoundation(CellStruct* base,bool mark) noexcept {
    mark_foundation(*this,*base,CurrentFoundation_Data,AltCellFlags::ContainsBuilding,mark);
}
void DisplayClass::MarkFoundationCopy(CellStruct* base,bool mark) noexcept {
    mark_foundation(*this,*base,CurrentFoundationCopy_Data,AltCellFlags::Unknown_4,mark);
}
#endif
