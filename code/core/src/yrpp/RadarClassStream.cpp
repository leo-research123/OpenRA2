// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 radar.cpp Serialize, calibrated to YR 0x006568A0/0x00656AC0:
// points precede cells; only mode/state/suspended mode/movie phase follow.
// Surfaces, geometry and the animation timer are not serialized by these entries.
// Additional terms: third_party/opents/LICENSE.md.
#include "yrpp/RadarClass.h"
#include "type_stream.hpp"
#include <new>

#if !defined(RA2_YRPP_GAME)
HRESULT RadarClass::Save(IStream* stream) {
    auto result=DisplayClass::Save(stream);
    if(result<0) return result;
    const int points=unknown_points_125C.Count;
    result=game::write_stream_bytes(stream,&points,4);
    if(result<0) return result;
    for(int i=0;i<points;++i) {
        result=game::write_stream_bytes(stream,&unknown_points_125C[i],8);
        if(result<0) return result;
    }
    const int cells=unknown_cells_1124.Count;
    result=game::write_stream_bytes(stream,&cells,4);
    if(result<0) return result;
    for(int i=0;i<cells;++i) {
        result=game::write_stream_bytes(stream,&unknown_cells_1124[i],4);
        if(result<0) return result;
    }
    for(const auto* field:{&unknown_14B0,&unknown_14AC,&unknown_14B4,&unknown_14B8}) {
        result=game::write_stream_bytes(stream,field,4);
        if(result<0) return result;
    }
    return 0;
}
HRESULT RadarClass::Load(IStream* stream) {
    auto result=DisplayClass::Load(stream);
    if(result<0) return result;
    int count=0;
    result=game::read_stream_bytes(stream,&count,4);
    if(result<0) return result;
    // This is the original reconstruction point, not a replacement/cleanup
    // operation on live resources. See the Load precondition in RadarClass.h.
    ::new (&RadarAudio) AudioController;
    ::new (&unknown_points_125C) DynamicVectorClass<Point2D>;
    Point2D point{};
    int points_read=0;
    for(;points_read<count;++points_read) {
        result=game::read_stream_bytes(stream,&point,8);
        if(result<0) return result;
        try { unknown_points_125C.AddItem(point); }
        catch (...) { unknown_points_125C.IsInitialized=true; }
    }
    result=game::read_stream_bytes(stream,&count,4);
    if(result<0) return result;
    ::new (&unknown_cells_1124) DynamicVectorClass<CellStruct>;
    // YR reuses the completed point-loop counter's stack slot as the cell
    // read buffer. Preserve its bytes and reuse the buffer on short reads.
    auto cell=std::bit_cast<CellStruct>(points_read);
    for(int i=0;i<count;++i) {
        result=game::read_stream_bytes(stream,&cell,4);
        if(result<0) return result;
        try { unknown_cells_1124.AddItem(cell); }
        catch (...) { unknown_cells_1124.IsInitialized=true; }
    }
    for(auto& foundation:FoundationTypePixels)
        ::new (&foundation) DynamicVectorClass<Point2D>;
    for(auto* field:{&unknown_14B0,&unknown_14AC,&unknown_14B4,&unknown_14B8}) {
        result=game::read_stream_bytes(stream,field,4);
        if(result<0) return result;
    }
    ::new (&RadarAudio) AudioController;
    return 0;
}
#endif
