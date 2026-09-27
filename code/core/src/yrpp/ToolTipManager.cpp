// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/tooltip.cpp, calibrated to YR 0x724000..0x724C00.
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ToolTipManager.h"
#include "yrpp/StringTable.h"
#include "tooltip_platform.hpp"
#include "map_runtime.hpp"
#include <cwchar>

ToolTipManager::ToolTipManager(HWND window) noexcept
    : CurrentToolTip(nullptr),hWnd(window),IsActive(false),CurrentMousePosition{},
      CurrentToolTipData{},ToolTipDelay(1000),LastToolTipDelay(0),ToolTipLifeTime(10000) {}

ToolTipManager::~ToolTipManager() {
    game::tooltip_kill_timer(*this);
    Hide();
    ToolTipIndex.Clear();
    for(auto* tip:ToolTips) GameDelete(tip);
    ToolTips.Clear();
}
bool ToolTipManager::Update(ToolTipManagerData&) { return true; }
void ToolTipManager::MarkToRedraw(ToolTipManagerData&) {
    CurrentToolTipData.Dimension={}; CurrentToolTip=nullptr;
}
void ToolTipManager::Draw(bool refresh) {
    if(refresh) Process();
    if(CurrentToolTip) DrawText(CurrentToolTipData);
}
void ToolTipManager::DrawText(ToolTipManagerData&) {}
wchar_t* ToolTipManager::GetToolTipText(unsigned int) { return nullptr; }
void ToolTipManager::SetState(bool state) {
    if(IsActive==state)return;
    IsActive=state;
    if(!state){game::tooltip_kill_timer(*this);Hide();}
}
int ToolTipManager::GetTimerDelay(){return ToolTipDelay;}
void ToolTipManager::SetTimerDelay(int value){ToolTipDelay=value;}
void ToolTipManager::SaveTimerDelay(){LastToolTipDelay=ToolTipDelay;}
void ToolTipManager::RestoreTimeDelay(){ToolTipDelay=LastToolTipDelay;}
int ToolTipManager::GetLifeTime(){return ToolTipLifeTime;}
void ToolTipManager::SetLifeTime(int value){ToolTipLifeTime=value;}
int ToolTipManager::GetToolTipCount(){return ToolTips.Count;}
bool ToolTipManager::Add(ToolTip& source) {
    ToolTip* tip=nullptr;
    if(ToolTipIndex.TryGet(int(source.GadgetID),tip))return false;
    // Native allocation failure is reported without leaking a partially added
    // region or propagating an exception across the original method boundary.
    try {
        tip=GameCreate<ToolTip>(); if(!tip)return false;
        *tip=source;
        if(!ToolTips.AddItem(tip)){GameDelete(tip);return false;}
        if(ToolTipIndex.AddIndex(int(tip->GadgetID),tip))return true;
    } catch(...) {}
    if(tip){ToolTips.Remove(tip);GameDelete(tip);}
    return false;
}
void ToolTipManager::Remove(unsigned int id) {
    ToolTip* tip=nullptr;if(!ToolTipIndex.TryGet(int(id),tip))return;
    if(CurrentToolTip==tip)Hide();
    ToolTipIndex.RemoveIndex(int(id));ToolTips.Remove(tip);GameDelete(tip);
}
bool ToolTipManager::Find(unsigned int id,ToolTip& output) {
    ToolTip* tip=nullptr;if(!ToolTipIndex.TryGet(int(id),tip))return false;
    output=*tip;return true;
}
ToolTip* ToolTipManager::FindFromPosition(Point2D& point) {
    // YR accepts both far edges and returns the first registered region.
    for(auto* tip:ToolTips){const auto& r=tip->Bounds;
        if(point.X>=r.X && std::int64_t(point.X)<=std::int64_t(r.X)+r.Width &&
           point.Y>=r.Y && std::int64_t(point.Y)<=std::int64_t(r.Y)+r.Height)return tip;
    }
    return nullptr;
}
bool ToolTipManager::Process() {
    if(!CurrentToolTip)return false;
    const wchar_t* text=CurrentToolTip->Text ? StringTable::LoadString(CurrentToolTip->Text)
                                           : GetToolTipText(CurrentToolTip->GadgetID);
    if(text && *text){
        // Original capacity is 0x100 UTF-16 code units. Keep native strings
        // terminated when a malformed/oversized CSF exceeds that capacity.
        std::wcsncpy(CurrentToolTipData.HelpText,text,0xFF);
        CurrentToolTipData.HelpText[0xFF]=0;
        CurrentToolTipData.Dimension={CurrentMousePosition.X,CurrentMousePosition.Y,0,0};
        if(Update(CurrentToolTipData))return true;
    }
    CurrentToolTipData.Dimension={};CurrentToolTip=nullptr;return false;
}
void ToolTipManager::Hide(){if(CurrentToolTip)MarkToRedraw(CurrentToolTipData);}
bool ToolTipManager::IsToolTipShowing(){return CurrentToolTip!=nullptr;}
void ToolTipManager::ProcessMessage(MSG* message) {
    if(!IsActive || !message)return;
    switch(message->message){
    case 0x201:case 0x202:case 0x204:case 0x205:case 0x207:case 0x208:
        game::tooltip_kill_timer(*this);Hide();return;
    case 0x200:
        if(ToolTipDelay && !(game::map_runtime().debug_map && *game::map_runtime().debug_map)){
            game::tooltip_kill_timer(*this);game::tooltip_set_timer(*this,ToolTipDelay);Hide();return;
        }
        break;
    case 0x113:
        if(message->wParam!=0x54544950)return;
        game::tooltip_kill_timer(*this);
        if(CurrentToolTip){Hide();return;}
        break;
    default:return;
    }
    if(!game::tooltip_pointer(CurrentMousePosition)){Hide();return;}
    CurrentToolTip=FindFromPosition(CurrentMousePosition);
    if(Process())game::tooltip_set_timer(*this,ToolTipLifeTime);
}
