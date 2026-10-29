#include "yrpp/InfantryClass.h"
#include "techno_spatial.hpp"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>

void SlaveManagerClass::ZeroOutSlaves() {
    for (const auto& pNode : this->SlaveNodes) {
        if (auto pSlave = pNode->Slave) {
            pSlave->SlaveOwner = nullptr;
        }
        pNode->Slave = nullptr;
        pNode->State = SlaveControlStatus::Dead;
        pNode->RespawnTimer.Start(this->RegenRate);
    }
}

#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<SlaveManagerClass*> nativeSlaveManagers;
}
DynamicVectorClass<SlaveManagerClass*>& SlaveManagerClass::Array = nativeSlaveManagers;

HRESULT YRPP_STDCALL SlaveManagerClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr unsigned words[]{0x335AAFE4, 0x11D52DA6, 0x100022BE, 0x6CA1624B};
    std::memcpy(output, words, sizeof(words));
    return 0;
}

SlaveManagerClass::SlaveManagerClass(TechnoClass* owner, InfantryTypeClass* slave, int count, int regenRate,
                                     int reloadRate) noexcept
    : AbstractClass(), Owner(owner), SlaveType(slave), SlaveCount(count), RegenRate(regenRate), ReloadRate(reloadRate) {
    // Node allocation failure is undefined in the EXE; no exception crosses this boundary.
    try {
        for (int i = 0; i < SlaveCount; ++i) {
            auto* node = GameCreate<SlaveControl>();
            node->RespawnTimer.Start(0);
            CreateSlave(node);
            // A failed vector growth skips the insertion in 0x6AF1A0.
            try {
                SlaveNodes.AddItem(node);
            } catch (const std::bad_alloc&) {
                SlaveNodes.IsInitialized = true;
            }
        }
        RespawnTimer.Start(10);
        State = SlaveManagerStatus::Ready;
        LastScanFrame = 0;
        try {
            Array.AddItem(this);
        } catch (const std::bad_alloc&) {
            Array.IsInitialized = true;
        }
    } catch (...) {
        std::abort();
    }
}

void SlaveManagerClass::CreateSlave(SlaveControl* node) {
    node->Slave = static_cast<InfantryClass*>(SlaveType->CreateObject(Owner->GetOwningHouse()));
    if (node->Slave) {
        node->Slave->Limbo();
        node->Slave->SlaveOwner = Owner;
        node->State = SlaveControlStatus::Unknown;
        node->RespawnTimer.Start(0);
    }
}

void SlaveManagerClass::SetOwner(TechnoClass* owner) {
    Owner->SlaveManager = nullptr;
    Owner = owner;
    if (auto* temporary = owner->SlaveManager) {
        temporary->Killed(nullptr, nullptr);
        if (owner->SlaveManager) GameDelete(owner->SlaveManager);
    }
    owner->SlaveManager = this;
    for (int i = SlaveNodes.Count - 1; i >= 0; --i)
        if (auto* slave = SlaveNodes[i]->Slave) slave->SlaveOwner = Owner;
}

void SlaveManagerClass::LostSlave(InfantryClass* slave) {
    for (int i = SlaveNodes.Count - 1; i >= 0; --i) {
        auto* node = SlaveNodes[i];
        if (node->Slave == slave) {
            slave->SlaveOwner = nullptr;
            node->Slave = nullptr;
            node->State = SlaveControlStatus::Dead;
            node->RespawnTimer.Start(RegenRate);
            return;
        }
    }
}

void SlaveManagerClass::Update() {
    if (RespawnTimer.GetTimeLeft()) return;
    RespawnTimer.Start(10);
    if (Owner) {
        UpdateSlaves();
        UpdateOwner();
    }
}

CellStruct* SlaveManagerClass::GetExitCell(CellStruct* output) {
    Owner->GetMapCoords(output);
    if (Owner->WhatAmI() == AbstractType::Building) {
        auto* type = static_cast<BuildingClass*>(Owner)->Type;
        output->X = short(output->X + type->GetFoundationWidth() - 1);
        output->Y = short(output->Y + type->GetFoundationHeight(false) / 2);
    }
    return output;
}

CellStruct* SlaveManagerClass::GetAlternateExitCell(CellStruct* output) {
    *output = CellStruct::Empty;
    if (Owner && Owner->WhatAmI() == AbstractType::Building) {
        CellStruct exit;
        GetExitCell(&exit);
        --exit.Y;
        if (MapClass::Instance.GetCellAt(exit)->GetBuilding() == Owner) *output = exit;
    }
    return output;
}

bool SlaveManagerClass::IsSlaveAtCell(InfantryClass* slave, CellClass* cell) {
    CellStruct exit;
    GetExitCell(&exit);
    if (cell->MapCoords != exit) {
        GetAlternateExitCell(&exit);
        if (exit == CellStruct::Empty || cell->MapCoords != exit) return false;
    }
    for (int i = SlaveNodes.Count - 1; i >= 0; --i)
        if (SlaveNodes[i]->Slave == slave) return true;
    return false;
}

CellStruct* SlaveManagerClass::FindDeployCell(CellStruct* output, CellStruct target) {
    auto* type = Owner->WhatAmI() == AbstractType::Building ? static_cast<BuildingClass*>(Owner)->Type
                                                            : Owner->GetTechnoType()->DeploysInto;
    const int width = type ? type->GetFoundationWidth() : 1;
    const int height = type ? type->GetFoundationHeight(false) : 1;
    const int zone = MapClass::Instance.GetMovementZoneType(Owner->GetMapCoords(), MovementZone::Normal, false);
    MapClass::Instance.NearByLocation(*output, target, SpeedType::Track, zone, MovementZone::Normal, false, width,
                                      height, true, false, false, false, target, false, true);
    if (*output != CellStruct::Empty) {
        if (width > 2) ++output->X;
        if (height > 2) ++output->Y;
    }
    return output;
}

void SlaveManagerClass::GuardSlaves() {
    for (int i = SlaveNodes.Count - 1; i >= 0; --i) {
        auto* node = SlaveNodes[i];
        if (node->State != SlaveControlStatus::Dead && node->Slave) node->Slave->Guard();
    }
}

void SlaveManagerClass::DeploySlaves() {
    for (int i = SlaveNodes.Count - 1; i >= 0; --i) {
        auto* node = SlaveNodes[i];
        if (node->State != SlaveControlStatus::Unknown) continue;
        CellStruct exit;
        GetExitCell(&exit);
        if (exit == CellStruct::Empty) continue;
        CoordStruct position;
        MapClass::PickInfantrySublocation(position, MapClass::Instance.GetCellAt(exit)->GetCoords(), false);
        if (position == CoordStruct::Empty || position == CoordStruct{128, 128, 0}) continue;
        if (node->Slave->Unlimbo(position, DirType::North)) {
            node->Slave->Scatter(Owner->GetCoords(), true, true);
            node->State = SlaveControlStatus::ScanningForTiberium;
        }
    }
}

namespace {
int cell_distance(CellStruct a, CellStruct b) {
    const auto x = short(a.X - b.X), y = short(a.Y - b.Y);
    return short(game::x87_integer(std::sqrt(double(x) * x + double(y) * y)));
}
}

void SlaveManagerClass::UpdateSlaves() {
    for (int i = 0; i < SlaveNodes.Count; ++i) {
        auto* node = SlaveNodes[i];
        auto* slave = node->Slave;
        if (!slave && node->State != SlaveControlStatus::Dead) {
            node->State = SlaveControlStatus::Dead;
            node->RespawnTimer.Start(RegenRate);
        }
        const auto send = [&](CellStruct at) {
            slave->SetDestination(MapClass::Instance.GetCellAt(at), true);
            slave->QueueMission(Mission::Move, false);
        };
        switch (node->State) {
            case SlaveControlStatus::ScanningForTiberium: {
                CellStruct at;
                slave->ScanForTiberium(&at, RulesClass::Instance->SlaveMinerSlaveScan / 256, 0);
                if (at == CellStruct::Empty) {
                    slave->SetArchiveTarget(nullptr);
                    GetExitCell(&at);
                    node->State = SlaveControlStatus::BringingItBack;
                } else
                    node->State = SlaveControlStatus::MovingToTiberium;
                send(at);
                break;
            }
            case SlaveControlStatus::MovingToTiberium:
                if (slave->GetCell()->LandType == LandType::Tiberium) {
                    if (slave->GetCurrentMission() != Mission::Harvest) slave->QueueMission(Mission::Harvest, false);
                    node->State = SlaveControlStatus::Harvesting;
                } else if (!slave->Destination)
                    node->State = SlaveControlStatus::ScanningForTiberium;
                break;
            case SlaveControlStatus::Harvesting:
                if (slave->GetStoragePercentage() == 1.0) {
                    slave->SetArchiveTarget(slave->GetCell());
                    CellStruct at;
                    GetExitCell(&at);
                    send(at);
                    node->State = SlaveControlStatus::BringingItBack;
                } else if (slave->GetCell()->LandType == LandType::Tiberium) {
                    if (slave->GetCurrentMission() != Mission::Harvest)
                        node->State = SlaveControlStatus::ScanningForTiberium;
                } else {
                    slave->QueueMission(Mission::Guard, false);
                    node->State = SlaveControlStatus::ScanningForTiberium;
                }
                break;
            case SlaveControlStatus::BringingItBack: {
                CellStruct exit;
                GetExitCell(&exit);
                const bool retarget =
                    slave->Destination && cell_distance(CellClass::Coord2Cell(slave->Destination->GetCoords()), exit) >
                                              RulesClass::Instance->CloseEnough;
                if (slave->GetMapCoords() == exit && !slave->Destination) {
                    slave->DeliverTiberium(Owner);
                    slave->Limbo();
                    node->State = SlaveControlStatus::Respawning;
                    node->RespawnTimer.Start(ReloadRate);
                } else if (!slave->Destination || retarget)
                    send(exit);
                break;
            }
            case SlaveControlStatus::Respawning:
                if (!node->RespawnTimer.GetTimeLeft()) {
                    slave->Health = slave->EstimatedHealth = slave->Type->Strength;
                    game::techno_spatial_dirty(slave);
                    node->State = SlaveControlStatus::Unknown;
                }
                break;
            case SlaveControlStatus::Dead:
                if (!node->RespawnTimer.GetTimeLeft()) CreateSlave(node);
                break;
            default:
                break;
        }
    }
}

void SlaveManagerClass::UpdateOwner() {
    const auto transition = [&](SlaveManagerStatus state) {
        State = state;
        LastScanFrame = 0x7FFFFFFF;
    };
    const auto ready = [&]() {
        State = SlaveManagerStatus::Ready;
        LastScanFrame = Unsorted::CurrentFrame;
    };
    const auto scan = [&](int range, DWORD close) {
        CellStruct result;
        Owner->ScanForTiberium(&result, range / 256, close);
        return result;
    };
    const auto kind = Owner->WhatAmI();
    auto* unit = kind == AbstractType::Unit ? static_cast<UnitClass*>(Owner) : nullptr;
    auto* building = kind == AbstractType::Building ? static_cast<BuildingClass*>(Owner) : nullptr;
    switch (State) {
        case SlaveManagerStatus::Ready:
            if (building && Owner->CurrentMission != Mission::Construction && Owner->CurrentMission != Mission::Selling)
                transition(SlaveManagerStatus::ScanningAgain);
            break;
        case SlaveManagerStatus::Scanning:
            if (unit && unit->Destination)
                transition(SlaveManagerStatus::Travelling);
            else {
                auto at = scan(RulesClass::Instance->SlaveMinerLongScan, 1);
                if (at != CellStruct::Empty) FindDeployCell(&at, at);
                if (at == CellStruct::Empty)
                    ready();
                else {
                    Owner->SetDestination(MapClass::Instance.GetCellAt(at), true);
                    Owner->QueueMission(Mission::Move, false);
                    transition(SlaveManagerStatus::Travelling);
                }
            }
            break;
        case SlaveManagerStatus::Travelling:
            if (!unit)
                ready();
            else if (!unit->Destination) {
                if (unit->TryToDeploy())
                    transition(SlaveManagerStatus::Working);
                else {
                    transition(SlaveManagerStatus::Deploying);
                    RespawnTimer.Start(30);
                }
            }
            break;
        case SlaveManagerStatus::Deploying:
            if (!unit)
                ready();
            else
                transition(unit->TryToDeploy() ? SlaveManagerStatus::Working : SlaveManagerStatus::Scanning);
            break;
        case SlaveManagerStatus::Working:
            if (building && building->BState)
                transition(SlaveManagerStatus::ScanningAgain);
            else if (unit && !unit->IsDeploying && Owner->CurrentMission == Mission::Guard)
                transition(SlaveManagerStatus::Travelling);
            break;
        case SlaveManagerStatus::ScanningAgain:
            if (unit) {
                ready();
                break;
            }
            if (scan(RulesClass::Instance->SlaveMinerShortScan, 0) == CellStruct::Empty) {
                const auto ore = scan(RulesClass::Instance->SlaveMinerLongScan, 1);
                CellStruct at;
                FindDeployCell(&at, ore);
                if (cell_distance(at, ore) + RulesClass::Instance->SlaveMinerScanCorrection / 256 <
                    cell_distance(Owner->GetMapCoords(), ore)) {
                    Owner->unknown_bool_4F8 = true;
                    Owner->SetArchiveTarget(Owner->GetCell());
                    Owner->QueueMission(Mission::Selling, false);
                    transition(SlaveManagerStatus::PackingUp);
                }
            }
            if (State != SlaveManagerStatus::PackingUp) DeploySlaves();
            break;
        case SlaveManagerStatus::PackingUp:
            if (unit) {
                GuardSlaves();
                transition(SlaveManagerStatus::Scanning);
            }
            break;
    }
}

void SlaveManagerClass::AllGuard() {
    State = SlaveManagerStatus::Ready;
    LastScanFrame = Unsorted::CurrentFrame;
    GuardSlaves();
}
void SlaveManagerClass::Deploy() {
    if (State == SlaveManagerStatus::Ready) {
        State = SlaveManagerStatus::Working;
        LastScanFrame = 0x7FFFFFFF;
        GuardSlaves();
    }
}
void SlaveManagerClass::Deploy2() { Deploy(); }

void SlaveManagerClass::Harvest() {
    AbstractClass* target = nullptr;
    const auto kind = Owner->WhatAmI();
    if (kind == AbstractType::Unit)
        target = static_cast<UnitClass*>(Owner)->Destination;
    else if (kind == AbstractType::Building)
        target = Owner->ArchiveTarget;
    if (target) {
        CellStruct at;
        FindDeployCell(&at, CellClass::Coord2Cell(target->GetCoords()));
        if (at == CellStruct::Empty) {
            if (kind == AbstractType::Building) return;
            State = SlaveManagerStatus::Ready;
            LastScanFrame = Unsorted::CurrentFrame;
            Owner->QueueMission(Mission::Guard, false);
            return;
        }
        if (kind == AbstractType::Unit) {
            Owner->SetDestination(MapClass::Instance.GetCellAt(at), true);
            Owner->QueueMission(Mission::Move, false);
            State = SlaveManagerStatus::Travelling;
        } else {
            Owner->SetArchiveTarget(MapClass::Instance.GetCellAt(at));
            State = SlaveManagerStatus::PackingUp;
        }
        LastScanFrame = 0x7FFFFFFF;
        GuardSlaves();
    } else {
        Owner->QueueMission(Mission::Guard, false);
        State = SlaveManagerStatus::Ready;
        LastScanFrame = Unsorted::CurrentFrame;
    }
}

bool SlaveManagerClass::ShouldWakeUpNow() {
    if (!Owner || State != SlaveManagerStatus::Ready) return false;
    if (!Owner->Owner->IsHumanPlayer || Owner->GetCell()->LandType == LandType::Tiberium) return true;
    const int deadline =
        std::bit_cast<int>(unsigned(LastScanFrame) + unsigned(RulesClass::Instance->SlaveMinerKickFrameDelay));
    if (deadline >= Unsorted::CurrentFrame) return false;
    CellStruct at;
    Owner->ScanForTiberium(&at, RulesClass::Instance->SlaveMinerShortScan / 256, 0);
    return at != CellStruct::Empty;
}

SlaveManagerClass::~SlaveManagerClass() {
    for (int i = SlaveNodes.Count - 1; i >= 0; --i)
        if (SlaveNodes[i]) GameDelete(SlaveNodes[i]);
    const int capacity = SlaveNodes.Capacity;
    // 0x6AF4A0 clears, restores capacity, unregisters, then destroys the vector.
    // Keep the transient allocation and its ordering relative to Array removal.
    try {
        SlaveNodes.Clear();
        SlaveNodes.SetCapacity(capacity);
    } catch (const std::bad_alloc&) {
        SlaveNodes.IsInitialized = true;
    } catch (...) {
        std::abort();
    }
    Array.Remove(this);
}

void SlaveManagerClass::BeginScanning() {
    if (State != SlaveManagerStatus::Ready) return;
    State = SlaveManagerStatus::Scanning;
    LastScanFrame = 0x7FFFFFFF;
    for (int i = SlaveNodes.Count - 1; i >= 0; --i) {
        auto* node = SlaveNodes[i];
        if (node->State != SlaveControlStatus::Dead && node->Slave) node->Slave->Guard();
    }
}

void SlaveManagerClass::Killed(TechnoClass* killer, HouseClass* forcedOwnerHouse) {
    if (!Owner) return;
    const int civilianSide = SideClass::FindIndex("Civilian");
    HouseClass* civilian = nullptr;
    for (auto* house : HouseClass::Array)
        if (house->Type->SideIndex == civilianSide) {
            civilian = house;
            break;
        }
    InfantryClass* firstFreed = nullptr;
    for (int i = SlaveNodes.Count - 1; i >= 0; --i) {
        auto* slave = SlaveNodes[i]->Slave;
        if (!slave || !Game::IsActive) continue;
        slave->SlaveOwner = nullptr;
        if (slave->InLimbo) {
            slave->RegisterDestruction(killer);
            slave->UnInit();
            continue;
        }
        auto* house = killer ? killer->Owner : forcedOwnerHouse ? forcedOwnerHouse : civilian;
        if (house) {
            slave->SetOwningHouse(house, true);
            slave->Guard();
            slave->Cheer(true);
            if (!firstFreed) firstFreed = slave;
        } else {
            int damage = slave->Health;
            slave->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, false, false, nullptr);
        }
    }
    if (Game::IsActive && firstFreed && !Unsorted::ScenarioInit && RulesClass::Instance->SlavesFreeSound != -1)
        VocClass::PlayAt(RulesClass::Instance->SlavesFreeSound, firstFreed->Location);
    Owner = nullptr;
}
#endif
