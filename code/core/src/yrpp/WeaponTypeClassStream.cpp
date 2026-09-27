// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// Field/ownership inventory: OpenTS 44fac744 weapon.cpp::Serialize.
// Modified for YR 0x00772CD0 / 0x00772EB0: 0x160 record, external lists and ISwizzle.
// OpenTS's per-member stream is NOT the YR wire format.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/StringTable.h"
#include "type_stream.hpp"
#if defined(RA2_YRPP_GAME)
#include "yrpp/SwizzleManagerClass.h"
#endif
#include <array>
#include <vector>
#include <limits>

namespace {
struct IntegerField { int WeaponTypeClass::* member; unsigned offset; };
constexpr IntegerField integers[]{
    {&WeaponTypeClass::AmbientDamage,0x98},{&WeaponTypeClass::Burst,0x9C},
    {&WeaponTypeClass::Damage,0xA4},{&WeaponTypeClass::Speed,0xA8},{&WeaponTypeClass::ROF,0xB0},
    {&WeaponTypeClass::Range,0xB4},{&WeaponTypeClass::MinimumRange,0xB8},
    {&WeaponTypeClass::DisguiseFakeBlinkTime,0x13C},{&WeaponTypeClass::RadLevel,0x158}};
struct BoolField { bool WeaponTypeClass::* member; unsigned offset; };
constexpr BoolField booleans[]{
#define FIELD(name,offset) {&WeaponTypeClass::name,offset}
    FIELD(UseFireParticles,0x129),FIELD(UseSparkParticles,0x12A),FIELD(OmniFire,0x12B),
    FIELD(DistributedWeaponFire,0x12C),FIELD(IsRailgun,0x12D),FIELD(Lobber,0x12E),FIELD(Bright,0x12F),
    FIELD(IsSonic,0x130),FIELD(Spawner,0x131),FIELD(LimboLaunch,0x132),FIELD(DecloakToFire,0x133),
    FIELD(CellRangefinding,0x134),FIELD(FireOnce,0x135),FIELD(NeverUse,0x136),FIELD(RevealOnFire,0x137),
    FIELD(TerrainFire,0x138),FIELD(SabotageCursor,0x139),FIELD(MigAttackCursor,0x13A),FIELD(DisguiseFireOnly,0x13B),
    FIELD(InfiniteMindControl,0x140),FIELD(FireWhileMoving,0x141),FIELD(DrainWeapon,0x142),FIELD(FireInTransport,0x143),
    FIELD(Suicide,0x144),FIELD(TurboBoost,0x145),FIELD(Supress,0x146),FIELD(Camera,0x147),FIELD(Charges,0x148),
    FIELD(IsLaser,0x149),FIELD(DiskLaser,0x14A),FIELD(IsLine,0x14B),FIELD(IsBigLaser,0x14C),FIELD(IsHouseColor,0x14D),
    FIELD(IonSensitive,0x14F),FIELD(AreaFire,0x150),FIELD(IsElectricBolt,0x151),FIELD(DrawBoltAsLaser,0x152),
    FIELD(IsAlternateColor,0x153),FIELD(IsRadBeam,0x154),FIELD(IsRadEruption,0x155),FIELD(IsMagBeam,0x15C)
#undef FIELD
};
HRESULT error(HRESULT hr) noexcept {
#if defined(RA2_YRPP_GAME)
    return hr;
#else
    return game::type_stream_error(hr);
#endif
}
HRESULT transfer(IStream* stream,void* bytes,unsigned size,bool save) {
    if (!stream) return error(game::stream_pointer_error);
#if defined(RA2_YRPP_GAME)
    ULONG done=0;
    auto hr=save?stream->Write(bytes,size,&done):stream->Read(bytes,size,&done);
    if (hr<0) return hr;
    return done==size?0:static_cast<HRESULT>(save?0x8003001Du:0x8003001Eu);
#else
    return save?game::write_type_bytes(stream,bytes,size):game::read_type_bytes(stream,bytes,size);
#endif
}
HRESULT word(IStream* stream,std::uint32_t& value,bool save) {
    unsigned char bytes[4]{};
    if (save) game::put_u32(bytes,value);
    const auto hr=transfer(stream,bytes,4,save);
    if (hr>=0 && !save) value=game::get_u32(bytes);
    return hr;
}
unsigned limit() noexcept {
#if defined(RA2_YRPP_GAME)
    return 1048576;
#else
    return game::type_stream_max_elements();
#endif
}
HRESULT token(const AbstractClass* object,std::uint32_t& result) noexcept {
#if defined(RA2_YRPP_GAME)
    static_assert(sizeof(void*)==4);
    result=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(object)); return 0;
#else
    return game::type_stream_save_token(object,result);
#endif
}
template<class T> bool valid_list(const TypeList<T>& list) {
    return list.Count>=0 && static_cast<unsigned>(list.Count)<=limit() &&
        list.Capacity>=list.Count && (!list.Count || list.Items);
}
template<class T> void list_record(unsigned char* p,const TypeList<T>& list,std::uint32_t vtable) {
    game::put_u32(p,vtable); // Items is rebuilt, never interpreted on Load.
    game::put_u32(p+8,list.Capacity);p[12]=list.IsInitialized;p[13]=list.IsAllocated;
    game::put_u32(p+16,list.Count);game::put_u32(p+20,list.CapacityIncrement);game::put_u32(p+24,list.unknown_18);
}
HRESULT read_list(IStream* stream,TypeList<int>& values) {
    std::uint32_t count=0;
    auto hr=word(stream,count,false);if(hr<0)return hr;
    if (count>limit()) return error(game::stream_invalid_record);
    for (std::uint32_t i=0;i<count;++i) {
        std::uint32_t value=0;hr=word(stream,value,false);if(hr<0)return hr;
        if (!values.AddItem(std::bit_cast<std::int32_t>(value))) return error(game::stream_failure);
    }
    return 0;
}
}

HRESULT YRPP_STDCALL WeaponTypeClass::Save(IStream* stream,BOOL clear) {
    if (!stream) return error(game::stream_pointer_error);
    try {
        if (!valid_list(Anim) || !valid_list(Report) || !valid_list(DownReport))
            return error(game::stream_invalid_record);
        std::array<unsigned char,0x160> record{};
        if (!EncodeTypeRecordBase(record.data(),record.size())) return error(game::stream_invalid_record);
        constexpr std::uint32_t tables[]{0x7F73B8,0x7F739C,0x7F7394,0x7F738C};
        for (unsigned i=0;i<4;++i) game::put_u32(record.data()+i*4,tables[i]);
        for (auto field:integers) game::put_u32(record.data()+field.offset,this->*field.member);
        for (auto field:booleans) record[field.offset]=this->*field.member;
        record[0x14E]=static_cast<unsigned char>(LaserDuration);
        const ColorStruct* colors[]{&LaserInnerColor,&LaserOuterColor,&LaserOuterSpread};
        for (unsigned i=0;i<3;++i) {
            record[0x120+i*3]=colors[i]->R;record[0x121+i*3]=colors[i]->G;record[0x122+i*3]=colors[i]->B;
        }
        const AbstractClass* refs[]{Projectile,Warhead,OccupantAnim,AssaultAnim,OpenToppedAnim,AttachedParticleSystem};
        constexpr unsigned offsets[]{0xA0,0xAC,0x110,0x114,0x118,0x11C};
        for (unsigned i=0;i<6;++i) {
            std::uint32_t value=0;auto hr=token(refs[i],value);if(hr<0)return hr;
            game::put_u32(record.data()+offsets[i],value);
        }
        list_record(record.data()+0xBC,Report,0x7E4DD8);
        list_record(record.data()+0xD8,DownReport,0x7E4DD8);
        list_record(record.data()+0xF4,Anim,0x7EB6D4);
        std::vector<std::uint32_t> animations(static_cast<std::size_t>(Anim.Count));
        for (int i=0;i<Anim.Count;++i) { auto hr=token(Anim[i],animations[i]);if(hr<0)return hr; }
        std::uint32_t identity=0;auto hr=token(this,identity);if(hr<0)return hr;
        hr=word(stream,identity,true);if(hr<0)return hr;
        hr=transfer(stream,record.data(),record.size(),true);if(hr<0)return hr;
        // Original base Save clears Dirty before writing the external lists.
        if (clear) Dirty=false;
        std::uint32_t count=static_cast<std::uint32_t>(Anim.Count);
        hr=word(stream,count,true);if(hr<0)return hr;
        for (auto value:animations) { hr=word(stream,value,true);if(hr<0)return hr; }
        for (auto* list:{&Report,&DownReport}) {
            count=static_cast<std::uint32_t>(list->Count);
            hr=word(stream,count,true);if(hr<0)return hr;
            for (int value:*list) {auto bits=static_cast<std::uint32_t>(value);hr=word(stream,bits,true);if(hr<0)return hr;}
        }
        return 0;
    } catch (...) { return error(game::stream_failure); }
}

HRESULT YRPP_STDCALL WeaponTypeClass::Load(IStream* stream) {
    if (!stream) return error(game::stream_pointer_error);
    try {
        std::array<unsigned char,0x160> record{};std::uint32_t identity=0;
        auto hr=word(stream,identity,false);if(hr<0)return hr;
        hr=transfer(stream,record.data(),record.size(),false);if(hr<0)return hr;
        if (!identity || record[0x20]>1 || record[0x3C] || !std::memchr(record.data()+0x3D,0,32) ||
            !std::memchr(record.data()+0x64,0,49)) return error(game::stream_invalid_record);
        for (auto field:booleans) if(record[field.offset]>1) return error(game::stream_invalid_record);
        TypeList<int> animation_ids,reports,down_reports;
        hr=read_list(stream,animation_ids);if(hr<0)return hr;
        hr=read_list(stream,reports);if(hr<0)return hr;
        hr=read_list(stream,down_reports);if(hr<0)return hr;
        TypeList<AnimTypeClass*> animations;
        for (int i=0;i<animation_ids.Count;++i) if(!animations.AddItem(nullptr)) return error(game::stream_failure);
        reports.unknown_18=game::get_i32(record.data()+0xD4);
        down_reports.unknown_18=game::get_i32(record.data()+0xF0);
        animations.unknown_18=game::get_i32(record.data()+0x10C);
        const auto* label=reinterpret_cast<const char*>(record.data()+0x3D);
        const wchar_t* ui_name=*label?StringTable::LoadString(label):L"";
#if !defined(RA2_YRPP_GAME)
        std::vector<game::TypeStreamFixup> refs;
        refs.reserve(static_cast<std::size_t>(animations.Count)+6);
        refs.push_back(game::type_fixup(Projectile,game::get_u32(record.data()+0xA0),this));
        refs.push_back(game::type_fixup(Warhead,game::get_u32(record.data()+0xAC),this));
        refs.push_back(game::type_fixup(OccupantAnim,game::get_u32(record.data()+0x110),this));
        refs.push_back(game::type_fixup(AssaultAnim,game::get_u32(record.data()+0x114),this));
        refs.push_back(game::type_fixup(OpenToppedAnim,game::get_u32(record.data()+0x118),this));
        refs.push_back(game::type_fixup(AttachedParticleSystem,game::get_u32(record.data()+0x11C),this));
        for (int i=0;i<animations.Count;++i)
            refs.push_back(game::type_fixup(animations[i],static_cast<std::uint32_t>(animation_ids[i]),this));
        hr=game::commit_type_references(identity,this,refs.data(),refs.size());if(hr<0)return hr;
#endif
        // Commit is allocation-free: keep live vptrs, RefCount and registrations.
        UniqueID=game::get_u32(record.data()+0x10);
        AbstractFlags=static_cast<::AbstractFlags>(game::get_u32(record.data()+0x14));
        unknown_18=game::get_u32(record.data()+0x18);Dirty=record[0x20]!=0;
        std::memcpy(ID,record.data()+0x24,24);zero_3C=0;
        std::memcpy(UINameLabel,record.data()+0x3D,32);UIName=ui_name;
        std::memcpy(Name,record.data()+0x64,49);
        for(auto field:integers) this->*field.member=game::get_i32(record.data()+field.offset);
        for(auto field:booleans) this->*field.member=record[field.offset]!=0;
        LaserDuration=std::bit_cast<char>(record[0x14E]);
        ColorStruct* colors[]{&LaserInnerColor,&LaserOuterColor,&LaserOuterSpread};
        for(unsigned i=0;i<3;++i) {
            colors[i]->R=record[0x120+i*3];colors[i]->G=record[0x121+i*3];colors[i]->B=record[0x122+i*3];
        }
        Report.Swap(reports);DownReport.Swap(down_reports);Anim.Swap(animations);
#if defined(RA2_YRPP_GAME)
        auto& swizzle=SwizzleManagerClass::Instance;
        swizzle.SwizzleManagerClass::Here_I_Am(std::bit_cast<LONG>(identity),this);
        auto queue=[&]<class T>(T*& field,std::uint32_t value) {
            field=reinterpret_cast<T*>(static_cast<std::uintptr_t>(value));
            return swizzle.SwizzleManagerClass::Swizzle(reinterpret_cast<void**>(&field));
        };
        // Original relocation order: warhead, projectile, particle, assault, occupant, open-topped, list.
        hr=queue(Warhead,game::get_u32(record.data()+0xAC));if(hr<0)return hr;
        hr=queue(Projectile,game::get_u32(record.data()+0xA0));if(hr<0)return hr;
        hr=queue(AttachedParticleSystem,game::get_u32(record.data()+0x11C));if(hr<0)return hr;
        hr=queue(AssaultAnim,game::get_u32(record.data()+0x114));if(hr<0)return hr;
        hr=queue(OccupantAnim,game::get_u32(record.data()+0x110));if(hr<0)return hr;
        hr=queue(OpenToppedAnim,game::get_u32(record.data()+0x118));if(hr<0)return hr;
        for(int i=0;i<Anim.Count;++i) {hr=queue(Anim[i],static_cast<std::uint32_t>(animation_ids[i]));if(hr<0)return hr;}
#endif
        return 0;
    } catch (...) { return error(game::stream_failure); }
}
