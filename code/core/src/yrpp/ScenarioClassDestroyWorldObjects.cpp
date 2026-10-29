/*
 * Object teardown follows EA REDALERT/SCENARIO.CPP::Clear_Scenario, revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc.; GPL-3.0-or-later, third_party/ea/LICENSE.TXT. YR 534450..5349B1
 * supplies the target index, collection order and deferred-expiration passes.
 */
#include "yrpp/ScenarioClass.h"
#include "yrpp/AbstractClass.h"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/BombClass.h"
#include "yrpp/BuildingLightClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/DiskLaserClass.h"
#include "yrpp/RadBeam.h"
#include "yrpp/EBolt.h"
#include "yrpp/EMPulseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/RadSiteClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/Unsorted.h"
// WaveClass.h relies on its includer for these.
#include "yrpp/CellClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/WaveClass.h"
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace {
int add(int value, unsigned amount) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) + amount); }
struct DepthScope {
    int& value;
    bool active = true;
    explicit DepthScope(int& depth) : value(depth) { value = add(value, 1); }
    int finish() {
        if (active) {
            value = add(value, 0xffffffffu);
            active = false;
        }
        return value;
    }
    ~DepthScope() { finish(); }
};
// Each pass deletes the first live entry through its virtual destructor until
// the collection is empty; destructors unregister themselves. A null first
// entry is skipped without progress in the original, which would never end.
template <class T>
void destroy(DynamicVectorClass<T*>& objects) {
    while (objects.Count > 0) {
        auto* object = objects.Items[0];
        if (!object) throw std::logic_error("Scenario destruction found an empty live object slot");
        delete static_cast<AbstractClass*>(object);
    }
    AbstractClass::RemoveAllInactive();
}
}

int YRPP_FASTCALL ScenarioClass::DestroyWorldObjects() {
    DepthScope depth(Unsorted::ScenarioInit);
    // 4E0410 preserves borrowed Items and IsInitialized, like the inline slot
    // array destruction in ClearWorld. Do not reset the object's vtable.
    auto& notices = AbstractClass::PendingDeletes;
    notices.Count = 0;
    if (notices.Items && notices.IsAllocated) {
        DLLDeleteArray(notices.Items, static_cast<size_t>(notices.Capacity));
        notices.Items = nullptr;
    }
    notices.IsAllocated = false;
    notices.Capacity = 0;
    // 0x69E210 releases shapes loaded into the original's file cache. Native
    // shape data belongs to the resource owner and outlives the scenario.
    auto& targets = AbstractClass::TargetIndex;
    while (targets.Count() > 0) {
        if (!targets.IndexTable) throw std::logic_error("Scenario destruction found an invalid target index");
        // The original comparator uses signed ID order; Sort also preserves
        // the calibrated CRT tie ordering and clears Archive only if unsorted.
        targets.Sort();
        auto* object = targets.IndexTable[0].Data;
        if (!object) throw std::logic_error("Scenario destruction found an empty live target");
        delete object;
    }
    AbstractClass::RemoveAllInactive();
    while (BulletClass::Array.Count > 0) {
        auto* bullet = BulletClass::Array.Items[0];
        if (!bullet) throw std::logic_error("Scenario destruction found an empty live bullet slot");
        bullet->Release();
    }
    AbstractClass::RemoveAllInactive();
    destroy(ObjectClass::Array);
    destroy(TagClass::Array);
    destroy(TriggerClass::Array);
    destroy(TubeClass::Array);
    destroy(BuildingLightClass::Array);
    destroy(OverlayClass::Array);
    destroy(ParticleSystemClass::Array);
    destroy(WaveClass::Array);
    destroy(FactoryClass::Array);
    destroy(SideClass::Array);
    destroy(TeamClass::Array);
    destroy(HouseClass::Array);
    destroy(AnimClass::Array);
    destroy(ScriptClass::Array);
    destroy(RadSiteClass::Array);
    destroy(LightSourceClass::Array);
    destroy(EMPulseClass::Array);
    destroy(CaptureManagerClass::Array);
    destroy(DiskLaserClass::Array);
    RadBeam::DeleteAll();
    destroy(ParasiteClass::Array);
    destroy(TemporalClass::Array);
    destroy(AirstrikeClass::Array);
    destroy(SpawnManagerClass::Array);
    destroy(BombClass::Array);
    // Spotlight has a nonvirtual destructor (0x5FF2D0).
    while (SpotlightClass::Array.Count > 0) {
        auto* spotlight = SpotlightClass::Array.Items[0];
        if (!spotlight) throw std::logic_error("Scenario destruction found an empty live spotlight slot");
        delete spotlight;
    }
    AbstractClass::RemoveAllInactive();
    destroy(FoggedObjectClass::Array);
    destroy(AlphaShapeClass::Array);
    destroy(TerrainClass::Array);
    EBolt::DeleteAll();
    LineTrail::DeleteAll();
    LaserDrawClass::DeleteAll();
    destroy(AbstractTypeClass::Array);
    if (TacticalClass::Instance) delete TacticalClass::Instance;
    TacticalClass::Instance = nullptr;
    MapClass::Instance.MapClass::DestructCells();
    AbstractClass::RemoveAllInactive();
    BeaconManagerClass::Instance.Reset();
    return depth.finish();
}
