// Original YR BombListClass, 0x00438BF0..0x004391B3.
#include "yrpp/BombListClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/VocClass.h"
#include "RulesClassReaders.hpp"
#include <bit>
#include <cstdlib>

namespace {
BombListClass bombList;
}
BombListClass& BombListClass::Instance = bombList;

void BombListClass::Plant(TechnoClass* source, ObjectClass* target) {
    if (!source || source->WhatAmI() != AbstractType::Infantry || !target || target->AttachedBomb) return;
    try {
        auto* bomb = GameCreate<BombClass>();
        bomb->OwnerHouse = source->GetOwningHouse();
        bomb->Owner = source;
        bomb->Target = target;
        bomb->PlantingFrame = Unsorted::CurrentFrame;
        bomb->DeathBomb = false;
        bomb->DetonationFrame =
            std::bit_cast<int>(unsigned(bomb->PlantingFrame) + unsigned(RulesClass::Instance->IvanTimedDelay));
        target->AttachedBomb = bomb;
        if (!Bombs.AddItem(bomb)) std::abort();
        UpdateDelay = 1;
        if (RulesClass::Instance->BombAttachSound != -1 && source->Owner->IsControlledByCurrentPlayer())
            VocClass::PlayAt(RulesClass::Instance->BombAttachSound, target->Location, nullptr);
    } catch (...) {
        // The original cannot continue after a failed bomb allocation.
        std::abort();
    }
}

void BombListClass::Update() {
    for (int i = Bombs.Count - 1; i >= 0; --i) {
        auto* bomb = Bombs[i];
        if (!bomb || !bomb->Target) {
            if (bomb) GameDelete(bomb);
            Bombs.RemoveItem(i);
        }
    }
    for (int i = Bombs.Count - 1; i >= 0; --i) {
        auto* bomb = Bombs[i];
        if (bomb->TickSound == -1) continue;
        if (bomb->Target->InLimbo) {
            bomb->Audio.Stop();
            bomb->ShouldPlayTickingSound = false;
        } else if (!bomb->ShouldPlayTickingSound) {
            VocClass::PlayAt(bomb->TickSound, bomb->Target->Location, &bomb->Audio);
            bomb->ShouldPlayTickingSound = true;
        } else {
            VocClass::UpdatePosition(&bomb->Target->Location, &bomb->Audio);
        }
    }
    if (UpdateDelay > 0) {
        --UpdateDelay;
        return;
    }
    UpdateDelay = 45;
    for (int i = Bombs.Count - 1; i >= 0; --i) {
        auto* bomb = Bombs[i];
        bool visible = bomb->OwnerHouse->IsControlledByCurrentPlayer();
        if (!visible)
            for (int d = 0; d < Detectors.Count; ++d) {
                auto* detector = Detectors[d];
                if (!detector->GetOwningHouse()->IsControlledByCurrentPlayer()) continue;
                const auto target = bomb->Target->GetCoords();
                const auto at = detector->GetCoords();
                const auto delta = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
                const int x = delta(at.X, target.X), y = delta(at.Y, target.Y), z = delta(at.Z, target.Z);
                const int distance = rule_integer(Math::sqrt(double(x) * x + double(y) * y + double(z) * z));
                const int range = std::bit_cast<int>(unsigned(detector->GetTechnoType()->BombSight) << 8);
                if (distance < range) {
                    visible = true;
                    break;
                }
            }
        const bool previous = bomb->Target->BombVisible;
        bomb->Target->BombVisible = visible;
        if (previous != visible) bomb->Target->NeedsRedraw = true;
    }
}

void BombListClass::AddDetector(TechnoClass* detector) {
    try {
        if (!Detectors.AddItem(detector)) std::abort();
    } catch (...) {
        std::abort();
    }
}

void BombListClass::RemoveDetector(TechnoClass* detector) { Detectors.Remove(detector); }

void BombListClass::PointerGotInvalid(AbstractClass* invalid) {
    Bombs.Remove(reinterpret_cast<BombClass*>(invalid));
    for (auto* bomb : Bombs)
        if (bomb && bomb->Owner == invalid) bomb->Owner = nullptr;
}
void BombListClass::Clear() {
    UpdateDelay = 45;
    const int bombs = Bombs.Capacity;
    Bombs.Clear();
    Bombs.SetCapacity(bombs, nullptr);
    const int detectors = Detectors.Capacity;
    Detectors.Clear();
    Detectors.SetCapacity(detectors, nullptr);
}
