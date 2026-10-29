// Original resource metadata, gamemd 410210/410220/4103E0/410450.
#include "yrpp/AbstractClass.h"
#include <cstring>

HRESULT YRPP_STDCALL AbstractClass::QueryInterface(REFIID iid, void** output) {
    // 410260: unlike GScreen, a failed query clears the output. RTTI is the
    // second interface; use the compiler's adjustment on every architecture.
    if (!output) return static_cast<HRESULT>(0x80004003u);
    *output = nullptr;
    DWORD words[4];
    std::memcpy(words, &iid, sizeof(words));
    if (words[1] == 0 && words[2] == 0xc0 && words[3] == 0x46000000 &&
        (words[0] == 0 || words[0] == 0x109 || words[0] == 0x10c))
        *output = static_cast<IPersistStream*>(this);
    else if (words[0] == 0x170dac82 && words[1] == 0x11d212e4 && words[2] == 0x60007581 && words[3] == 0xb55b0508)
        *output = static_cast<IRTTITypeInfo*>(this);
    if (!*output) return static_cast<HRESULT>(0x80004002u);
    try {
        AddRef();
    } catch (...) {
        *output = nullptr;
        return static_cast<HRESULT>(0x80004005u);
    }
    return 0;
}

AbstractType YRPP_STDCALL AbstractClass::What_Am_I() const { return WhatAmI(); }
int YRPP_STDCALL AbstractClass::Fetch_ID() const { return static_cast<int>(UniqueID); }
HRESULT YRPP_STDCALL AbstractClass::IsDirty() { return Dirty ? 0 : 1; }
HRESULT YRPP_STDCALL AbstractClass::GetSizeMax(ULARGE_INTEGER* dest) {
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    dest->QuadPart = static_cast<DWORD>(Size()) + 4u;
    return 0;
}

AbstractClass::AbstractClass() noexcept
    : UniqueID(0xffffffffu), AbstractFlags(static_cast<::AbstractFlags>(0)), unknown_18(0), RefCount(0), Dirty(false) {
    // 410170 only clears the low three flag bits. Native construction initializes
    // all flags: retaining unspecified allocator bytes is not a portable contract.
}
// Keep the x86 ABI's four base-vtable stores observable after this call.
// Optimized end-of-lifetime elimination otherwise removes the 0x4101F0 work.
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma optimize("", off)
#endif
AbstractClass::~AbstractClass() {
#if defined(_MSC_VER) && defined(_M_IX86)
    // Access the representation through character types so base-table setup
    // cannot be omitted as an empty destructor or a dead end-of-life store.
    auto* representation = reinterpret_cast<volatile unsigned char*>(this);
    for (unsigned i = 0; i < 4 * sizeof(void*); ++i) {
        const auto value = representation[i];
        representation[i] = value;
    }
#endif
}
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma optimize("", on)
#endif

// Non-template interface helpers; bodies retained from the corresponding header.

const char* AbstractClass::GetRTTIName() const { return GetRTTIName(WhatAmI()); }

void AbstractClass::AnnounceExpiredPointer(bool removed) { AnnounceExpiredPointer(this, removed); }

CoordStruct AbstractClass::GetCoords() const {
    CoordStruct ret;
    this->GetCoords(&ret);
    return ret;
}

CoordStruct AbstractClass::GetDestination(TechnoClass* pDocker) const {
    CoordStruct ret;
    this->GetDestination(&ret, pDocker);
    return ret;
}

CoordStruct AbstractClass::GetCenterCoords() const {
    CoordStruct ret;
    this->GetCenterCoords(&ret);
    return ret;
}

DirStruct AbstractClass::GetTargetDirection(AbstractClass* pTarget) const {
    DirStruct ret;
    this->GetTargetDirection(&ret, pTarget);
    return ret;
}

bool AbstractClass::operator<(const AbstractClass& rhs) const { return this->UniqueID < rhs.UniqueID; }

// Supplied target functions 410300/410310: these are not COM refcount updates.
#include "yrpp/CRC.h"
#include "yrpp/ScenarioClass.h"
ULONG YRPP_STDCALL AbstractClass::AddRef() { return 1; }
ULONG YRPP_STDCALL AbstractClass::Release() { return 1; }
void YRPP_STDCALL AbstractClass::Create_ID() {  // 410230, adjusted IRTTI this
    UniqueID = ScenarioClass::Instance ? static_cast<DWORD>(ScenarioClass::Instance->CreateUniqueID()) : 0u;
}
void AbstractClass::Init() {}                                // 410470: confirmed nullsub
void AbstractClass::PointerExpired(AbstractClass*, bool) {}  // 410480
void AbstractClass::ComputeCRC(CRCEngine& crc) const {       // 410410
    crc(static_cast<int>(UniqueID));
    crc(Dirty);
}
int AbstractClass::GetOwningHouseIndex() const { return -1; }          // 410490
HouseClass* AbstractClass::GetOwningHouse() const { return nullptr; }  // 4104A0
int AbstractClass::GetArrayIndex() const { return 0; }                 // 4104B0, NOT -1
bool AbstractClass::IsDead() const { return true; }                    // 410440, NOT false
CoordStruct* AbstractClass::GetCoords(CoordStruct* out) const {        // 4104C0
    *out = CoordStruct::Empty;
    return out;
}
CoordStruct* AbstractClass::GetDestination(CoordStruct* out, TechnoClass*) const {  // 4104F0
    CoordStruct value;
    *out = *GetCoords(&value);
    return out;
}
bool AbstractClass::IsOnFloor() const { return false; }                // 410520
bool AbstractClass::IsInAir() const { return false; }                  // 410530
CoordStruct* AbstractClass::GetCenterCoords(CoordStruct* out) const {  // 410540
    CoordStruct value;
    *out = *GetCoords(&value);
    return out;
}
void AbstractClass::Update() {}  // 410570: confirmed nullsub

// Supplied secondary-interface thunks 410580/410590: verified empty semantics.
bool AbstractClass::INoticeSink_Unknown(DWORD, DWORD) { return false; }
void YRPP_STDCALL AbstractClass::INoticeSource_Unknown() {}

void AbstractClass::NotifyTriggerNodeExpired(bool removed) {
    // Event/action arm of 7258D0. Iterate the actual original listener list.
    for (int i = 0; i < TriggerExpirationListeners.Count; ++i)
        if (auto* receiver = TriggerExpirationListeners[i]) receiver->PointerExpired(this, removed);
}

const char* YRPP_FASTCALL AbstractClass::GetRTTIName(AbstractType type) {
    // Original name table at 0x816EE0, including non-class display names.
    static constexpr const char* names[] = {"<none>",
                                            "Unit",
                                            "Aircraft",
                                            "AircraftType",
                                            "Anim",
                                            "AnimType",
                                            "Building",
                                            "BuildingType",
                                            "Bullet",
                                            "BulletType",
                                            "Campaign",
                                            "Cell",
                                            "Factory",
                                            "House",
                                            "HouseType",
                                            "Infantry",
                                            "InfantryType",
                                            "Isotile",
                                            "IsotileType",
                                            "Light",
                                            "Overlay",
                                            "OverlayType",
                                            "Particle",
                                            "ParticleType",
                                            "ParticleSystem",
                                            "ParticleSystemType",
                                            "Script",
                                            "ScriptType",
                                            "Side",
                                            "Smudge",
                                            "SmudgeType",
                                            "Special",
                                            "SuperWeaponType",
                                            "TaskForce",
                                            "Team",
                                            "TeamType",
                                            "Terrain",
                                            "TerrainType",
                                            "Trigger",
                                            "TriggerType",
                                            "UnitType",
                                            "VoxelAnim",
                                            "VoxelAnimType",
                                            "Wave",
                                            "Tag",
                                            "TagType",
                                            "Tiberium",
                                            "Action",
                                            "Event",
                                            "WeaponType",
                                            "WarheadType",
                                            "Waypoint",
                                            "Abstract",
                                            "Tube",
                                            "LightSource",
                                            "EMPulse",
                                            "TacticalMap",
                                            "SuperWeapon",
                                            "AITrigger",
                                            "AITriggerType",
                                            "Neuron",
                                            "FoggedObject",
                                            "AlphaShape",
                                            "VeinholeMonster",
                                            "NavyType",
                                            "SpawnManager",
                                            "CaptureManager",
                                            "Parasite",
                                            "Bomb",
                                            "RadSite",
                                            "Temporal",
                                            "Airstrike",
                                            "SlaveManager",
                                            "DiskLaser"};
    const auto index = static_cast<unsigned>(type);
    return index < sizeof(names) / sizeof(*names) ? names[index] : "Unknown";
}

// Original raw-object persistence. Files retain the ABI of their producing build;
// x86 records match the original layout, while native-width files are host-local.
#include "original_stream.hpp"
#include "yrpp/SwizzleManagerClass.h"
#include <cstdint>
HRESULT YRPP_STDCALL AbstractClass::Save(IStream* stream, BOOL clearDirty) {
    if (!stream) return static_cast<HRESULT>(0x80004003u);
    try {
        const auto identity = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this));
        auto result = game::original_stream_write(stream, &identity, 4);
        if (result >= 0) {
            result = game::original_stream_write(stream, this, static_cast<ULONG>(Size()));
            if (result >= 0 && clearDirty) Dirty = false;
        }
        return result;
    } catch (...) {
        return static_cast<HRESULT>(0x80004005u);
    }
}
HRESULT YRPP_STDCALL AbstractClass::Load(IStream* stream) {
    if (!stream) return static_cast<HRESULT>(0x80004003u);
    try {
        std::uint32_t identity = 0;
        auto result = game::original_stream_read(stream, &identity, 4);
        if (result >= 0) {
            SwizzleManagerClass::Instance.Here_I_Am(static_cast<LONG>(identity), this);
            const auto references = RefCount;
            result = game::original_stream_read(stream, this, static_cast<ULONG>(Size()));
            RefCount = references;
        }
        return result;
    } catch (...) {
        return static_cast<HRESULT>(0x80004005u);
    }
}
