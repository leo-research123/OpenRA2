// Supplied 006A4780/006A48A0: target base record, then count and country indices.
#include "yrpp/SideClass.h"
#include "type_stream.hpp"
HRESULT YRPP_STDCALL SideClass::Save(IStream* stream, BOOL clear) {
    if(!stream) return game::type_stream_error(game::stream_pointer_error);
    try {
        if(HouseTypes.Count<0 || static_cast<unsigned>(HouseTypes.Count)>game::type_stream_max_elements())
            return game::type_stream_error(game::stream_invalid_record);
        std::array<unsigned char,180> record{};
        if(!EncodeTypeRecordBase(record.data(),record.size())) return game::type_stream_error(game::stream_invalid_record);
        constexpr std::uint32_t vtables[]{0x7f2ec0,0x7f2ea4,0x7f2e9c,0x7f2e94};
        for(int i=0;i<4;++i) game::put_u32(record.data()+i*4,vtables[i]);
        game::put_u32(record.data()+152,0x7e4dd8);
        // Items is rebuilt on Load. The original pointer bytes have no meaning
        // in a portable record and are intentionally zero, not truncated.
        game::put_u32(record.data()+160,HouseTypes.Capacity);
        record[164]=HouseTypes.IsInitialized; record[165]=HouseTypes.IsAllocated;
        game::put_u32(record.data()+168,HouseTypes.Count);
        game::put_u32(record.data()+172,HouseTypes.CapacityIncrement);
        game::put_u32(record.data()+176,HouseTypes.unknown_18);
        auto hr=game::write_type_record(stream,*this,record,clear); if(hr<0)return hr;
        hr=game::write_type_u32(stream,HouseTypes.Count); if(hr<0)return hr;
        for(int i=0;i<HouseTypes.Count;++i) {
            hr=game::write_type_u32(stream,HouseTypes[i]); if(hr<0)return hr;
        }
        return 0;
    } catch (...) { return game::type_stream_error(game::stream_failure); }
}
HRESULT YRPP_STDCALL SideClass::Load(IStream* stream) {
    try {
        std::array<unsigned char,180> record{}; std::uint32_t token=0,count=0;
        auto hr=game::read_type_record(stream,record,token); if(hr<0)return hr;
        hr=game::read_type_u32(stream,count); if(hr<0)return hr;
        if(count>game::type_stream_max_elements()) return game::type_stream_error(game::stream_invalid_record);
        TypeList<int> countries; // original reconstructs increment=10, ignores serialized pointer/capacity
        countries.unknown_18=game::get_i32(record.data()+176);
        for(std::uint32_t i=0;i<count;++i) {
            std::uint32_t value=0; hr=game::read_type_u32(stream,value); if(hr<0)return hr;
            if(!countries.AddItem(std::bit_cast<std::int32_t>(value))) return game::type_stream_error(game::stream_failure);
        }
        hr=game::type_stream_load_token(token,this); if(hr<0)return hr;
        if(!DecodeTypeRecordBase(record.data(),record.size())) return game::type_stream_error(game::stream_invalid_record);
        HouseTypes.Swap(countries);
        return 0;
    } catch (...) { return game::type_stream_error(game::stream_failure); }
}
