// Existing YRpp PlanMgr node/member/branch model, calibrated to YR
// 0x633D30 / 0x633FA0 / 0x6340B0 / 0x6349B0 / 0x634CC0 / 0x634E10 /
// 0x635060 / 0x635120 / 0x6351E0 / 0x635DB0.
// The pinned OpenTS revision has no corresponding PlanMgr module.
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/Unsorted.h"
#include <bit>
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
int add32(int value,int delta) noexcept {
    return std::bit_cast<int>(unsigned(value)+unsigned(delta));
}
template<class T>
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void append(DynamicVectorClass<T*>& vector,T* item) noexcept {
    try { vector.AddItem(item); }
    catch(const std::bad_alloc&) {
        // Original vector growth returns false after restoring validity. Keep
        // the caller's original partial state; it does not roll back the item.
        vector.IsInitialized=true;
    }
}
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void clear(DynamicVectorClass<PlanningBranchClass*>& branches) noexcept { branches.Clear(); }
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void remove_member(DynamicVectorClass<PlanningMemberClass*>& members,PlanningMemberClass* member) noexcept {
    members.Remove(member);
}
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
int find_node(const DynamicVectorClass<PlanningNodeClass*>& nodes,PlanningNodeClass* node) noexcept {
    return nodes.FindItemIndex(node);
}
template<class T> T* allocate() noexcept {
    void* memory=YRMemory::Allocate(sizeof(T));
    if(!memory)std::abort(); // The original immediately dereferences null.
    return ::new(memory) T;
}
}
PlanningNodeClass::~PlanningNodeClass() noexcept {
    ClearBranches();
    Game::PlanningManager_UnregisterNode(this);
}
int PlanningNodeClass::AddMember(TechnoClass* owner,const EventClass* event) noexcept {
    auto* member=allocate<PlanningMemberClass>();
    member->Owner=nullptr;member->Packet=nullptr;member->field_8=-1;member->field_C=0;
    member->Owner=owner;
    void* memory=YRMemory::Allocate(sizeof(EventClass));
    if(!memory)std::abort();
    auto* packet=::new(memory) EventClass(noinit_t{});
    packet->Type=EventType::Empty;*packet=*event;member->Packet=packet;
    append(PlanningMembers,member);
    auto* token=owner->PlanningToken;
    append(token->PlanningNodes,this);
    if(token->field_8C==-1)token->field_8C=token->PlanningNodes.Count-1;
    return AddBranch(member);
}
void PlanningNodeClass::RemoveMember(TechnoClass* owner) noexcept {
    auto* member=FindMember(owner);
    if(!member)return;
    ReleaseBranch(member);
    GameDelete(member->Packet);
    remove_member(PlanningMembers,member);
    GameDelete(member);
    if(!PlanningMembers.Count)GameDelete(this);
}
int PlanningNodeClass::FindBranch(TechnoClass* owner) const noexcept {
    const auto* previous=GetPrevious(owner);
    if(previous==owner->PlanningToken->GetLastNode())return -1;
    const EventClass source(*previous->FindMember(owner)->Packet);
    const int count=PlanningBranches.Count;
    if(count) {
        CoordStruct storage;
        const auto coords=*Game::PlanningManager_EventCoords(&storage,&source);
        for(int i=0;i<count;++i) {
            const auto* branch=PlanningBranches[i];
            if(branch->MemberCount) {
                const EventClass packet(branch->Packet);
                if(coords==*Game::PlanningManager_EventCoords(&storage,&packet))return i;
            }
        }
    }
    return -1;
}
int PlanningNodeClass::AddBranch(PlanningMemberClass* member) noexcept {
    auto* owner=member->Owner;
    int index=FindBranch(owner);
    if(index!=-1) {
        auto* branch=PlanningBranches[index];
        branch->field_74=-1;field_B4=-1;
        branch->MemberCount=add32(branch->MemberCount,1);
        member->field_8=index;return index;
    }
    const auto* previous=GetPrevious(owner);
    if(previous==owner->PlanningToken->GetLastNode()) {
        member->field_8=-1;return -1;
    }
    const EventClass source(*previous->FindMember(owner)->Packet);
    auto* branch=allocate<PlanningBranchClass>();
    branch->Packet=source;branch->MemberCount=add32(branch->MemberCount,1);
    append(PlanningBranches,branch);
    index=PlanningBranches.Count-1;
    if(CommonBranch.MemberCount>0 && field_B0==-1) {
        field_B0=-1;
        if(CommonBranch.MemberCount) {
            const EventClass common(CommonBranch.Packet);
            const int count=PlanningBranches.Count;
            for(int i=0;i<count;++i) {
                // The original snapshots the branch event before querying the
                // common event, and queries both coordinates on every step.
                const EventClass packet(PlanningBranches[i]->Packet);
                CoordStruct a,b;
                const auto first=*Game::PlanningManager_EventCoords(&a,&common);
                const auto second=*Game::PlanningManager_EventCoords(&b,&packet);
                if(first==second) {field_B0=i;break;}
            }
        }
    }
    field_B4=-1;member->field_8=index;return index;
}
int PlanningNodeClass::ReleaseLoopBranch(PlanningMemberClass* member) noexcept {
    member->field_C=0;
    CommonBranch.MemberCount=add32(CommonBranch.MemberCount,-1);
    if(!CommonBranch.MemberCount)field_B0=-1;
    return CommonBranch.MemberCount;
}
void PlanningNodeClass::ReleaseBranch(PlanningMemberClass* member) noexcept {
    if(member->field_C)ReleaseLoopBranch(member);
    if(member->field_8!=-1) {
        auto* branch=PlanningBranches[member->field_8];
        branch->field_74=-1;field_B4=-1;
        branch->MemberCount=add32(branch->MemberCount,-1);
        if(!branch->MemberCount)Game::PlanningManager_FlagNode(this);
    }
    member->field_8=-1;
}
void PlanningNodeClass::ClearBranches() noexcept {
    const int count=PlanningBranches.Count;
    for(int i=0;i<count;++i)GameDelete(PlanningBranches[i]);
    clear(PlanningBranches);
    const int members=PlanningMembers.Count;
    for(int i=0;i<members;++i) {
        PlanningMembers[i]->field_8=-1;PlanningMembers[i]->field_C=0;
    }
    CommonBranch.MemberCount=0;field_B0=-1;
}
void PlanningNodeClass::InvalidateDisplay(bool branches) noexcept {
    field_B4=-1;
    if(branches) {
        const int count=PlanningBranches.Count;
        for(int i=0;i<count;++i)PlanningBranches[i]->field_74=-1;
        CommonBranch.field_74=-1;
    }
}
void PlanningNodeClass::UpdateLoopBranch() noexcept {
    const int members=PlanningMembers.Count;
    for(int i=0;i<members;++i) {
        auto* member=PlanningMembers[i];auto* owner=member->Owner;
        const auto* token=owner->PlanningToken;
        auto* self=this;
        if(find_node(token->PlanningNodes,self)==token->StepsToClosedLoop) {
            member->field_C=1;CommonBranch.MemberCount=add32(CommonBranch.MemberCount,1);
            if(CommonBranch.MemberCount==1)
                CommonBranch.Packet=*owner->PlanningToken->GetLastNode()->FindMember(owner)->Packet;
        }
    }
    field_B0=-1;
    if(CommonBranch.MemberCount) {
        const EventClass common(CommonBranch.Packet);
        const int count=PlanningBranches.Count;
        for(int i=0;i<count;++i) {
            const EventClass packet(PlanningBranches[i]->Packet);
            CoordStruct a,b;
            const auto first=*Game::PlanningManager_EventCoords(&a,&common);
            const auto second=*Game::PlanningManager_EventCoords(&b,&packet);
            if(first==second){field_B0=i;break;}
        }
    }
}
#endif
