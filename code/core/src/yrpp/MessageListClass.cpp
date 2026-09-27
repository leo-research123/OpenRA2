// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/msglist.cpp. EA/OpenTS; third_party/opents/LICENSE.md.
// YR 0x005D39D0..0x005D4C28 changes character storage, wrapping, animation,
// fixed 19-pixel line height and timer units. Existing class/list/buffers retained.
#include "yrpp/MessageListClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cwchar>
#include <cstring>
#if !defined(RA2_YRPP_GAME)
namespace {
TextLabelClass* next(TextLabelClass* value){return static_cast<TextLabelClass*>(value->GetNext());}
void free_buffer(MessageListClass& list,const TextLabelClass& label){
    for(int i=0;i<14;++i)if(label.Text==list.MessageBuffers[i])list.BufferAvail[i]=1;
}
void* expiry(int timeout){
    if(timeout==-1)return nullptr;
    return reinterpret_cast<void*>(std::uintptr_t(DWORD(timeout)+DWORD(Game::TickCount.GetTimeElapsed())));
}
}
MessageListClass::MessageListClass() noexcept
    :MessageList(nullptr),MessagePos{},MaxMessageCount(0),MaxCharacters(0),Height(0),
     EnableOverflow(false),IsEdit(false),AdjustEdit(false),EditPos{},EditLabel(nullptr),
     EditCurrentPos(0),EditInitialPos(0),CursorCharacter(0),OverflowStart(0),OverflowEnd(0),Width(0) {
    EditBuffer[0]=OverflowBuffer[0]=0;
    for(auto& available:BufferAvail)available=1;
}
MessageListClass::~MessageListClass() noexcept {Init(0,0,0,0,0,0,0,0,0,0);}
void MessageListClass::Init(int x,int y,int maximum,int characters,int,int editX,int editY,int overflow,int start,int end,int width) noexcept {
    Width=width-8;
    while(MessageList){auto* label=MessageList;MessageList=static_cast<TextLabelClass*>(label->Remove());delete label;}
    for(auto& available:BufferAvail)available=1;
    if(IsEdit)delete EditLabel;
    MessageList=nullptr;MessagePos={x,y};MaxMessageCount=std::min(maximum,14);MaxCharacters=std::min(characters,112);
    Height=19;EnableOverflow=static_cast<unsigned char>(overflow)!=0;IsEdit=false;
    AdjustEdit=editX==-1 || editY==-1;EditPos=AdjustEdit?MessagePos:Point2D(editX,editY);
    EditLabel=nullptr;EditBuffer[0]=OverflowBuffer[0]=0;
    EditCurrentPos=EditInitialPos=0;CursorCharacter=0;
    OverflowStart=DWORD(start);OverflowEnd=DWORD(end);
    if(end>=MaxCharacters)OverflowEnd=DWORD(MaxCharacters-1);
    if(start>=std::bit_cast<int>(OverflowEnd))OverflowStart=OverflowEnd-1;
}
TextLabelClass* MessageListClass::GetLabel(int id) const noexcept {
    for(auto* label=MessageList;label;label=next(label))
        if(DWORD(reinterpret_cast<std::uintptr_t>(label->UserData2))==DWORD(id))return label;
    return nullptr;
}
wchar_t* MessageListClass::GetMessage(int id) const noexcept {auto* label=GetLabel(id);return label?label->Text:nullptr;}
int MessageListClass::NumMessages() const noexcept {
    int count=0;for(auto* label=MessageList;label;label=next(label))++count;
    return count+(IsEdit && AdjustEdit);
}
void MessageListClass::ComputeY() noexcept {
    int y=MessagePos.Y+(IsEdit && AdjustEdit?Height:0);
    for(auto* label=MessageList;label;label=next(label)){label->Y=y;y+=Height;}
}
void MessageListClass::SetWidth(int width) noexcept {
    Width=width-8;
    for(auto* label=MessageList;label;label=next(label))label->PixWidth=DWORD(Width);
    if(IsEdit)EditLabel->PixWidth=DWORD(Width);
}
int MessageListClass::Manage() noexcept {
    int changed=0;
    for(auto* label=MessageList;label;){
        const int due=std::bit_cast<int>(DWORD(reinterpret_cast<std::uintptr_t>(label->UserData1)));
        if(due && Game::TickCount.GetTimeElapsed()>due){
            auto* following=next(label);MessageList=static_cast<TextLabelClass*>(label->Remove());
            free_buffer(*this,*label);delete label;changed=1;label=following;
        }else label=next(label);
    }
    if(changed)ComputeY();return changed;
}
int MessageListClass::TrimMessage(wchar_t* dest,wchar_t* source,int minimum,int maximum,int direction) noexcept {
    if(minimum<=0)return 0;
    const int length=int(std::wcslen(source));maximum=std::min(maximum,length);
    int count=minimum;
    if(direction){for(int i=maximum;i>=minimum;--i)if(source[i-1] && source[i-1]<=0x20){count=i;break;}}
    else {for(int i=minimum;i<=maximum;++i)if(source[i-1] && source[i-1]<=0x20){count=i;break;}}
    // Original requires 0 < minimum <= readable source length; overlapping
    // left shift has memmove semantics in its calibrated x86 CRT memcpy.
    if(dest){std::memmove(dest,source,std::size_t(count)*sizeof(wchar_t));dest[count]=0;}
    std::memmove(source,source+count,std::size_t(length-count+1)*sizeof(wchar_t));return count;
}
TextLabelClass* MessageListClass::AddMessage(const wchar_t* name,int id,const wchar_t* text,int color,TextPrintType style,int timeout,bool silent) noexcept {
    auto* font=BitFont::Instance;if(!text || !font)return nullptr;
    if(name && std::wcslen(name)>160)return nullptr;
    wchar_t message[162];message[0]=0;
    if(name){std::wcscpy(message,name);std::wcscat(message,L":");}
    const auto prefix=std::wcslen(message);
    const int available=Width-font->GetTextWidth(message)-8;
    if(available<=0)return nullptr;
    const int length=int(std::wcslen(text));const int fit=font->GetTextFit(text,available,111,true);
    if(fit<0)return nullptr;
    // Original recurses without progress if a printable glyph cannot fit.
    // Reject that invalid layout before publishing an empty success label.
    if(!fit && *text>=0x20)return nullptr;
    // Valid original callers fit the name and 111-character line into 162
    // code units. Reject an oversized external name rather than overflow.
    if(prefix+unsigned(fit)>=162)return nullptr;
    std::wcsncat(message,text,fit);message[prefix+fit]=0;
    if(MaxMessageCount>0 && NumMessages()+1>MaxMessageCount){
        auto* old=MessageList;if(!old)return nullptr;
        MessageList=static_cast<TextLabelClass*>(old->Remove());free_buffer(*this,*old);delete old;
    }
    // The original dereferences the allocation even on failure. Native new
    // cannot propagate through this noexcept boundary (same fatal precondition).
    auto* label=new TextLabelClass(message,MessagePos.X,MessagePos.Y,color,TextPrintType(DWORD(style)|0x8000));
    const auto& runtime=game::scenario_runtime();const int mode=runtime.session_mode(runtime.context);
    label->Animate=!silent && (mode==3?SessionClass::AnimateLanMessages:mode==4?SessionClass::AnimateInternetMessages:true);
    label->UserData1=expiry(timeout);label->UserData2=reinterpret_cast<void*>(std::uintptr_t(DWORD(id)));label->PixWidth=DWORD(Width);
    int slot=0;while(slot<14 && !BufferAvail[slot])++slot;
    if(slot==14){delete label;return nullptr;}
    BufferAvail[slot]=0;
    // Original clears 80 WCHARs, sets unit 80 to zero, then copies the line.
    std::wmemset(MessageBuffers[slot],0,80);MessageBuffers[slot][80]=0;
    std::wcscpy(MessageBuffers[slot],message);label->Text=MessageBuffers[slot];
    if(!silent)VocClass::PlayGlobal(RulesClass::Instance->IncomingMessage,0x2000,1.0f,nullptr);
    if(MessageList)label->AddTail(*MessageList);else MessageList=label;
    ComputeY();
    if(fit<length){
        auto* rest=text+fit;while(*rest && *rest<0x20)++rest;
        if(*rest)AddMessage(name,id,rest,color,style,timeout,true);
    }
    return label;
}
#endif
