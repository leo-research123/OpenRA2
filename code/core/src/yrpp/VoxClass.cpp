// YR EVA definitions at 0x00752CB0..0x00753398. Retains the existing VoxClass
// object and array; fixed OpenTS vox.cpp provides the speech-module foundation,
// but YR's per-side INI records have no equivalent TS object to substitute.
#include "yrpp/VoxClass.h"
#include "yrpp/Audio.h"
#include "yrpp/Memory.h"
#include <cstring>
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {DynamicVectorClass<VoxClass*> voices;int eva_index{};}
DynamicVectorClass<VoxClass*>& VoxClass::Array=voices;
int& VoxClass::EVAIndex=eva_index;
VoxClass::VoxClass(const char* name) noexcept {
    Volume=1.0f;
    if(!name || std::strlen(name)>=sizeof(Name))std::abort();
    std::strcpy(Name,name);Yuri[0]=Russian[0]=Allied[0]='\0';
    Priority=VoxPriority::Normal;Type=VoxType::Standard;unknown_int_50=2;
    try {Array.AddItem(this);}catch(const std::bad_alloc&) {Array.IsInitialized=true;}
}
VoxClass::~VoxClass() noexcept {Array.Remove(this);}
const char* YRPP_FASTCALL VoxClass::GetName(int index) noexcept {
    // Valid contract: -1 or a nonnegative index. Other negative values were
    // unchecked original array accesses; native treats them as a miss.
    return index<0 || index>=Array.Count?"<none>":Array[index]->Name;
}
const char* VoxClass::GetFilename() const noexcept {
    return EVAIndex==0?Allied:EVAIndex==1?Russian:Yuri;
}
bool VoxClass::LoadFromINI(CCINIClass* ini) noexcept {
    ini->Reset();Volume=1.0f;if(!ini->GetSection(Name))return false;
    Volume=static_cast<float>(ini->ReadDouble(Name,"Volume",Volume));
    char value[500];ini->ReadString(Name,"Type","",value,sizeof(value));
    if(!_strcmpi(value,"QUEUE"))Type=VoxType::Queue;
    else if(!_strcmpi(value,"STANDARD"))Type=VoxType::Standard;
    else if(!_strcmpi(value,"INTERRUPT"))Type=VoxType::Interrupt;
    else if(!_strcmpi(value,"QUEUED_INTERRUPT"))Type=VoxType::QueuedInterrupt;
    ini->ReadString(Name,"Priority","",value,sizeof(value));
    if(!_strcmpi(value,"LOW"))Priority=VoxPriority::Low;
    else if(!_strcmpi(value,"NORMAL"))Priority=VoxPriority::Normal;
    else if(!_strcmpi(value,"IMPORTANT"))Priority=VoxPriority::Important;
    else if(!_strcmpi(value,"CRITICAL"))Priority=VoxPriority::Critical;
    ini->ReadString(Name,"Yuri","",value,sizeof(value));std::strncpy(Yuri,value,9);Yuri[8]='\0';
    ini->ReadString(Name,"Russian","",value,sizeof(value));std::strncpy(Russian,value,9);Russian[8]='\0';
    ini->ReadString(Name,"Allied","",value,sizeof(value));std::strncpy(Allied,value,9);Allied[8]='\0';
    return true;
}
bool YRPP_FASTCALL VoxClass::LoadAllFromINI(CCINIClass* ini) noexcept {
    if(!ini->GetSection("DialogList"))return false;
    const int count=ini->GetKeyCount("DialogList");bool result=false;char name[200];
    for(int i=0;i<count;++i) {
        result=false;
        if(ini->ReadString("DialogList",ini->GetKeyName("DialogList",i),"",name,sizeof(name))) {
            auto* voice=Find(name);
            if(!voice) {voice=GameCreate<VoxClass>(name);if(!voice)std::abort();}
            result=voice->LoadFromINI(ini);
        }
    }
    return result;
}
void VoxClass::DeleteAll() noexcept {
    Stop(true);
    while(Array.Count>0)GameDelete(Array[0]);
}
#endif
