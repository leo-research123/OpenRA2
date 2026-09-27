// YR LineTrail 0x556A20..0x556DF0. No corresponding class exists in the
// fixed OpenTS 44fac744 baseline. Draw 0x556C00 uses DSurface 0x4BEAC0.
#include "yrpp/LineTrail.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Memory.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <bit>

#if !defined(RA2_YRPP_GAME)
namespace { DynamicVectorClass<LineTrail*> trails; }
DynamicVectorClass<LineTrail*>& LineTrail::Array=trails;
#endif

LineTrail::LineTrail():Color(128,128,128),Owner(nullptr),Decrement(16),ActiveSlot(0),Trails{} {
    for(auto& node:Trails){node.Position=CoordStruct::Empty;node.Value=0;}
    Array.AddItem(this);
}
LineTrail::~LineTrail(){Detach();}
void LineTrail::Detach(){
    if(Owner){Owner->LineTrailer=nullptr;Owner=nullptr;}
}
void LineTrail::SetDecrement(int value){
    Decrement=GameOptionsClass::Instance.DetailLevel?value:std::bit_cast<int>(2u*unsigned(value));
}
void LineTrail::Update(){
    if(Owner&&Owner->Location!=Trails[ActiveSlot].Position){
        ActiveSlot=(ActiveSlot+31)%32;
        Trails[ActiveSlot]={Owner->Location,255};
    }
    for(auto& node:Trails)node.Value=std::max(0,std::bit_cast<int>(unsigned(node.Value)-unsigned(Decrement)));
}
void LineTrail::UpdateAll(){
    for(int i=Array.Count-1;i>=0;--i){
        auto* trail=Array[i];trail->Update();
        if(!trail->Owner&&!trail->Trails[trail->ActiveSlot].Value){
            Array.Remove(trail);GameDelete(trail);
        }
    }
}
void LineTrail::Draw() const {
    int first=ActiveSlot;
    if(Trails[first].Position==CoordStruct::Empty)return;
    for(int next=(first+1)%32;next!=ActiveSlot;next=(next+1)%32){
        const auto& a=Trails[first];const auto& b=Trails[next];
        if(!a.Value||b.Position==CoordStruct::Empty)break;
        Point2D start{},end{};
        TacticalClass::Instance->CoordsToClient(&a.Position,&start);
        TacticalClass::Instance->CoordsToClient(&b.Position,&end);
        // Headless map views have Tactical bounds without the UI surfaces.
        const auto bounds=DSurface::ViewBounds.Width>0?DSurface::ViewBounds:TacticalClass::ViewBounds;
        DSurface::SubmitBlendedLine(bounds,start,end,Color,a.Value,
            -2-TacticalClass::AdjustForZ(a.Position.Z),-2-TacticalClass::AdjustForZ(b.Position.Z));
        first=next;
    }
}
void LineTrail::DrawAll(){for(int i=Array.Count-1;i>=0;--i)Array[i]->Draw();}
void LineTrail::DeleteAll(){
    // Native ownership must release every allocation, including multiple
    // detached tails; do not skip shifted entries in a forward erase loop.
    while(Array.Count){auto* trail=Array[Array.Count-1];Array.Remove(trail);GameDelete(trail);}
    Array.Clear();
}
