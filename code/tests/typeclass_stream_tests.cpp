#include "support/test_support.hpp"
#include "support/type_stream_fixture.hpp"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include <vector>
#include <limits>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <algorithm>
namespace {

using MemoryStream = ra2::test::MemoryStream<>;
using Session = ra2::test::StreamSession<MemoryStream>;
std::uint32_t word(const MemoryStream& stream,std::size_t offset) {
    const auto* p=stream.bytes.data()+offset;
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
struct IoCall { AbstractClass* object; MemoryStream* stream; bool save; BOOL clear=0; HRESULT result=-1; };
game::TypeStreamStatus call(Session& session,IoCall& io) {
    return game::with_type_stream(*session.value,[](void* raw) {
        auto& io=*static_cast<IoCall*>(raw);
        io.result=io.save ? io.object->Save(io.stream->handle(),io.clear) : io.object->Load(io.stream->handle());
    },&io);
}
void session_lifetimes() {
    Session outer, inner;
    struct Pair { Session* outer; Session* inner; } pair{&outer,&inner};
    const auto status=game::with_type_stream(*outer.value,[](void* raw) {
        auto& pair=*static_cast<Pair*>(raw);
        const auto inner_result=game::with_type_stream(*pair.inner->value,[](void* context) {
            auto& nested=*static_cast<Pair*>(context);
            game::destroy_type_stream_session(nested.outer->value);
            EXPECT_TRUE((nested.outer->value!=nullptr)) << "cannot destroy a borrowed outer session";
        },&pair);
        EXPECT_TRUE((inner_result==game::TypeStreamStatus::complete)) << "inner session is not the destroy target";
    },&pair);
    EXPECT_TRUE((status==game::TypeStreamStatus::failure)) << "outer borrowed-destroy attempt is reported";
    game::destroy_type_stream_session(outer.value);
    EXPECT_TRUE((!outer.value)) << "session can be destroyed after scope exit";
}
void roundtrip() {
    ScriptTypeClass original("SCRIPT"); original.ActionsCount=2; original.IsGlobal=true;
    original.ScriptActions[0]={18,7}; original.ScriptActions[1]={3,-5}; original.ScriptActions[49]={41,-91};
    original.UniqueID=0x12345678; original.RefCount=11; original.Dirty=true;
    std::strcpy(original.Name,"saved script");
    SideClass side("SIDE"); side.HouseTypes.AddItem(0); side.HouseTypes.AddItem(2); side.HouseTypes.AddItem(-1);
    side.HouseTypes.CapacityIncrement=42; side.HouseTypes.unknown_18=9;
    InfantryTypeClass infantry("GI"); UnitTypeClass unit("TANK");
    TaskForceClass force("FORCE"); force.Group=7; force.CountEntries=2; force.IsGlobal=true;
    force.Entries[0]={2,&infantry}; force.Entries[1]={3,&unit}; force.Entries[5]={77,&infantry};
    Session saving; MemoryStream script_data,side_data,task_data;
    IoCall script_save{&original,&script_data,true,1},side_save{&side,&side_data,true},task_save{&force,&task_data,true};
    EXPECT_TRUE((call(saving,script_save)==game::TypeStreamStatus::complete && script_save.result==0)) << "script save";
    EXPECT_TRUE((!original.Dirty && script_data.bytes.size()==568)) << "fixed x86 size and successful Dirty clear";
    EXPECT_TRUE((word(script_data,4)==0x7f1008 && word(script_data,20)==0x12345678 && script_data.bytes[36]==1)) << "target vtable value, UniqueID and pre-clear Dirty encoded at exact offsets";
    EXPECT_TRUE((word(script_data,100)==0)) << "UIName is not a native pointer";
    EXPECT_TRUE((call(saving,side_save)==game::TypeStreamStatus::complete && side_save.result==0 &&
        side_data.bytes.size()==200)) << "side record plus count and three 32-bit values";
    EXPECT_TRUE((call(saving,task_save)==game::TypeStreamStatus::complete && task_save.result==0 &&
        task_data.bytes.size()==216)) << "task record target size, independent of host pointers";
    std::uint32_t infantry_token=0,unit_token=0;
    EXPECT_TRUE((game::type_stream_reference_id(*saving.value,&infantry,infantry_token)==game::TypeStreamStatus::complete)) << "stable reference id";
    EXPECT_TRUE((game::type_stream_reference_id(*saving.value,&unit,unit_token)==game::TypeStreamStatus::complete)) << "second reference id";
    EXPECT_TRUE((infantry_token && infantry_token<100 && word(task_data,172)==infantry_token &&
        word(task_data,212)==infantry_token && infantry_token!=unit_token)) << "six-slot encoding uses session tokens";
    ScriptTypeClass loaded("NEW_SCRIPT"); SideClass loaded_side("NEW_SIDE"); TaskForceClass loaded_force("NEW_FORCE");
    loaded.RefCount=37;
    loaded_side.HouseTypes.AddItem(99); loaded_side.HouseTypes.CapacityIncrement=99;
    const int registry_count=AbstractTypeClass::Array.Count;
    Session loading;
    IoCall script_load{&loaded,&script_data,false},side_load{&loaded_side,&side_data,false},task_load{&loaded_force,&task_data,false};
    EXPECT_TRUE((call(loading,script_load)==game::TypeStreamStatus::complete && script_load.result==0)) << "script load";
    EXPECT_TRUE((loaded.RefCount==37 && loaded.UniqueID==original.UniqueID && loaded.Dirty &&
        loaded.ScriptActions[49].Argument==-91 && loaded.ActionsCount==2 && loaded.IsGlobal)) << "preserved refcount, full action slots";
    EXPECT_TRUE((!std::strcmp(loaded.Name,"saved script") && loaded.WhatAmI()==AbstractType::ScriptType)) << "local vtable and restored base name";
    EXPECT_TRUE((call(loading,side_load)==game::TypeStreamStatus::complete && loaded_side.HouseTypes.Count==3 &&
        loaded_side.HouseTypes[2]==-1 && loaded_side.HouseTypes.CapacityIncrement==10 && loaded_side.HouseTypes.unknown_18==9)) << "side reconstructs owning container instead of restoring native bytes";
    EXPECT_TRUE((call(loading,task_load)==game::TypeStreamStatus::complete && loaded_force.CountEntries==2 &&
        loaded_force.Group==7 && !loaded_force.Entries[0].Type)) << "load defers referenced type pointers";
    std::uint32_t unresolved=0;
    EXPECT_TRUE((game::resolve_type_stream_references(*loading.value,unresolved)==game::TypeStreamStatus::unresolved_references &&
        unresolved==3)) << "all active and inactive non-null slots await relocation";
    EXPECT_TRUE((game::bind_type_stream_reference(*loading.value,infantry_token,&infantry)==game::TypeStreamStatus::complete)) << "bind actual normally constructed InfantryType";
    EXPECT_TRUE((game::resolve_type_stream_references(*loading.value,unresolved)==game::TypeStreamStatus::unresolved_references &&
        unresolved==1 && !loaded_force.Entries[0].Type)) << "incomplete graph is not partially published";
    EXPECT_TRUE((game::bind_type_stream_reference(*loading.value,unit_token,&unit)==game::TypeStreamStatus::complete)) << "bind actual normally constructed UnitType";
    EXPECT_TRUE((game::resolve_type_stream_references(*loading.value,unresolved)==game::TypeStreamStatus::complete &&
        !unresolved && loaded_force.Entries[0].Type==&infantry && loaded_force.Entries[1].Type==&unit &&
        loaded_force.Entries[5].Type==&infantry)) << "relocation of shared and inactive references";
    EXPECT_TRUE((AbstractTypeClass::Array.Count==registry_count)) << "Load does not duplicate registration or invoke noinit construction";
    // Failure semantics are explicit, rather than silently accepting a short read.
    MemoryStream short_data=script_data; short_data.cursor=0; short_data.bytes.pop_back();
    Session broken; ScriptTypeClass unchanged("UNCHANGED"); unchanged.ActionsCount=12;
    IoCall broken_load{&unchanged,&short_data,false};
    EXPECT_TRUE((call(broken,broken_load)==game::TypeStreamStatus::failure && broken_load.result<0 &&
        unchanged.ActionsCount==12 && !std::strcmp(unchanged.ID,"UNCHANGED"))) << "short read fails without clobbering local object";
    original.Dirty=true; MemoryStream short_write; short_write.limit=2;
    IoCall failed_save{&original,&short_write,true,1};
    EXPECT_TRUE((call(broken,failed_save)==game::TypeStreamStatus::failure && original.Dirty)) << "failed record write retains Dirty";
    // A class-incompatible binding cannot be cast into a TechnoType pointer.
    task_data.cursor=0; Session wrong; TaskForceClass wrong_force("WRONG"); IoCall wrong_load{&wrong_force,&task_data,false};
    EXPECT_TRUE((call(wrong,wrong_load)==game::TypeStreamStatus::complete)) << "wrong-type fixture load";
    EXPECT_TRUE((game::bind_type_stream_reference(*wrong.value,infantry_token,&original)==game::TypeStreamStatus::complete)) << "bind exists but has wrong type";
    EXPECT_TRUE((game::resolve_type_stream_references(*wrong.value,unresolved)==game::TypeStreamStatus::failure &&
        !wrong_force.Entries[0].Type)) << "wrong type is rejected before any pointer write";
    game::cancel_type_stream_references(*wrong.value);
    EXPECT_TRUE((original.Load(nullptr)<0 && original.Save(nullptr,0)<0)) << "null IStream contract";
}
}

TEST(TypeclassStream, Contracts) {
    session_lifetimes(); roundtrip();
}
