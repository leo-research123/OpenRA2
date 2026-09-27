// Existing YRpp 9402d7da hierarchy; field initialization calibrated to fixed
// YR 7b8a0685.
#include "yrpp/PowerClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "x87_integer.hpp"
#include <bit>

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
PowerClass::PowerClass()
    : RadarClass(), PowerNeedRedraw{}, unknown_timer_1510{}, unknown_151C{}, unknown_timer_1520{},
      unknown_152C{}, unknown_1530{}, unknown_1534{}, unknown_bool_1538{},
      PowerOutput{}, PowerDrain{} {
    // 63F6B0 uses the system clock in 16 ms ticks, not a stopped frame timer.
    unknown_timer_1510.StartTime = static_cast<int>(SystemTimer::GetTime());
    unknown_timer_1520.StartTime = static_cast<int>(SystemTimer::GetTime());
    PowerOutput = PowerDrain = -1;
}
PowerClass::~PowerClass() = default;

#if !defined(RA2_YRPP_GAME)
// Recovered from the fixed YR executable.
void PowerClass::Init_Clear() {
    RadarClass::Init_Clear();
    PowerOutput=PowerDrain=-1;
    unknown_timer_1510.Start(0);
    unknown_151C=0;
    unknown_timer_1520.Start(0);
    unknown_152C=unknown_1530=unknown_1534=0;
    unknown_bool_1538=false;
}

void PowerClass::FlashPower() {
    unknown_151C=10;
    unknown_timer_1510.Start(3);
}

int PowerClass::DesiredPowerHeight() const {
    const int maximum=(SidebarClass::CameoHeight+3)/3;
    DWORD drain=0,output=0;
    for(auto* building:HouseClass::CurrentPlayer->Buildings) {
        drain+=static_cast<DWORD>(building->Type->PowerDrain);
        output+=static_cast<DWORD>(building->Type->PowerBonus);
    }
    // 0x0063F850 uses the type ratings of every entry in the owner's list,
    // including damaged/offline buildings, to select the scale of the bar.
    int empty=game::x87_integer(400.0/(double(std::bit_cast<int>(drain+output))+400.0)*maximum);
    if(empty<=0)empty=0;
    if(empty>=maximum-1)empty=maximum-1;
    return maximum-empty;
}

int PowerClass::PowerUpdateDelay() const {
    const int desired=DesiredPowerHeight();
    int current=std::bit_cast<int>(unknown_152C+unknown_1530+unknown_1534);
    if(current>desired)current=desired;
    return game::x87_integer(double(current)/desired*5.0);
}

int PowerClass::DesiredPowerLevels(int& green,int& yellow,int& red) const {
    const int maximum=(SidebarClass::CameoHeight+3)/3;
    const int desired=DesiredPowerHeight();
    const auto* player=HouseClass::CurrentPlayer;
    const int drain=static_cast<int>(player->Power_Drain());
    const double surplus=std::bit_cast<int>(static_cast<DWORD>(player->Power_Output())-static_cast<DWORD>(drain));
    double yellowPower=100.0,greenPower=0.0;
    if(surplus<0.0)yellowPower=0.0;
    else {
        if(surplus<100.0)yellowPower=surplus;
        greenPower=surplus-yellowPower;
    }
    double redFraction=1.0,greenFraction=0.0,yellowFraction=0.0;
    const double total=double(player->Power_Drain())+yellowPower+greenPower;
    if(total>0.0) {
        redFraction=double(player->Power_Drain())/total;
        greenFraction=greenPower/total;
        yellowFraction=yellowPower/total;
    }
    const double redPips=desired*redFraction;
    const double yellowPips=desired*yellowFraction;
    const double greenPips=desired*greenFraction;
    red=game::x87_integer(redPips);
    yellow=game::x87_integer(yellowPips);
    green=game::x87_integer(greenPips);
    // Fractional remainders are assigned to red, in the original add order.
    red=game::x87_integer(((yellowPips-yellow)+(greenPips-green)+(redPips-red))+red+0.01);
    return maximum;
}

void PowerClass::RemovePowerPip() {
    int green,yellow,red;
    DesiredPowerLevels(green,yellow,red);
    if(std::bit_cast<int>(unknown_152C)>green)--unknown_152C;
    else if(std::bit_cast<int>(unknown_1534)>red)--unknown_1534;
    else if(std::bit_cast<int>(unknown_1530)>yellow)--unknown_1530;
}

void PowerClass::AddPowerPip() {
    int green,yellow,red;
    DesiredPowerLevels(green,yellow,red);
    if(std::bit_cast<int>(unknown_1534)<red)++unknown_1534;
    else if(std::bit_cast<int>(unknown_152C)<green)++unknown_152C;
    else if(std::bit_cast<int>(unknown_1530)<yellow)++unknown_1530;
}

void PowerClass::Update(const int& keyCode,const Point2D& mouseCoords) {
    if(SidebarClass::Instance.IsSidebarActive) {
        const auto redraw=[&] {
            PowerNeedRedraw=true;
            SidebarClass::Instance.SidebarNeedsRedraw=true;
            GScreenClass::Instance.MarkNeedsRedraw(0);
        };
        if(!unknown_bool_1538&&std::bit_cast<int>(unknown_151C)>0&&!unknown_timer_1510.GetTimeLeft()) {
            redraw();
            --unknown_151C;
            unknown_timer_1510.Start(3);
        }
        auto* player=HouseClass::CurrentPlayer;
        if(player->Power_Drain()!=PowerOutput||player->Power_Output()!=PowerDrain||unknown_bool_1538) {
            redraw();
            if(player->Power_Drain()!=PowerOutput||player->Power_Output()!=PowerDrain) {
                unknown_bool_1538=true;
                FlashPower();
            }
            PowerOutput=static_cast<int>(player->Power_Drain());
            PowerDrain=static_cast<int>(player->Power_Output());
            int green,yellow,red;
            DesiredPowerLevels(green,yellow,red);
            if(!unknown_timer_1520.GetTimeLeft()) {
                unknown_bool_1538=false;
                const auto adjust=[&](DWORD& count,int target) {
                    unknown_bool_1538=true;
                    if(std::bit_cast<int>(count)>target) { --count;AddPowerPip(); }
                    else { ++count;RemovePowerPip(); }
                };
                if(std::bit_cast<int>(unknown_1534)!=red)adjust(unknown_1534,red);
                else if(std::bit_cast<int>(unknown_152C)!=green)adjust(unknown_152C,green);
                else if(std::bit_cast<int>(unknown_1530)!=yellow)adjust(unknown_1530,yellow);
                if(unknown_bool_1538)unknown_timer_1520.Start(PowerUpdateDelay());
            }
        }
    }
    RadarClass::Update(keyCode,mouseCoords);
}

#endif
