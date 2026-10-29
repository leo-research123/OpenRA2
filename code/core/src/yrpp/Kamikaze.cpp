// Original missile target retention and attack scheduling, YR 0x54E3B0.
#include "yrpp/Kamikaze.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/MapClass.h"

namespace {
Kamikaze kamikaze;
}
Kamikaze& Kamikaze::Instance = kamikaze;

Kamikaze::~Kamikaze() {
    for (int i = Nodes.Count - 1; i >= 0; --i)
        GameDelete(Nodes[i]);
}

void Kamikaze::Add(AircraftClass* aircraft, AbstractClass* target) {
    if (!aircraft->Type->MissileSpawn) {
        aircraft->Crash(nullptr);
        return;
    }
    auto* node = GameCreate<KamikazeControl>();
    node->Item = aircraft;
    if (target) {
        const auto at = target->GetCoords();
        node->Cell = MapClass::Instance.GetCellAt(CellStruct{short(at.X / 256), short(at.Y / 256)});
    } else
        node->Cell = aircraft->GetCell()->GetNeighbourCell(
            static_cast<FacingType>(aircraft->PrimaryFacing.Current().GetValue<3>()));
    aircraft->IsKamikaze = true;
    aircraft->Ammo = 1;
    try {
        Nodes.AddItem(node);
    } catch (const std::bad_alloc&) {
        Nodes.IsInitialized = true;
    }
}

void Kamikaze::Update() {
    if (UpdateTimer.GetTimeLeft()) return;
    UpdateTimer.Start(30);
    for (int i = 0; i < Nodes.Count; ++i) {
        auto* node = Nodes[i];
        auto* aircraft = node->Item;
        aircraft->Ammo = 1;
        auto* cell = node->Cell;
        if (!cell)
            cell = aircraft->GetCell()->GetNeighbourCell(
                static_cast<FacingType>(aircraft->PrimaryFacing.Current().GetValue<3>()));
        aircraft->SetTarget(cell);
        aircraft->QueueMission(Mission::Attack, false);
    }
}

void Kamikaze::Remove(AbstractClass* object) {
    for (int i = Nodes.Count - 1; i >= 0; --i) {
        auto* node = Nodes[i];
        if (node->Item == object) {
            GameDelete(node);
            Nodes.RemoveItem(i);
            return;
        }
        if (node->Cell == object) {
            const auto at = object->GetCoords();
            auto* cell = MapClass::Instance.GetCellAt(CellStruct{short(at.X / 256), short(at.Y / 256)});
            node->Item->SetTarget(cell);
            node->Item->QueueMission(Mission::Attack, false);
            node->Cell = cell;
            return;
        }
    }
}

void Kamikaze::Clear() {
    for (int i = Nodes.Count - 1; i >= 0; --i)
        GameDelete(Nodes[i]);
    const int capacity = Nodes.Capacity;
    Nodes.Clear();
    try {
        Nodes.SetCapacity(capacity);
    } catch (const std::bad_alloc&) {
        Nodes.IsInitialized = true;
    }
    UpdateTimer.Start(1);
}
