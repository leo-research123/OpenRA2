#include "api/resource_queries.hpp"
#include "api/filesystem.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Straws.h"
#include "yrpp/FileFormats/SHP.h"
#include <memory>
#include <vector>
namespace game {
struct IniHandle { CCINIClass ini; };
bool load_ini(ResourceHandle& resources,const char* filename,IniHandle*& output) noexcept {
    if(output || !filename) return false;
    try {
        auto handle=std::make_unique<IniHandle>();
        struct Load { CCINIClass& ini; const char* filename; int result=0; } load{handle->ini,filename};
        std::string error;
        if(!with_resources(resources,[](void* p){ auto& l=*static_cast<Load*>(p); l.result=l.ini.LoadFromFile(l.filename); },&load,error) || load.result!=1) return false;
        output=handle.release(); return true;
    } catch(...) {return false;}
}
void destroy_ini(IniHandle* handle) noexcept {delete handle;}
bool ini_section_count(const IniHandle& h,std::uint32_t& count) noexcept {
    count=0; for(const auto* section:h.ini.Sections) { (void)section; ++count; } return true;
}
bool ini_section_name(const IniHandle& h,std::uint32_t index,const char*& name) noexcept {
    name=nullptr;
    for(const auto* section:h.ini.Sections) if(index--==0) { name=section->Name; return true; }
    return false;
}
bool ini_entry_count(IniHandle& h,const char* section,std::uint32_t& count) noexcept {
    count=0;
    try {h.ini.Reset(); if(auto* s=h.ini.GetSection(section)) for(const auto* e:s->Entries) {(void)e; ++count;} return true;} catch(...) {count=0;return false;}
}
bool ini_entry(IniHandle& h,const char* section,std::uint32_t index,const char*& key,const char*& value) noexcept {
    key=value=nullptr;
    try {h.ini.Reset(); if(auto* s=h.ini.GetSection(section)) for(const auto* e:s->Entries) if(index--==0) {key=e->Key;value=e->Value;return true;}} catch(...) {}
    return false;
}
bool ini_exists(IniHandle& h,const char* section,const char* key,bool& value) noexcept {
    value=false;
    try {h.ini.Reset();value=h.ini.Exists(section,key);return true;} catch(...) {return false;}
}
bool ini_string(IniHandle& h,const char* section,const char* key,char* output,std::uint32_t capacity) noexcept {
    if(!output || !capacity) return false;
    output[0]=0;
    try {h.ini.Reset(); if(!h.ini.Exists(section,key)) return false; h.ini.ReadString(section,key,"",output,capacity);return true;} catch(...) {return false;}
}
bool ini_integer(IniHandle& h,const char* section,const char* key,int fallback,int& value) noexcept {
    value=fallback;
    try {h.ini.Reset();value=h.ini.ReadInteger(section,key,fallback);return true;} catch(...) {return false;}
}
bool ini_bool(IniHandle& h,const char* section,const char* key,bool fallback,bool& value) noexcept {
    value=fallback;
    try {h.ini.Reset();value=h.ini.ReadBool(section,key,fallback);return true;} catch(...) {return false;}
}
bool ini_double(IniHandle& h,const char* section,const char* key,double fallback,double& value) noexcept {
    value=fallback;
    try {h.ini.Reset();value=h.ini.ReadDouble(section,key,fallback);return true;} catch(...) {return false;}
}
bool ini_preview(IniHandle& h,int& width,int& height,std::uint8_t* rgb,std::uint32_t capacity) noexcept {
    width=height=0;
    try {
        h.ini.Reset(); int size[4],fallback[4]{}; h.ini.Read4Integers(size,"Preview","Size",fallback);
        if(size[2]<=0 || size[3]<=0 || size[2]>2048 || size[3]>2048 || !h.ini.GetSection("PreviewPack")) return false;
        const int count=size[2]*size[3]*3;
        if(!rgb) {width=size[2];height=size[3];return capacity==0;}
        if(capacity<std::uint32_t(count)) return false;
        std::vector<unsigned char> packed(std::size_t(count)*2+4096);
        const auto read=h.ini.ReadUUBlock("PreviewPack",packed.data(),packed.size()); if(!read) return false;
        BufferStraw source(packed.data(),static_cast<int>(read)); LZOStraw stream(1,8192); stream.Get_From(source);
        if(stream.Get(rgb,count)!=count) return false;
        width=size[2];height=size[3];return true;
    } catch(...) {return false;}
}
bool resource_package_count(const ResourceHandle&,std::uint32_t& count) noexcept {
    count=0; for(const auto* mix:MixFileClass::MIXes) {(void)mix;++count;}return true;
}
bool resource_package(const ResourceHandle&,std::uint32_t index,const char*& name,std::uint32_t& entries) noexcept {
    name=nullptr;entries=0;
    try {for(const auto* mix:MixFileClass::MIXes) if(index--==0) {
        name=mix->FileName; const MixHeaderData* headers=nullptr;int count=0;
        if(mix->headers(headers,count)) entries=std::uint32_t(count);return true;
    }} catch(...) {} return false;
}
bool mount_map_resources(ResourceHandle& h) noexcept {
    try {bool mounted=false;std::string error;
        return with_resources(h,[](void* p){*static_cast<bool*>(p)=MixFileClass::LoadMapMixes();},&mounted,error)&&mounted;
    } catch(...) {return false;}
}
bool localized_string(const ResourceHandle&,const char* label,std::uint16_t* output,std::uint32_t capacity,std::uint32_t& length) noexcept {
    length=0;
    try {
        if(!label || !StringTable::IsLoaded) return false;
        const auto* text=StringTable::TryFetchString(label); if(!text || !*text) return false;
        while(text[length]) ++length;
        if(!output) return capacity==0;
        if(capacity<=length) return false;
        for(std::uint32_t i=0;i<=length;++i) output[i]=static_cast<std::uint16_t>(text[i]);
        return true;
    } catch(...) {return false;}
}
void unload_shapes() noexcept {Unload_All_Shapes();}
}
