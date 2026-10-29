// Lightning-storm activation and stop request, YR 0x539EB0 / 0x53A090.
#include "yrpp/SuperClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <algorithm>
#include <bit>
#include <cstdlib>

namespace {
AnimClass* lightning_animation(AnimTypeClass* type, const CoordStruct& at, int flags = 0x600, int zAdjust = 0) {
    if (!type) return nullptr;
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    return memory ? ::new (memory) AnimClass(type, at, 0, 1, flags, zAdjust, false) : nullptr;
}
int animation_frames(const AnimClass& anim) {
    auto* image = anim.Type->GetImage();
    return image ? image->Frames : 0;
}
}

void YRPP_FASTCALL LightningStorm::Start(int duration, int deferment, CellStruct cell, HouseClass* owner) {
    if (cell == CellStruct::Empty)
        while (!MapClass::Instance.CoordinatesLegal(cell)) {
            const short y =
                static_cast<short>(ScenarioClass::Instance->Random.RandomRanged(0, MapClass::Instance.MaxHeight));
            cell.X = static_cast<short>(ScenarioClass::Instance->Random.RandomRanged(0, MapClass::Instance.MaxWidth));
            cell.Y = y;
        }
    Coords = cell;
    Owner = owner;
    if (Active) return;
    if (deferment) {
        if (!Deferment || Deferment >= deferment) Deferment = deferment;
        Duration = duration;
        return;
    }
    RadarEventClass::Create(RadarEventType::SuperweaponActivated, cell);
    Duration = duration;
    StartTime = Unsorted::CurrentFrame;
    Active = true;
    for (int i = 0; i < HouseClass::Array.Count; ++i) {
        auto* house = HouseClass::Array[i];
        if ((!Owner || !Owner->IsAlliedWith(house)) && !house->Defeated && !Unsorted::MuteSWLaunches)
            house->CreateRadarOutage(duration);
    }
    if (HouseClass::CurrentPlayer) HouseClass::CurrentPlayer->RecheckRadar = true;
    ScenarioClass::UpdateLighting();
    if (RulesClass::Instance->LightningPrintText) {
        const int color = HouseClass::CurrentPlayer ? HouseClass::CurrentPlayer->ColorSchemeIndex : 3;
        VocClass::PlayGlobal(RulesClass::Instance->StormSound, 0x2000, 1.0f, nullptr);
        MessageListClass::Instance.AddMessage(
            nullptr, 0, StringTable::LoadString("TXT_LIGHTNING_STORM", nullptr, "D:\\ra2mdpost\\ION.CPP", 531), color,
            static_cast<TextPrintType>(0x4046), 150, false);
    }
    MapClass::Instance.MarkNeedsRedraw(1);
}
void LightningStorm::RequestStop() {
    if (Active) TimeToEnd = true;
}

void LightningStorm::Strike(CellStruct cell) {
    auto& rules = *RulesClass::Instance;
    if (!rules.WeatherConClouds.Count || !rules.WeatherConBolts.Count) return;
    auto* tile = MapClass::Instance.GetCellAt(cell);
    auto* image = rules.WeatherConBolts[0]->GetImage();
    if (!image) return;
    const int height = (unsigned(tile->Flags) & 0x100u ? CellClass::BridgeHeight : 0) +
                       TacticalClass::PixelToZ(image->Height / 2) +
                       Unsorted::LevelHeight * static_cast<signed char>(tile->Level);
    const CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, height};
    if (at == CoordStruct::Empty) return;
    auto* type =
        rules.WeatherConClouds[ScenarioClass::Instance->Random.Random() % unsigned(rules.WeatherConClouds.Count)];
    if (auto* anim = lightning_animation(type, at)) {
        if (!CloudsManifesting.AddItem(anim) || !CloudsPresent.AddItem(anim)) std::abort();
    }
}
void LightningStorm::Strike2(CoordStruct coords) {
    auto& rules = *RulesClass::Instance;
    auto& map = MapClass::Instance;
    auto& random = ScenarioClass::Instance->Random;
    auto* tile = map.GetCellAt(coords);
    if (rules.WeatherConBolts.Count) {
        auto* type = rules.WeatherConBolts[random.Random() % unsigned(rules.WeatherConBolts.Count)];
        if (auto* anim = lightning_animation(type, tile->GetCellCoords()))
            if (!BoltsPresent.AddItem(anim)) std::abort();
    }
    const auto cell = tile->MapCoords;
    const int level = static_cast<signed char>(tile->Level);
    const CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128,
                         level * Unsorted::LevelHeight +
                             (unsigned(tile->Flags) & 0x100u ? CellClass::BridgeHeight : 0)};
    if (at == CoordStruct::Empty) return;
    if (rules.LightningSounds.Count)
        VocClass::PlayAt(rules.LightningSounds[random.Random() % unsigned(rules.LightningSounds.Count)], at);
    const int damage = rules.LightningDamage;
    lightning_animation(MapClass::SelectDamageAnimation(damage, rules.LightningWarhead, tile->LandType, at), at, 0x2600,
                        -15);
    auto* building = tile->GetBuilding();
    Point2D point{};
    auto* object = tile->FindTechnoNearestTo(point, false, nullptr);
    const bool infantry = object && object->WhatAmI() == AbstractType::Infantry;
    const int land = int(tile->LandType);
    const bool debris = !building && !object && (land == 1 || land == 3 || land == 4 || land == 11);
    MapClass::FlashbangWarheadAt(damage, rules.LightningWarhead, at, false, SpotlightFlags::None);
    MapClass::DamageArea(at, damage, nullptr, rules.LightningWarhead, true, Owner);
    point = {};
    const bool changed = building != tile->GetBuilding() ||
                         object != tile->FindTechnoNearestTo(point, false, nullptr) ||
                         level != static_cast<signed char>(tile->Level);
    if (!infantry && (changed || debris) && rules.MetallicDebris.Count) {
        const int count = random.RandomRanged(2, 4);
        for (int i = 0; i < count; ++i)
            lightning_animation(rules.MetallicDebris[random.RandomRanged(0, rules.MetallicDebris.Count - 1)], at);
    }
}
void LightningStorm::Update() {
    // Original 0x0053A6C0 advances nuclear, dominator, then chrono effects.
    if (NukeFlash::Duration != -1 &&
        std::bit_cast<int>(unsigned(NukeFlash::Duration) + unsigned(NukeFlash::StartTime)) < Unsorted::CurrentFrame) {
        if (NukeFlash::IsFadingIn()) NukeFlash::FadeOut();
        else if (NukeFlash::IsFadingOut()) NukeFlash::Status = NukeFlashStatus::Inactive;
    }
    PsyDom::Update();
    ChronoScreenEffect::Update();
    for (int i = BoltsPresent.Count - 1; i >= 0; --i)
        if (BoltsPresent[i]->Animation.Value >= animation_frames(*BoltsPresent[i]) / 2) BoltsPresent.RemoveItem(i);
    for (int i = CloudsManifesting.Count - 1; i >= 0; --i) {
        auto* anim = CloudsManifesting[i];
        if (anim->Animation.Value > animation_frames(*anim) / 2) {
            Strike2(anim->GetCoords());
            CloudsManifesting.RemoveItem(i);
        }
    }
    if (!CloudsPresent.Count) {
        if (TimeToEnd) {
            if (Active) {
                Active = false;
                Owner = nullptr;
                Coords = {-1, -1};
                ScenarioClass::UpdateLighting();
            }
            TimeToEnd = false;
        }
    } else
        for (int i = CloudsPresent.Count - 1; i >= 0; --i)
            if (CloudsPresent[i]->Animation.Value >= animation_frames(*CloudsPresent[i]) - 1)
                CloudsPresent.RemoveItem(i);
    auto& rules = *RulesClass::Instance;
    if (!Active || TimeToEnd) {
        if (Deferment > 0) {
            --Deferment;
            if (!Deferment)
                Start(Duration, 0, Coords, Owner);
            else if (Deferment % 225 == 0 && rules.LightningPrintText) {
                VoxClass::Play("EVA_LightningStormApproaching");
                MessageListClass::Instance.AddMessage(
                    nullptr, 0,
                    StringTable::LoadString("TXT_LIGHTNING_STORM_APPROACHING", nullptr, "D:\\ra2mdpost\\ION.CPP", 974),
                    HouseClass::CurrentPlayer ? HouseClass::CurrentPlayer->ColorSchemeIndex : 3,
                    static_cast<TextPrintType>(70), 150, false);
            }
        }
        return;
    }
    const int frame = Unsorted::CurrentFrame;
    if (Duration != -1 && std::bit_cast<int>(unsigned(Duration) + unsigned(StartTime)) < frame) {
        TimeToEnd = true;
        return;
    }
    if (rules.LightningHitDelay > 0 && frame % rules.LightningHitDelay == 0) Strike(Coords);
    if (rules.LightningScatterDelay <= 0 || frame % rules.LightningScatterDelay) return;
    const int radius = rules.LightningCellSpread >> 1;
    for (int attempt = 0; attempt < 3; ++attempt) {
        CellStruct at = Coords;
        at.X = short(int(at.X) + ScenarioClass::Instance->Random.RandomRanged(-radius, radius));
        at.Y = short(int(at.Y) + ScenarioClass::Instance->Random.RandomRanged(-radius, radius));
        bool close = false;
        for (auto* cloud : CloudsPresent) {
            const auto point = cloud->GetCoords();
            if (std::abs(int(at.X) - short(point.X / 256)) + std::abs(int(at.Y) - short(point.Y / 256)) <
                rules.LightningSeparation) {
                close = true;
                break;
            }
        }
        if (MapClass::Instance.CoordinatesLegal(at) && !close) {
            Strike(at);
            break;
        }
    }
}
void LightningStorm::Init() {
    CloudsPresent.Clear();
    CloudsManifesting.Clear();
    BoltsPresent.Clear();
    Active = TimeToEnd = false;
    Owner = nullptr;
    Coords = {-1, -1};
    Duration = StartTime = -1;
    Deferment = 0;
}
