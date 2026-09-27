// Copyright 2025 Electronic Arts Inc.
// Copyright 2026 OpenTS contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 additional terms and warranty disclaimers apply;
// see code/third_party/opents/LICENSE.md. Modified by RedAlert2Open, 2026.
// Door transition algorithm, OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9;
// calibrated to YR 0x004A5150..0x004A5360 and the game's x87 control word.
#include "yrpp/TransitionTimer.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
namespace {
double truncate_fp(double nearest,double residual){
 return (nearest>0&&residual<0)||(nearest<0&&residual>0)?std::nextafter(nearest,0.0):nearest;
}
int duration(double time,double& scaled){
 const double product=time*900.0;scaled=truncate_fp(product,std::fma(time,900.0,-product));
 return std::isfinite(scaled)&&scaled>=-2147483648.0&&scaled<2147483648.0?
     static_cast<int>(scaled):std::numeric_limits<int>::min();
}
int subtract(int a,int b){return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b));}
int remaining(const CDTimerClass& timer){
 if(timer.StartTime==-1)return timer.TimeLeft;
 const int elapsed=subtract(Unsorted::CurrentFrame,timer.StartTime);
 return elapsed>=timer.TimeLeft?0:subtract(timer.TimeLeft,elapsed);
}
}
double TransitionTimer::PercentageDone(){
 const auto total=std::bit_cast<std::int32_t>(Rate2);
 if(!State1||!total)return 1.0;
 const double done=subtract(total,remaining(ActionTimer));
 const double nearest=done/total;
 const double residual=std::fma(-nearest,double(total),done);
 return truncate_fp(nearest,total<0?-residual:residual);
}
bool TransitionTimer::IsTimerFinished(){return PercentageDone()==1.0;}
void TransitionTimer::StartTimer11(double time){
 if(!State1&&State2)return;
 const int frames=duration(time,Rate1);State1=State2=true;ActionTimer.Start(frames);Rate2=std::uint32_t(frames);
}
void TransitionTimer::StartTimer10(double time){
 if(!State1&&!State2)return;
 const int frames=duration(time,Rate1);State1=true;State2=false;ActionTimer.Start(frames);Rate2=std::uint32_t(frames);
}
void TransitionTimer::Update(){
 if(!State1)return;
 ActionTimer.TimeLeft=subtract(std::bit_cast<std::int32_t>(Rate2),remaining(ActionTimer));State2=!State2;
}
void TransitionTimer::SetToDone(){if(State1)State1=false;}
