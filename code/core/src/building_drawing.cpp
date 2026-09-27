#include "building_drawing.hpp"
#include "yrpp/BuildingClass.h"
namespace game {
namespace {thread_local BuildingDrawing* current=nullptr;}
BuildingDrawing* building_drawing() noexcept{return current;}
DrawingStatus with_building_drawing(BuildingDrawing& drawing,void(*call)(void*),void* context) noexcept {
 auto* previous=current;current=&drawing;drawing.status=DrawingStatus::skipped;
 try{if(call)call(context);else drawing.status=DrawingStatus::invalid_argument;}
 catch(...){drawing.status=DrawingStatus::backend_failure;}
 current=previous;return drawing.status;
}
DrawingStatus draw_building_parts(BuildingClass& building,const TypeDrawingContext& context,
        SHPStruct* depth,int intensity,Point2D point,RectangleStruct clip,bool upper) noexcept{
 BuildingDrawing drawing{context,depth,intensity};auto*previous=current;current=&drawing;
 try{if(upper)building.Draw(point,clip);else building.DrawIt(&point,&clip);}
 catch(...){drawing.status=DrawingStatus::backend_failure;}
 current=previous;return drawing.status;
}
}
