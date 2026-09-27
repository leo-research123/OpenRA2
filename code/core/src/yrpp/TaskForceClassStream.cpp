// Supplied 006E86A0/006E8680: six references are swizzled, including inactive slots.
#include "yrpp/TaskForceClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "type_stream.hpp"
HRESULT YRPP_STDCALL TaskForceClass::Save(IStream* stream, BOOL clear) {
    if(!stream) return game::type_stream_error(game::stream_pointer_error);
    try {
        if(CountEntries<0 || CountEntries>6) return game::type_stream_error(game::stream_invalid_record);
        std::array<unsigned char,212> record{};
        if(!EncodeTypeRecordBase(record.data(),record.size())) return game::type_stream_error(game::stream_invalid_record);
        constexpr std::uint32_t vtables[]{0x7f4680,0x7f4664,0x7f465c,0x7f4654};
        for(int i=0;i<4;++i) game::put_u32(record.data()+i*4,vtables[i]);
        game::put_u32(record.data()+152,Group); game::put_u32(record.data()+156,CountEntries); game::put_u32(record.data()+160,IsGlobal);
        for(int i=0;i<6;++i) {
            std::uint32_t token=0; auto hr=game::type_stream_save_token(Entries[i].Type,token); if(hr<0)return hr;
            game::put_u32(record.data()+164+i*8,Entries[i].Amount);
            game::put_u32(record.data()+168+i*8,token);
        }
        auto hr=game::write_type_record(stream,*this,record,clear); return hr<0 ? hr : 0;
    } catch (...) { return game::type_stream_error(game::stream_failure); }
}
HRESULT YRPP_STDCALL TaskForceClass::Load(IStream* stream) {
    try {
        std::array<unsigned char,212> record{}; std::uint32_t token=0;
        auto hr=game::read_type_record(stream,record,token); if(hr<0)return hr;
        const int count=game::get_i32(record.data()+156);
        if(count<0 || count>6 || game::get_u32(record.data()+160)>1) return game::type_stream_error(game::stream_invalid_record);
        hr=game::type_stream_load_token(token,this); if(hr<0)return hr;
        std::uint32_t references[6]; TechnoTypeClass** slots[6];
        for(int i=0;i<6;++i) { references[i]=game::get_u32(record.data()+168+i*8); slots[i]=&Entries[i].Type; }
        hr=game::queue_techno_references(slots,references,6); if(hr<0)return hr;
        if(!DecodeTypeRecordBase(record.data(),record.size())) return game::type_stream_error(game::stream_invalid_record);
        Group=game::get_i32(record.data()+152); CountEntries=count; IsGlobal=game::get_i32(record.data()+160);
        for(int i=0;i<6;++i) Entries[i].Amount=game::get_i32(record.data()+164+i*8);
        return 0;
    } catch (...) { return game::type_stream_error(game::stream_failure); }
}
