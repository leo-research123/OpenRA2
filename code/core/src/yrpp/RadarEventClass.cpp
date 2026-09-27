// RadarEventClass, calibrated to fixed gamemd 65FA70..660B80.
// No replacement event model: original 0x40 records and original Array/history.
#include "yrpp/RadarEventClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Memory.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <new>

namespace {
// 7F0998, 17 records of {suppression distance, visible frames, retained frames,
// suppress-nearby}. The binary reads this table, not the Rules TypeLists.
constexpr int parameters[17][4]={
    {8,200,400,1},{8,200,400,0},{8,200,400,0},{8,200,600,1},
    {8,200,400,1},{6,200,400,1},{2,0,200,1},{8,0,200,1},
    {2,0,400,1},{5,0,400,0},{8,0,100,0},{8,200,200,1},
    {8,200,400,0},{8,0,5,0},{8,0,200,1},{8,0,400,1},{8,200,600,1}};
constexpr double quarter_pi=0.7853981633974483,half_pi=1.570796326794897,two_pi=6.283185307179586;
float store_float(double value) noexcept {
    const auto nearest=static_cast<float>(value);
    return std::abs(double(nearest))>std::abs(value) ?
        std::bit_cast<float>(std::bit_cast<unsigned>(nearest)-1u) : nearest;
}
bool valid(RadarEventType type) noexcept { return unsigned(type)<17; }
}

RadarEventClass::RadarEventClass(RadarEventType type,CellStruct cell) noexcept
    : Type(type),RadarX{},RadarY{},Speed{},RotationValue(std::bit_cast<float>(1061752795u)),
      RotationSpeed(RulesClass::Instance->RadarEventRotationSpeed),ColorValue{},
      ColorSpeed(RulesClass::Instance->RadarEventColorSpeed),MapCoords(cell),
      DurationTimer(0),VisibilityTimer(0),Rotating(true),Visible(true) {
    const auto& radar=RadarClass::Instance;
    const auto point=radar.CellToRadar(cell);
    RadarX=point.X-radar.unknown_rect_149C.X;
    RadarY=point.Y-radar.unknown_rect_149C.Y;
    Speed=store_float(std::max({RadarX,RadarY,radar.unknown_rect_149C.Width-RadarX,
        radar.unknown_rect_149C.Height-RadarY}));
}
RadarEventClass::~RadarEventClass() { Array.Remove(this); }
bool RadarEventClass::Create(RadarEventType type,CellStruct cell) noexcept {
    if (!valid(type) || !RulesClass::Instance || !std::isfinite(RadarClass::Instance.RadarSizeFactor) ||
        RadarClass::Instance.RadarSizeFactor<=0 ||
        !std::isfinite(RulesClass::Instance->RadarEventSpeed) ||
        !std::isfinite(RulesClass::Instance->RadarEventRotationSpeed) ||
        !std::isfinite(RulesClass::Instance->RadarEventColorSpeed)) return false;
    try {
        if (parameters[int(type)][3]) for (const auto* event : Array) {
            if (event->Type!=type) continue;
            const auto dx=std::bit_cast<short>(static_cast<unsigned short>(int(cell.X)-event->MapCoords.X));
            const auto dy=std::bit_cast<short>(static_cast<unsigned short>(int(cell.Y)-event->MapCoords.Y));
            const auto distance=std::bit_cast<short>(static_cast<unsigned short>(int(std::sqrt(double(dx)*dx+double(dy)*dy))));
            if (distance<parameters[int(type)][0]) return false;
        }
        if (Array.Count>=Array.Capacity && !Array.Reserve(Array.Count+1)) return false;
        auto* storage=YRMemory::Allocate(sizeof(RadarEventClass));
        if (!storage) return false;
        auto* event=::new(storage) RadarEventClass(type,cell);
        if (!Array.AddItem(event)) { event->~RadarEventClass(); YRMemory::Deallocate(event); return false; }
        HistoryIndex=(HistoryIndex+1)%8; History[HistoryIndex]=cell; HistoryCursor=HistoryIndex;
        RadarClass::Instance.unknown_bool_14D9=true;
        return true;
    } catch (...) { return false; }
}
void RadarEventClass::Clear() noexcept {
    while (Array.Count) {
        auto* event=Array[Array.Count-1];
        event->~RadarEventClass(); YRMemory::Deallocate(event);
    }
}
void RadarEventClass::Update() noexcept {
    if (!Visible || !RulesClass::Instance || !valid(Type)) return;
    if (!Rotating && !VisibilityTimer.HasTimeLeft()) Visible=false;
    const auto& rules=*RulesClass::Instance;
    const double radius=std::max(double(Speed)-rules.RadarEventSpeed,double(store_float(rules.RadarEventMinRadius)));
    Speed=store_float(radius);
    // 0x65FE72..0x65FE94 multiplies by 2/pi before _ftol.
    const double angle=double(RotationValue)+quarter_pi;
    const float remainder=store_float(angle-int(angle*(2.0/3.141592653589793))*half_pi);
    if (Rotating) {
        if (std::abs(radius-rules.RadarEventMinRadius)>=0.01) RotationValue=store_float(double(RotationValue)+RotationSpeed);
        else if (remainder>=RotationSpeed) {
            RotationValue=store_float(double(RotationValue)+RotationSpeed);
            const float reduced=store_float(double(RotationSpeed)-double(rules.RadarEventRotationSpeed)*0.02);
            RotationSpeed=store_float(std::max(double(rules.RadarEventRotationSpeed)*0.33333334,double(reduced)));
        } else {
            RotationValue=store_float(double(RotationValue)+remainder); Rotating=false;
            VisibilityTimer.Start(parameters[int(Type)][1]); DurationTimer.Start(parameters[int(Type)][2]);
        }
    }
    if (RotationValue>two_pi) RotationValue=store_float(double(RotationValue)-two_pi);
    const double color=double(ColorValue)+ColorSpeed;
    ColorValue=store_float(color);
    if (color<0 && ColorSpeed<0) { ColorSpeed=-ColorSpeed; ColorValue=0; }
    else if (color>1 && ColorSpeed>0) { ColorSpeed=-ColorSpeed; ColorValue=1; }
}
bool RadarEventClass::UpdateAll() noexcept {
    const bool changed=Array.Count>0;
    for (auto* event : Array) event->Update();
    return changed;
}
void RadarEventClass::RemoveFinished() noexcept {
    for (int i=Array.Count-1;i>=0;--i) {
        auto* event=Array[i];
        if (!event->Rotating && !event->DurationTimer.HasTimeLeft()) {
            event->~RadarEventClass(); YRMemory::Deallocate(event);
        }
    }
}
static_assert(sizeof(RadarEventClass)==0x40);
static_assert(offsetof(RadarEventClass,DurationTimer)==0x24);
static_assert(offsetof(RadarEventClass,Visible)==0x3D);

bool RadarEventClass::GetVertices(Point2D* output,unsigned count) const noexcept {
    if (!output || count<4 || !std::isfinite(Speed) || Speed<0 || Speed>65536 ||
        !std::isfinite(RotationValue) || std::abs(RotationValue)>10000) return false;
    // 4CAD00/4CACB0 quantize into the original 8192-step sine table.
    // Reproduce its toward-zero float entries; two cardinal residuals are
    // fixed table words rather than the current platform libm residuals.
    int n=int(double(RotationValue)*std::bit_cast<float>(0x4522F983u));
    int index=(n/2)%8192; if (index<0) index+=8192;
    const auto sine=[](int i) {
        if (i==4096) return std::bit_cast<float>(0x250D3000u);
        if (i==8192) return std::bit_cast<float>(0xA58D3000u);
        return store_float(std::sin(i*(two_pi/8192.0)));
    };
    int si=index,ci=index+2048;
    if (n&1) {if(si<8191) ++si;if(ci<10239) ++ci;}
    const int x=int(store_float(double(sine(ci))*Speed));
    const int y=int(store_float(double(sine(si))*Speed));
    output[0]={x,y};output[1]={-y,x};output[2]={-x,-y};output[3]={y,-x};
    return true;
}
