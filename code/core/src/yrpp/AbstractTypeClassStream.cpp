// Original 00410320/00410380/00410960 record contract, explicit target layout.
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/StringTable.h"
#include "type_stream.hpp"
#include <cstring>
bool AbstractTypeClass::EncodeTypeRecordBase(unsigned char* record, std::size_t size) const {
    if(!record || size<152 || !std::memchr(UINameLabel,0,sizeof(UINameLabel)) ||
       !std::memchr(Name,0,sizeof(Name))) return false;
    game::put_u32(record+16,UniqueID);
    game::put_u32(record+20,static_cast<std::uint32_t>(AbstractFlags));
    game::put_u32(record+24,unknown_18);
    game::put_u32(record+28,static_cast<std::uint32_t>(RefCount));
    record[32]=Dirty;
    std::memcpy(record+36,ID,24); record[60]=zero_3C;
    std::memcpy(record+61,UINameLabel,32);
    // UIName is reconstructed from the label, never serialized as a host pointer.
    game::put_u32(record+96,0);
    std::memcpy(record+100,Name,49);
    return true;
}
bool AbstractTypeClass::DecodeTypeRecordBase(const unsigned char* record, std::size_t size) {
    if(!record || size<152 || record[60] || record[32]>1 ||
        !std::memchr(record+61,0,32) || !std::memchr(record+100,0,49)) return false;
    const auto* label=reinterpret_cast<const char*>(record+61);
    const wchar_t* display=*label ? StringTable::LoadString(label) : L"";
    UniqueID=game::get_u32(record+16);
    AbstractFlags=static_cast<::AbstractFlags>(game::get_u32(record+20));
    unknown_18=game::get_u32(record+24);
    // 00410380 explicitly retains the live RefCount across loading.
    Dirty=record[32]!=0;
    std::memcpy(ID,record+36,24); zero_3C=0;
    std::memcpy(UINameLabel,record+61,32); UIName=display;
    std::memcpy(Name,record+100,49);
    return true;
}
