#include "yrpp/MapClass.h"
#include "yrpp/PriorityQueueClass.h"
int YRPP_FASTCALL PriorityQueueClassNode::SurfaceDataCount(){
 const auto& r=MapClass::Instance.MapRect;return 2*r.Width*(r.Height+4);
}
int YRPP_FASTCALL PriorityQueueClassNode::ToSurfaceIndex(const CellStruct& c){
 const int w=MapClass::Instance.MapRect.Width;return ((c.X-c.Y+w-1)>>1)+w*(c.X-w+c.Y-1);
}
