// YR-only BeaconClass::DrawRadar (0x00430650), with approved whole-radar
// composition replacing the original dirty rectangle/pixel queue tail.
#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#include "type_drawing.hpp"
#if !defined(RA2_YRPP_GAME)
void BeaconClass::DrawRadar(Surface* surface,RectangleStruct bounds,bool clear) noexcept {
    using namespace game;
    if(clear)return; // The next complete composition supplies these background pixels.
    const auto& manager=BeaconManagerClass::Instance;
    const auto record=[](DrawingStatus result){record_ui_drawing(result);record_type_drawing_result(result);};
    if(manager.RadarBeaconAnimPeriod<=0){record(DrawingStatus::unavailable);return;}
    const int frame=Unsorted::CurrentFrame%manager.RadarBeaconAnimPeriod;
    if(frame>=manager.RadarBeaconFrameCount)return; // Includes the original erase frame.
    auto* image=BeaconManagerClass::RadarBeaconArt;
    if(!image){record(DrawingStatus::unavailable);return;}
    Point2D point;
    if(!RadarClass::Instance.GetCrdOnRadar(&point,&Coord,true)){record(DrawingStatus::unavailable);return;}
    auto* house=HouseClass::Array[HouseID];
    auto* scheme=ColorScheme::Array.GetItemOrDefault(house->ColorSchemeIndex);
    if(!scheme){record(DrawingStatus::unavailable);return;}
    ShapeDrawingRequest request;
    request.image=image;request.frame=frame;request.position=point;
    request.clip=bounds;request.flags=0x600;request.intensity=1000;
    if(surface) {
        const auto* context=active_type_drawing();
        if(!context || !context->legacy_target || !context->legacy_palette){record(DrawingStatus::unavailable);return;}
        try {
            auto result=context->legacy_target(context->backend_context,surface,request.target);
            if(result==DrawingStatus::drawn)result=context->legacy_palette(context->backend_context,scheme->LightConvert,request.palette);
            if(result==DrawingStatus::drawn)result=submit_type_shape(*context,request);
            record(result);
        } catch(...) {record(DrawingStatus::backend_failure);}
    } else {
        auto* ui=game_ui_frame();
        if(!ui || !ui->drawing.color_scheme_palette){record(DrawingStatus::unavailable);return;}
        // Shape positions are relative to the supplied clipping rectangle in
        // both original software blits and the GPU packet contract.
        request.target=ui->drawing.types.target;
        try {
            auto result=ui->drawing.color_scheme_palette(ui->drawing.types.backend_context,
                scheme->Colors,scheme->ShadeCount,request.palette);
            if(result==DrawingStatus::drawn)result=submit_type_shape(ui->drawing.types,request);
            record(result);
        } catch(...) {record(DrawingStatus::backend_failure);}
    }
}
#endif
