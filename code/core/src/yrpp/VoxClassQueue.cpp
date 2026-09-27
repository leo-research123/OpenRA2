// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 vox.cpp Speak/Speak_AI and
// voxqueue.cpp request-control model. Copyright Electronic Arts / OpenTS;
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR uses its original circular audio links, four FIFO priority lists and one
// standard slot, not OpenTS's fixed eight-entry replacement queue.
#include "yrpp/VoxClass.h"
#include "yrpp/Audio.h"
#include "yrpp/Memory.h"
#include <bit>
#include <cstring>
#include <cstdlib>

#if !defined(RA2_YRPP_GAME)
namespace {
VoxClass::QueueLink interrupts{},critical{},priorities[4]{};
VoxClass::QueueEntry* standard{};
int initialized{},serial{},suppress{},paused{},current_control{},current_priority{};
VoxClass* current{};
AudioStream* stream{};
unsigned long long gap{};
void initialize(VoxClass::QueueLink& link) noexcept {link.Next=link.Previous=link.Sentinel=&link;}
VoxClass::QueueEntry* first(const VoxClass::QueueLink* link) noexcept {
    if(!link || !link->Next || link->Next==link->Next->Sentinel)return nullptr;
    return static_cast<VoxClass::QueueEntry*>(link->Next);
}
void unlink(VoxClass::QueueLink& node) noexcept {
    node.Previous->Next=node.Next;node.Next->Previous=node.Previous;
    node.Next=node.Previous=&node;
}
void append(VoxClass::QueueLink& list,VoxClass::QueueLink& node) noexcept {
    node.Next=&list;node.Previous=list.Previous;list.Previous=&node;node.Previous->Next=&node;
}
void clear(VoxClass::QueueLink& list) noexcept {
    while(auto* entry=first(&list)) {unlink(*entry);entry->Voice->unknown_int_50=2;YRMemory::Deallocate(entry);}
}
VoxClass::QueueEntry* find(const VoxClass::QueueLink& list,const VoxClass* voice) noexcept {
    for(auto* entry=first(&list);entry;entry=first(entry))if(entry->Voice==voice)return entry;
    return nullptr;
}
int increment(int& value) noexcept {return value=std::bit_cast<int>(unsigned(value)+1u);}
int decrement(int& value) noexcept {
    int result=value;
    if(value) {result=std::bit_cast<int>(unsigned(value)-1u);value=result<0?0:result;}
    return result;
}
}
VoxClass::QueueLink& VoxClass::InterruptQueue=interrupts;
VoxClass::QueueLink& VoxClass::CriticalQueue=critical;
VoxClass::QueueLink (&VoxClass::PriorityQueues)[4]=priorities;
VoxClass::QueueEntry*& VoxClass::StandardSlot=standard;
int& VoxClass::Initialized=initialized;
int& VoxClass::NextSerial=serial;
int& VoxClass::SuppressCount=suppress;
int& VoxClass::PauseCount=paused;
int& VoxClass::CurrentControl=current_control;
int& VoxClass::CurrentPriority=current_priority;
VoxClass*& VoxClass::Current=current;
AudioStream*& VoxClass::Stream=stream;
unsigned long long& VoxClass::GapMilliseconds=gap;

bool VoxClass::Initialize() noexcept {
    SuppressCount=PauseCount=0;
    for(auto& queue:PriorityQueues)initialize(queue);
    initialize(CriticalQueue);initialize(InterruptQueue);
    StandardSlot=nullptr;Initialized=1;
    if(!Stream)Stream=AudioStream::Create(3000,0);
    if(!Stream)return false;
    Stream->SetName("EVA Stream");
    Stream->SetVolumeControl(Audio::MasterVolume);
    Stream->SetSecondaryVolumeControl(Audio::SpeechVolume);
    return true;
}
void VoxClass::Shutdown() noexcept {
    ClearQueue();
    if(Stream) {Stream->Stop();Stream->Destroy();Stream=nullptr;}
}
void VoxClass::ClearQueue() noexcept {
    if(!Initialized)return;
    if(StandardSlot) {
        StandardSlot->Voice->unknown_int_50=2;YRMemory::Deallocate(StandardSlot);StandardSlot=nullptr;
    }
    clear(CriticalQueue);clear(InterruptQueue);
    for(auto& queue:PriorityQueues)clear(queue);
}
void YRPP_FASTCALL VoxClass::Enqueue(VoxClass* voice,int control,int priority) noexcept {
    auto* entry=static_cast<QueueEntry*>(YRMemory::Allocate(sizeof(QueueEntry)));
    if(!entry)std::abort(); // Original immediately dereferences this allocation.
    entry->Next=entry->Previous=entry;entry->Sentinel=nullptr;
    entry->Voice=voice;entry->Priority=priority;entry->Control=control;
    entry->Serial=NextSerial%100;increment(NextSerial);voice->unknown_int_50=1;
    if(control==int(VoxType::QueuedInterrupt))append(InterruptQueue,*entry);
    else if(control==int(VoxType::Queue))append(PriorityQueues[priority],*entry);
    else if(priority==int(VoxPriority::Critical))append(CriticalQueue,*entry);
    else if(InterruptQueue.Previous==&InterruptQueue && CriticalQueue.Previous==&CriticalQueue
            && (!StandardSlot || StandardSlot->Priority<priority)) {
        // 0x0075264F replaces the slot without disposing/resetting the old
        // request. Preserve that observable YR behavior rather than importing
        // OpenTS's replacement/capacity policy.
        StandardSlot=entry;
    } else {voice->unknown_int_50=2;YRMemory::Deallocate(entry);}
}
VoxClass::QueueEntry* YRPP_FASTCALL VoxClass::FindQueued(const VoxClass* voice) noexcept {
    if(auto* entry=find(CriticalQueue,voice))return entry;
    if(StandardSlot && StandardSlot->Voice==voice)return StandardSlot;
    for(int priority=3;priority>=0;--priority)if(auto* entry=find(PriorityQueues[priority],voice))return entry;
    return find(InterruptQueue,voice);
}
void YRPP_FASTCALL VoxClass::PlayIndex(int index,int control,int priority) noexcept {
    if(!Stream || index<0 || index>=Array.Count || SuppressCount)return;
    auto* voice=Array[index];if(voice==Current)return;
    if(control==-1)control=int(voice->Type);
    if(priority==-1)priority=int(voice->Priority);
    if(Current && control==int(VoxType::Interrupt)) {
        clear(InterruptQueue);
        if(Current) {Current->unknown_int_50=2;Current=nullptr;}
        if(Stream)Stream->Stop();
        ClearQueue();GapMilliseconds=0;
    }
    const auto* queued=FindQueued(voice);
    if(!queued || queued->Control!=control)Enqueue(voice,control,priority);
    Update();
}
void YRPP_FASTCALL VoxClass::Play(const char* name,int control,int priority) noexcept {
    if(name)PlayIndex(FindIndex(name),control,priority);
}
void VoxClass::Update() noexcept {
    if(Audio::Quiet || !Audio::IsAvailable() || !Stream || Stream->IsPlaying())return;
    const auto deadline=GapMilliseconds+Stream->EndTime();
    if(Audio::GetTime()<=deadline || PauseCount)return;
    if(Current) {Current->unknown_int_50=2;Current=nullptr;}
    auto* entry=first(&InterruptQueue);
    if(!entry)entry=first(&CriticalQueue);
    if(entry) {
        unlink(*entry);
        // This original discard deliberately does not reset its voice state.
        if(StandardSlot) {YRMemory::Deallocate(StandardSlot);StandardSlot=nullptr;}
    } else if(StandardSlot) {entry=StandardSlot;StandardSlot=nullptr;}
    else for(int priority=3;priority>=0;--priority) {
        entry=first(&PriorityQueues[priority]);
        if(entry) {unlink(*entry);break;}
    }
    if(!entry)return;
    auto* voice=entry->Voice;const int control=entry->Control,priority=entry->Priority;
    YRMemory::Deallocate(entry);
    if(!voice)return;
    voice->unknown_int_50=2;
    char filename[256];std::strcpy(filename,voice->GetFilename());std::strcat(filename,".WAV");
    if(Stream->PlayWAV(filename,true)) {
        GapMilliseconds=500;Current=voice;CurrentControl=control;CurrentPriority=priority;
        voice->unknown_int_50=0;
    }
}
void YRPP_FASTCALL VoxClass::SilenceIndex(int index) noexcept {
    if(index<0 || index>=Array.Count)return;
    while(auto* entry=FindQueued(Array[index])) {
        if(entry->Previous!=entry)unlink(*entry);
        if(entry==StandardSlot)StandardSlot=nullptr;
        entry->Voice->unknown_int_50=2;YRMemory::Deallocate(entry);
    }
}
void YRPP_FASTCALL VoxClass::Stop(bool clearQueue) noexcept {
    if(Current) {Current->unknown_int_50=2;Current=nullptr;}
    if(Stream)Stream->Stop();
    if(clearQueue)ClearQueue();
}
bool VoxClass::IsSpeaking() noexcept {
    Update();if(!Stream)return false;
    if(Stream->IsPlaying() || InterruptQueue.Previous!=&InterruptQueue
        || CriticalQueue.Previous!=&CriticalQueue || StandardSlot)return true;
    for(auto& queue:PriorityQueues)if(queue.Previous!=&queue)return true;
    return false;
}
int VoxClass::Suppress() noexcept {return increment(SuppressCount);}
int VoxClass::Unsuppress() noexcept {return decrement(SuppressCount);}
int VoxClass::Pause() noexcept {if(Stream)Stream->Pause();return increment(PauseCount);}
int VoxClass::Resume() noexcept {if(Stream)Stream->Resume();return decrement(PauseCount);}
void VoxClass::Reset() noexcept {Stop(true);PauseCount=SuppressCount=0;}
#endif
