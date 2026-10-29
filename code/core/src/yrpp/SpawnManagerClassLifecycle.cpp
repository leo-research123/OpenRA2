// Original spawn ownership and expiration, YR 0x6B6C90/0x6B7010/0x6B7100.
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/Kamikaze.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"
#include <cstring>

namespace {
DynamicVectorClass<SpawnManagerClass*> managers;
}
DynamicVectorClass<SpawnManagerClass*>& SpawnManagerClass::Array = managers;

SpawnManagerClass::SpawnManagerClass(TechnoClass* owner, AircraftTypeClass* type, int count, int regen,
                                     int reload) noexcept
    : AbstractClass(), Owner(owner), SpawnType(type), SpawnCount(count), RegenRate(regen), ReloadRate(reload),
      SpawnedNodes(), UpdateTimer(0), SpawnTimer(0), Target(nullptr), NewTarget(nullptr),
      Status(SpawnManagerStatus::Idle) {
    for (int i = 0; i < SpawnCount; ++i) {
        auto* memory = YRMemory::Allocate(sizeof(SpawnControl));
        if (!memory) break;
        auto* node = ::new (memory) SpawnControl{};
        // 0x6B6D31 initializes the timer before aircraft creation can fail.
        node->SpawnTimer.Start(0);
        node->Unit = static_cast<AircraftClass*>(SpawnType->CreateObject(Owner->GetOwningHouse()));
        if (node->Unit) {
            const auto& rules = *RulesClass::Instance;
            node->IsSpawnMissile =
                SpawnType == rules.V3Rocket.Type || SpawnType == rules.DMisl.Type || SpawnType == rules.CMisl.Type;
            node->Unit->Limbo();
            node->Unit->SpawnOwner = Owner;
            node->Status = SpawnNodeStatus::Idle;
            node->SpawnTimer.Start(0);
            // Vector allocation throws in the native allocator. The original
            // returns false and leaves the node outside the registered list.
            try {
                SpawnedNodes.AddItem(node);
            } catch (const std::bad_alloc&) {
                SpawnedNodes.IsInitialized = true;
            }
        }
    }
    UpdateTimer.Start(20);
    SpawnTimer.Start(0);
    try {
        Array.AddItem(this);
    } catch (const std::bad_alloc&) {
        Array.IsInitialized = true;
    }
}

SpawnManagerClass::~SpawnManagerClass() {
    if (Game::IsActive) KillNodes();
    for (int i = SpawnedNodes.Count - 1; i >= 0; --i)
        GameDelete(SpawnedNodes[i]);
    Array.Remove(this);
}

HRESULT YRPP_STDCALL SpawnManagerClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[]{0x0679E981, 0x11D3AD9D, 0x100016BE, 0x6CA1624B};
    std::memcpy(id, words, sizeof(words));
    return 0;
}

void SpawnManagerClass::KillNodes() {
    const int delay = Owner->Health > 0 && Owner->IsAlive ? 0 : RegenRate;
    for (int i = SpawnedNodes.Count - 1; i >= 0; --i) {
        auto* node = SpawnedNodes[i];
        const auto status = node->Status;
        if (status == SpawnNodeStatus::Dead) continue;
        if (status == SpawnNodeStatus::TakeOff) Kamikaze::Instance.Remove(node->Unit);
        node->Status = SpawnNodeStatus::Dead;
        if (status == SpawnNodeStatus::Idle || status == SpawnNodeStatus::Reloading ||
            status == SpawnNodeStatus::TakeOff)
            node->Unit->UnInit();
        else
            Kamikaze::Instance.Add(node->Unit, Target);
        node->Unit = nullptr;
        node->SpawnTimer.Start(delay);
    }
}

void SpawnManagerClass::ResetTarget() {
    for (int i = 0; i < SpawnedNodes.Count; ++i) {
        auto* node = SpawnedNodes[i];
        if (node->Status == SpawnNodeStatus::Preparing && node->Unit->Type->MissileSpawn) {
            Kamikaze::Instance.Add(node->Unit, Target);
            Kamikaze::Instance.UpdateTimer.Start(2);
            UnlinkPointer(node->Unit);
        }
    }
    Status = SpawnManagerStatus::Idle;
    NewTarget = nullptr;
    Target = nullptr;
}

void SpawnManagerClass::UnlinkPointer(AbstractClass* object) {
    if (object == Target) {
        Target = nullptr;
        if (!NewTarget) ResetTarget();
    } else if (object == NewTarget)
        NewTarget = nullptr;
    else {
        for (int i = SpawnedNodes.Count - 1; i >= 0; --i) {
            auto* node = SpawnedNodes[i];
            if (node->Unit != object) continue;
            if (node->Unit->Health <= 0 || node->Unit->IsKamikaze || node->IsSpawnMissile == 1) {
                node->Unit = nullptr;
                node->Status = SpawnNodeStatus::Dead;
                node->SpawnTimer.Start(RegenRate);
            }
            return;
        }
        if (object == Owner) {
            KillNodes();
            ResetTarget();
        }
    }
}
