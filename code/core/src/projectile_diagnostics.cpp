#include "api/projectile_diagnostics.hpp"
#include "projectile_diagnostics.hpp"
#include "yrpp/BulletClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/Unsorted.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <filesystem>

namespace game {
struct ProjectileOrigin {
    const BulletClass* bullet=nullptr;
    int bullet_id=0,source_id=-1,house=-1,frame=0,target_id=-1;
    char type[32]="<none>",target_type[32]="<none>";
};
struct ProjectileLogState {
    std::mutex mutex;
    std::FILE* file=nullptr;
    char path[4096]{},previous[4112]{};
    std::uint64_t sequence=0;
    std::size_t bytes=0;
    // Fixed diagnostic cache, no allocation in simulation hooks. A collision
    // reports origin_known=0 instead of attributing another projectile's owner.
    ProjectileOrigin origins[8192]{};
    ~ProjectileLogState(){if(file)std::fclose(file);}
};
static ProjectileLogState projectile_log;
static bool rotate_projectile_log() noexcept {
    if(projectile_log.file){std::fclose(projectile_log.file);projectile_log.file=nullptr;}
    // Keep the current log intact and disable output if rotation fails.
    if(auto* existing=std::fopen(projectile_log.path,"rb")){
        std::fclose(existing);std::remove(projectile_log.previous);
        if(std::rename(projectile_log.path,projectile_log.previous)!=0)return false;
    }
    projectile_log.file=std::fopen(projectile_log.path,"wb");projectile_log.bytes=0;
    return projectile_log.file!=nullptr;
}
bool configure_projectile_log(const char* path) noexcept {
    try {
        const std::lock_guard lock(projectile_log.mutex);
        if(projectile_log.file){std::fclose(projectile_log.file);projectile_log.file=nullptr;}
        if(!path)return true;
        if(!*path||std::strlen(path)>=sizeof(projectile_log.path))return false;
        const auto parent=std::filesystem::u8path(path).parent_path();std::error_code error;
        if(!parent.empty())std::filesystem::create_directories(parent,error);
        if(error)return false;
        std::snprintf(projectile_log.path,sizeof(projectile_log.path),"%s",path);
        std::snprintf(projectile_log.previous,sizeof(projectile_log.previous),"%s.previous",path);
        for(auto& origin:projectile_log.origins)origin=ProjectileOrigin{};
        projectile_log.sequence=0;
        if(!rotate_projectile_log())return false;
        const char* header="RA2 projectile diagnostics v1; source metadata captured at construction; events flushed; max_bytes=8388608\n";
        if(std::fputs(header,projectile_log.file)<0||std::fflush(projectile_log.file)!=0){std::fclose(projectile_log.file);projectile_log.file=nullptr;return false;}
        projectile_log.bytes=std::strlen(header);return true;
    }catch(...){return false;}
}
static void write_projectile_event(const BulletClass& bullet,const char* event,const char* detail) noexcept {
    if(!projectile_log.file)return;
    const auto& saved=projectile_log.origins[unsigned(bullet.UniqueID)%8192];
    const bool known=saved.bullet==&bullet&&saved.bullet_id==bullet.UniqueID;
    const auto* type=bullet.Type;const auto* owner=bullet.Owner;
    char line[2048];
    const int size=std::snprintf(line,sizeof(line),
        "seq=%llu frame=%d event=%s bullet_id=%d projectile=%.24s weapon=%.24s origin_known=%d source_id=%d source_type=%.24s source_house=%d created_frame=%d initial_target_id=%d initial_target_type=%.24s "
        "owner_id=%d owner_alive=%d owner_hp=%d owner_disguised=%d target_id=%d "
        "pos=%d,%d,%d source_pos=%d,%d,%d target_pos=%d,%d,%d velocity=%.9g,%.9g,%.9g "
        "rot=%d inviso=%d arcing=%d vertical=%d dropping=%d elasticity=%.9g %s\n",
        static_cast<unsigned long long>(++projectile_log.sequence),Unsorted::CurrentFrame,event,int(bullet.UniqueID),type?type->ID:"<none>",
        bullet.WeaponType?bullet.WeaponType->ID:"<none>",known,known?saved.source_id:-1,known?saved.type:"<unknown>",known?saved.house:-1,known?saved.frame:-1,known?saved.target_id:-1,known?saved.target_type:"<unknown>",
        owner?int(owner->UniqueID):-1,owner?int(owner->IsAlive):0,owner?owner->Health:0,owner?int(owner->Disguised):0,bullet.Target?int(bullet.Target->UniqueID):-1,
        bullet.Location.X,bullet.Location.Y,bullet.Location.Z,bullet.SourceCoords.X,bullet.SourceCoords.Y,bullet.SourceCoords.Z,
        bullet.TargetCoords.X,bullet.TargetCoords.Y,bullet.TargetCoords.Z,bullet.Velocity.X,bullet.Velocity.Y,bullet.Velocity.Z,
        type?type->ROT:0,type?int(type->Inviso):0,type?int(type->Arcing):0,type?int(type->Vertical):0,type?int(type->Dropping):0,type?type->Elasticity:0.0,detail);
    if(size<=0)return;
    const auto count=std::size_t(size)<sizeof(line)?std::size_t(size):sizeof(line)-1;
    if(projectile_log.bytes+count>8u*1024u*1024u&&!rotate_projectile_log())return;
    if(std::fwrite(line,1,count,projectile_log.file)!=count||std::fflush(projectile_log.file)!=0){
        std::fclose(projectile_log.file);projectile_log.file=nullptr;return;
    }
    projectile_log.bytes+=count;
}
void projectile_log_created(const BulletClass& bullet) noexcept {
    try {
        const std::lock_guard lock(projectile_log.mutex);if(!projectile_log.file)return;
        auto& saved=projectile_log.origins[unsigned(bullet.UniqueID)%8192];saved=ProjectileOrigin{};
        saved.bullet=&bullet;saved.bullet_id=bullet.UniqueID;saved.frame=Unsorted::CurrentFrame;
        if(const auto* owner=bullet.Owner){
            saved.source_id=owner->UniqueID;saved.house=owner->Owner?owner->Owner->ArrayIndex:-1;
            if(auto* type=owner->GetTechnoType())std::snprintf(saved.type,sizeof(saved.type),"%.24s",type->ID);
        }
        if(bullet.Target){
            saved.target_id=bullet.Target->UniqueID;
            if((bullet.Target->AbstractFlags&::AbstractFlags::Object)!=::AbstractFlags::None){
                if(auto* type=static_cast<const ObjectClass*>(bullet.Target)->GetType())std::snprintf(saved.target_type,sizeof(saved.target_type),"%.24s",type->ID);
            }else std::snprintf(saved.target_type,sizeof(saved.target_type),"<non-object>");
        }
        write_projectile_event(bullet,"created","");
    }catch(...){}
}
void projectile_log_event(const BulletClass& bullet,const char* event) noexcept {
    try {const std::lock_guard lock(projectile_log.mutex);write_projectile_event(bullet,event,"");}catch(...){}
}
void projectile_log_contact(const BulletClass& bullet,int x,int y,int z,int floor,int slope,bool fell,bool rose,bool obstacle) noexcept {
    try {
        const std::lock_guard lock(projectile_log.mutex);
        char detail[256];
        std::snprintf(detail,sizeof(detail),"next_pos=%d,%d,%d floor=%d slope=%d bridge_fell=%d bridge_rose=%d obstacle=%d bounce_without_owner=%d ramp_original_va=0x%llX",
            x,y,z,floor,slope,int(fell),int(rose),int(obstacle),int(!bullet.Owner),
            static_cast<unsigned long long>(0xB45188u+unsigned(slope)*0x30u));
        write_projectile_event(bullet,"physics_contact",detail);
    }catch(...){}
}
}
