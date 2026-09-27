// Supplied 00691D90/00691DE0; 564-byte x86 record plus a 32-bit object token.
#include "yrpp/ScriptTypeClass.h"
#include "type_stream.hpp"
HRESULT YRPP_STDCALL ScriptTypeClass::Save(IStream* stream, BOOL clear) {
    if(!stream) return game::type_stream_error(game::stream_pointer_error);
    try {
        if(ActionsCount<0 || ActionsCount>50) return game::type_stream_error(game::stream_invalid_record);
        std::array<unsigned char,564> record{};
        if(!EncodeTypeRecordBase(record.data(),record.size())) return game::type_stream_error(game::stream_invalid_record);
        constexpr std::uint32_t vtables[]{0x7f1008,0x7f0fec,0x7f0fe4,0x7f0fdc};
        for(int i=0;i<4;++i) game::put_u32(record.data()+i*4,vtables[i]);
        game::put_u32(record.data()+152,ArrayIndex); game::put_u32(record.data()+156,IsGlobal);
        game::put_u32(record.data()+160,ActionsCount);
        for(int i=0;i<50;++i) {
            game::put_u32(record.data()+164+i*8,ScriptActions[i].Action);
            game::put_u32(record.data()+168+i*8,ScriptActions[i].Argument);
        }
        auto hr=game::write_type_record(stream,*this,record,clear); return hr<0 ? hr : 0;
    } catch (...) { return game::type_stream_error(game::stream_failure); }
}
HRESULT YRPP_STDCALL ScriptTypeClass::Load(IStream* stream) {
    try {
        std::array<unsigned char,564> record{}; std::uint32_t token=0;
        auto hr=game::read_type_record(stream,record,token); if(hr<0)return hr;
        const int count=game::get_i32(record.data()+160);
        if(game::get_u32(record.data()+156)>1 || count<0 || count>50) return game::type_stream_error(game::stream_invalid_record);
        hr=game::type_stream_load_token(token,this); if(hr<0)return hr;
        if(!DecodeTypeRecordBase(record.data(),record.size())) return game::type_stream_error(game::stream_invalid_record);
        ArrayIndex=game::get_i32(record.data()+152); IsGlobal=game::get_i32(record.data()+156); ActionsCount=count;
        for(int i=0;i<50;++i) {
            ScriptActions[i].Action=game::get_i32(record.data()+164+i*8);
            ScriptActions[i].Argument=game::get_i32(record.data()+168+i*8);
        }
        return 0;
    } catch (...) { return game::type_stream_error(game::stream_failure); }
}
