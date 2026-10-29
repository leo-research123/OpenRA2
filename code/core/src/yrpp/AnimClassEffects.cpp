// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 anim.cpp Start/Middle, calibrated to YR 0x424CE0/0x424F00.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/VocClass.h"
#include <bit>
#include <new>
#include <cstdlib>

void YRPP_FASTCALL AnimClass::ApplyNukeDamage(AnimClass*, const CellStruct* cell) noexcept {
    try {
        const CoordStruct at{int(cell->X) * 256 + 128, int(cell->Y) * 256 + 128, 0};
        auto& rules = *RulesClass::Instance;
        MapClass::DamageArea(at, rules.AtomDamage, nullptr, rules.NukeWarhead, false, nullptr);
        // 0x425245 requests a palette fade only for campaign mode. Its helper
        // (0x4A3C30) returns immediately on non-indexed surfaces, as used here.
    } catch (...) {
        std::abort();
    }
}

void AnimClass::Start() const {
    auto& self = *const_cast<AnimClass*>(this);
    self.Mark(MarkType::Change);
    if (!Type) return;
    if (!IsInert && Type->Report != -1)
        VocClass::PlayAt(Type->Report, GetCoords(), &self.StartSoundAudioController);
    else
        self.StartSoundAudioController.Stop();
    self.StopSoundAudioController.Stop();
    if (!Type->MiddleFrameIndex) Middle();
    if (IsInert || !Type->TiberiumChainReaction) return;
    auto* cell = MapClass::Instance.GetCellAt(GetCoords());
    const int index = cell->GetContainedTiberiumIndex();
    if (index == -1) return;
    auto* tiberium = TiberiumClass::Array[index];
    cell->ReduceTiberium(cell->OverlayData + 1);
    auto& random = ScenarioClass::Instance->Random;
    if (tiberium->Debris.Count > 0 && random.Random() % 3 == 0) {
        // Allocation precedes the random choice in the original.
        auto* storage = YRMemory::Allocate(sizeof(AnimClass));
        if (storage) {
            auto at = GetCoords();
            at.Z = std::bit_cast<int>(unsigned(at.Z) + 10u);
            const int choice = random.RandomRanged(0, tiberium->Debris.Count - 1);
            auto* debris = ::new (storage) AnimClass(tiberium->Debris[choice], at);
            debris->LightConvert = ColorScheme::Array[tiberium->Color]->LightConvert;
            debris->TintColor = static_cast<short>(cell->Intensity_Normal);
        }
    }
    auto& rules = *RulesClass::Instance;
    MapClass::DamageArea(Location, rules.TiberiumExplosionDamage, nullptr, rules.C4Warhead, false, nullptr);
    cell->RecalcAttributes(-1);
    MapClass::Instance.ResetZones(cell->MapCoords);
    MapClass::Instance.RecalculateSubZones(cell->MapCoords);
}

bool AnimClass::Middle() const {
    if (!Type) return false;
    auto* cell = MapClass::Instance.GetCellAt(CellClass::Coord2Cell(GetCoords()));
    int width = 30, height = 30;
    if (auto* image = GetImage()) {
        if (Type->MiddleFrameWidth == -1) Type->MiddleFrameWidth = image->GetFrameBounds(Type->MiddleFrameIndex).Width;
        if (Type->MiddleFrameHeight == -1)
            Type->MiddleFrameHeight = image->GetFrameBounds(Type->MiddleFrameIndex).Height;
        width = Type->MiddleFrameWidth;
        height = Type->MiddleFrameHeight;
    }
    if (Type->SpawnsParticle != -1 && ParticleSystemClass::DefaultSystem) {
        auto* particle = ParticleTypeClass::Array[Type->SpawnsParticle];
        for (int i = 0; i < Type->NumParticles; ++i)
            ParticleSystemClass::DefaultSystem->SpawnParticle(particle, Location);
    }
    const int altitude = GetHeight();
    if (altitude >= 30) return (altitude & 0xFF) != 0;
    // 0x42508D uses the inclusive 0..0x7FFFFFFE range, unlike RandomDouble.
    if (Type->Scorch &&
        (!Type->Crater ||
         double(ScenarioClass::Instance->Random.RandomRanged(0, 0x7FFFFFFE)) * 4.656612877414201e-10 < 0.5))
        return SmudgeTypeClass::ScorchTheGround(GetCoords(), width, height, false);
    if (!Type->Crater) return false;
    cell->ReduceTiberium(6);
    return Type->ForceBigCraters ? SmudgeTypeClass::CraterTheGround(GetCoords(), 300, 300, true)
                                 : SmudgeTypeClass::CraterTheGround(GetCoords(), width, height, false);
}
