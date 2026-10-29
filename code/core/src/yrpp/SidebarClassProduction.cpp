// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 sidebar.cpp Strip/Select production; YR 0x6A6300/0x6A8420.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/RulesClass.h"
#include "type_resources.hpp"
#include <algorithm>
#include <cwchar>
#include <bit>

void SidebarClass::FlashCameo(TechnoTypeClass* type, int duration) {
    int index = BuildingTypeClass::Array.FindItemIndex(static_cast<BuildingTypeClass*>(type));
    if (index < 0) index = UnitTypeClass::Array.FindItemIndex(static_cast<UnitTypeClass*>(type));
    if (index < 0) index = InfantryTypeClass::Array.FindItemIndex(static_cast<InfantryTypeClass*>(type));
    if (index < 0) index = AircraftTypeClass::Array.FindItemIndex(static_cast<AircraftTypeClass*>(type));
    if (index < 0) return;
    const int tab = GetObjectTabIdx(type->WhatAmI(), index, 0);
    if (tab < 0 || tab >= 4) return;
    const unsigned end = unsigned(Unsorted::CurrentFrame) + unsigned(duration);
    for (auto& cameo : Tabs[tab].Cameos)
        if (cameo.ItemIndex == index) {
            cameo.FlashEndFrame = std::bit_cast<int>(end);
            break;
        }
    unknown_53A0 = end + 1u;
}

bool YRPP_STDCALL BuildType::SortsBefore(AbstractType left, int li, AbstractType right, int ri) {
    const auto* a = TechnoTypeClass::GetByTypeAndIndex(left, li);
    const auto* b = TechnoTypeClass::GetByTypeAndIndex(right, ri);
    if (right == AbstractType::None) return true;
    const auto special = [](AbstractType kind) {
        return kind == AbstractType::Special || kind == AbstractType::Super || kind == AbstractType::SuperWeaponType;
    };
    const bool specialA = special(left), specialB = special(right);
    if (specialA && specialB) {
        const auto* x = SuperWeaponTypeClass::Array[li];
        const auto* y = SuperWeaponTypeClass::Array[ri];
        if (x->RechargeTime != y->RechargeTime) return x->RechargeTime < y->RechargeTime;
        return std::wcscmp(x->UIName, y->UIName) <= 0;
    }
    if (specialA || specialB) return specialA;
    if (!a || !b) return false;
    auto* owner = HouseClass::CurrentPlayer;
    if (owner) {
        const bool ownA = a->AIBasePlanningSide == owner->Type->SideIndex,
                   ownB = b->AIBasePlanningSide == owner->Type->SideIndex;
        if (ownA != ownB) return ownA;
    }
    // 0x6A8542: only unit/aircraft types participate in these two flags.
    const bool vehicleA = left == AbstractType::UnitType || left == AbstractType::AircraftType;
    const bool vehicleB = right == AbstractType::UnitType || right == AbstractType::AircraftType;
    const bool airA = vehicleA && a->ConsideredAircraft, navalA = vehicleA && a->Naval;
    const bool airB = vehicleB && b->ConsideredAircraft, navalB = vehicleB && b->Naval;
    const bool groundA = !airA && !navalA, groundB = !airB && !navalB;
    if (groundA) {
        if (airB || navalB) return true;
    } else if (airA) {
        if (groundB) return false;
        if (navalB) return true;
    } else if (navalA && (groundB || airB))
        return false;
    if (a->TechLevel != b->TechLevel) return a->TechLevel < b->TechLevel;
    const int ac = a->GetActualCost(owner), bc = b->GetActualCost(owner);
    if (ac != bc) return ac < bc;
    return std::wcscmp(a->UIName ? a->UIName : L"", b->UIName ? b->UIName : L"") <= 0;
}
#if !defined(RA2_YRPP_GAME)
void StripClass::InsertCameo(AbstractType kind, int index) noexcept {
    const int count = ++CameoCount;
    for (int at = 0; at < count; ++at) {
        if (!BuildType::SortsBefore(kind, index, Cameos[at].ItemType, Cameos[at].ItemIndex)) continue;
        for (int i = count - 1; i > at; --i)
            Cameos[i] = Cameos[i - 1];
        auto& cameo = Cameos[at];
        cameo.ItemType = kind;
        cameo.ItemIndex = index;
        if (kind == AbstractType::BuildingType)
            cameo.IsAlt = static_cast<BYTE>(BuildingTypeClass::Array[index]->BuildCat);
        cameo.CurrentFactory = nullptr;
        cameo.unknown_10 = 0;
        cameo.Progress.Value = 0;
        cameo.Progress.Start(0);
        cameo.FlashEndFrame = 0;
        return;
    }
}
#endif
bool SidebarClass::AddCameo(AbstractType kind, int index) {
    if (Unsorted::ArmageddonMode) return false;
    if (kind == AbstractType::BuildingType) {
        const auto* type = static_cast<BuildingTypeClass*>(TechnoTypeClass::GetByTypeAndIndex(kind, index));
        // These require separate placement protocols in the native runtime.
        if (!type || type->ToTile || type->PowersUpBuilding[0] || type->Gate || type->LaserFence ||
            type->LaserFencePost || type->FirestormWall)
            return false;
    }
    bool wasEmpty = true;
    for (const auto& strip : Tabs)
        if (strip.CameoCount > 0) {
            wasEmpty = false;
            break;
        }
    const int tab = GetObjectTabIdx(kind, index, 0);
    if (tab < 0) return false;
    auto& strip = Tabs[tab];
    // Native storage has exactly 75 slots; do not reproduce the original's
    // off-by-one write when a full strip receives a distinct extra item.
    if (strip.CameoCount >= 75) return false;
    for (int i = 0; i < strip.CameoCount; ++i)
        if (strip.Cameos[i].ItemType == kind && strip.Cameos[i].ItemIndex == index) return false;
    // 0x6A63FD..0x6A641E: submit before insertion; Vox owns request deduplication.
    if (!Unsorted::ScenarioInit && kind != AbstractType::Special && !game::type_resources().audio_unavailable)
        VoxClass::Play("EVA_NewConstructionOptions", -1, -1);
    strip.InsertCameo(kind, index);
    strip.NeedsRedraw = true;
    Instance.SidebarBackgroundNeedsRedraw = true;
    if (TabButtons[tab].Disabled) TabButtons[tab].Enable();
    if (tab == ActiveTabIndex) UpdateScrollButtons();
    Activate(1);
    if (wasEmpty && tab != ActiveTabIndex) SetTab(tab);
    SidebarNeedsRedraw = true;
    MarkNeedsRedraw(0);
    return true;
}
bool SidebarClass::LinkFactory(FactoryClass* factory, AbstractType kind, int index) {
    const int tab = GetObjectTabIdx(kind, index, 0);
    if (tab < 0) return false;
    auto& strip = Tabs[tab];
    for (int i = 0; i < strip.CameoCount; ++i) {
        auto& cameo = strip.Cameos[i];
        if (cameo.ItemType == kind && cameo.ItemIndex == index) {
            cameo.CurrentFactory = factory;
            strip.IsBuilding = strip.NeedsRedraw = true;
            return true;
        }
    }
    return false;
}
bool YRPP_STDCALL SidebarClass::UnlinkFactory(AbstractType kind, int index, FactoryClass* factory) {
    const int tab = GetObjectTabIdx(kind, index, 0);
    if (tab < 0) return false;
    // Strip::Abandon_Production 0x6ABB60 clears a completed building before
    // abandoning its factory, while the product is still a valid original class.
    if (factory && factory->Object && factory->Object->WhatAmI() == AbstractType::Building) {
        auto* product = factory->Object;
        Game::ClearSidebarTabObject(product && product->WhatAmI() == AbstractType::Building ? product : nullptr);
    }
    auto& strip = Instance.Tabs[tab];
    bool changed = false, building = false;
    for (int i = 0; i < strip.CameoCount; ++i) {
        auto& cameo = strip.Cameos[i];
        if (cameo.CurrentFactory == factory) {
            factory->AbandonProduction();
            cameo.CurrentFactory = nullptr;
            cameo.unknown_10 = 0;
            changed = true;
        } else if (cameo.CurrentFactory)
            building = true;
    }
    if (changed) {
        strip.NeedsRedraw = true;
        GScreenClass::Instance.GScreenClass::MarkNeedsRedraw(0);
        Game::SidebarChanged = true;
    }
    if (!building) strip.IsBuilding = false;
    return changed;
}
void SidebarClass::RefreshBuildables() {
    auto* owner = HouseClass::CurrentPlayer;
    if (!owner || Unsorted::ArmageddonMode) return;
    bool activeChanged = false;
    for (auto& strip : Tabs)
        for (int i = strip.CameoCount - 1; i >= 0; --i) {
            const auto& cameo = strip.Cameos[i];
            auto* type = TechnoTypeClass::GetByTypeAndIndex(cameo.ItemType, cameo.ItemIndex);
            const bool lost = cameo.ItemType == AbstractType::Special &&
                              (unsigned(cameo.ItemIndex) >= unsigned(owner->Supers.Count) ||
                               !owner->Supers[cameo.ItemIndex]->IsPresent);
            if (lost || (type && (!type->FindFactory(true, false, false, owner) ||
                                  owner->CanBuild(type, false, true) == CanBuildResult::Unbuildable))) {
                for (int n = i; n + 1 < strip.CameoCount; ++n)
                    strip.Cameos[n] = strip.Cameos[n + 1];
                --strip.CameoCount;
                strip.NeedsRedraw = true;
                // 0x6AA9D6..0x6AAA37 clears the vacated entry. InsertCameo scans this
                // sentinel when the new item sorts after every remaining cameo.
                strip.Cameos[strip.CameoCount] = BuildType(0, AbstractType::None);
                strip.Cameos[strip.CameoCount].Progress.Start(0);
                if (&strip == &Tabs[ActiveTabIndex]) activeChanged = true;
            }
        }
    for (int i = 0; i < 4; ++i) {
        Tabs[i].TopRowIndex =
            std::clamp(Tabs[i].TopRowIndex, 0, std::max(0, (Tabs[i].CameoCount + 1) / 2 - GetUsableCameoCount() / 2));
        if (Tabs[i].CameoCount)
            TabButtons[i].Enable();
        else
            TabButtons[i].Disable();
    }
    if (!Tabs[ActiveTabIndex].CameoCount)
        for (int i = 0; i < 4; ++i)
            if (Tabs[i].CameoCount) {
                SetTab(i);
                break;
            }
    if (activeChanged) {
        UpdateScrollButtons();
        SidebarNeedsRedraw = true;
        MarkNeedsRedraw(0);
    }
}
void SidebarClass::UpdateProduction() {
    auto* owner = HouseClass::CurrentPlayer;
    if (!owner) return;
    // Publish one-time grants too; those do not pass through the building grant loop.
    for (int i = 0; i < owner->Supers.Count; ++i)
        if (owner->Supers[i]->IsPresent && !owner->Supers[i]->Type->PostClick) AddCameo(AbstractType::Special, i);
    bool lost = false;
    for (const auto& strip : Tabs)
        for (int i = 0; i < strip.CameoCount; ++i) {
            const auto& cameo = strip.Cameos[i];
            if (cameo.ItemType == AbstractType::Special &&
                (unsigned(cameo.ItemIndex) >= unsigned(owner->Supers.Count) ||
                 !owner->Supers[cameo.ItemIndex]->IsPresent))
                lost = true;
        }
    if (lost) RefreshBuildables();
    // Strip::AI 0x6A8B30: ready specials and completed buildings flash their tab.
    for (int tab = 0; tab < 2; ++tab) {
        bool flash = false;
        const auto& strip = Tabs[tab];
        for (int i = 0; i < strip.CameoCount; ++i) {
            const auto& cameo = strip.Cameos[i];
            if (cameo.ItemType == AbstractType::Special) {
                if (unsigned(cameo.ItemIndex) < unsigned(owner->Supers.Count) &&
                    owner->Supers[cameo.ItemIndex]->ShouldFlashTab())
                    flash = true;
            } else if (auto* factory = cameo.CurrentFactory) {
                if (factory->IsDone() && factory->Object && factory->Object->WhatAmI() == AbstractType::Building)
                    flash = true;
            }
        }
        if (flash) {
            const int delay = 10 - Unsorted::CurrentFrame % 10;
            TabButtons[tab].StartFlashing(10, delay, (Unsorted::CurrentFrame + delay) / 10 % 2 == 0);
        } else
            TabButtons[tab].StopFlashing();
    }
    // A factory can be linked to several queued cameos; consume its change once.
    for (auto* factory : {owner->Primary_ForAircraft, owner->Primary_ForInfantry, owner->Primary_ForVehicles, owner->Primary_ForShips,
                          owner->Primary_ForBuildings, owner->Primary_ForDefenses}) {
        if (!factory || !factory->HasProgressChanged()) continue;
        SidebarNeedsRedraw = true;
        if (factory->IsDone() && factory->Object) {
            auto* type = factory->Object->GetTechnoType();
            if (type->WhatAmI() == AbstractType::BuildingType) {
                Game::SetSidebarTabObject(factory->Object);
                if (!game::type_resources().audio_unavailable) VoxClass::Play("EVA_ConstructionComplete", -1, -1);
                continue;
            }
            auto* building = type->FindFactory(false, true, false, owner);
            if (!building || (type->WhatAmI() != AbstractType::AircraftType && !building->Type->Naval &&
                              (building->HasAnyLink() || building->GetCurrentMission() == Mission::Unload ||
                               building->QueuedMission == Mission::Unload))) {
                factory->IsDifferent = true;
                continue;
            }
            EventClass event;
            event.Type = EventType::Place;
            event.HouseIndex = static_cast<char>(owner->ArrayIndex);
            event.Frame = Unsorted::CurrentFrame;
            // 0x6A8ED2: mobile completion addresses the active category, not a cameo.
            event.Place = {factory->Object->WhatAmI(), -1, type->Naval, CellStruct::Empty};
            if (!EventClass::OutList.Add(event)) factory->IsDifferent = true;
        }
    }
}
bool SelectClass::Action(GadgetFlag flags, DWORD* key, KeyModifier modifier) {
    if (!Strip) return false;
    const int slot = Index + Strip->TopRowIndex * 2;
    if (slot < 0 || slot >= Strip->CameoCount) return false;
    auto* owner = HouseClass::CurrentPlayer;
    const auto& cameo = Strip->Cameos[slot];
    if (!owner) return false;
    if (cameo.ItemType == AbstractType::Special) {
        // 0x6AAD00 handles specials before resolving a TechnoType.
        auto bits = static_cast<unsigned>(flags) & ~static_cast<unsigned>(GadgetFlag::LeftUp);
        if (cameo.ItemIndex == -1) bits = 0;
        auto& display = DisplayClass::Instance;
        if (bits & static_cast<unsigned>(GadgetFlag::RightPress)) display.CurrentSWTypeIndex = -1;
        if ((bits & static_cast<unsigned>(GadgetFlag::LeftPress)) &&
            unsigned(cameo.ItemIndex) < unsigned(owner->Supers.Count)) {
            if (!game::type_resources().audio_unavailable)
                VocClass::PlayGlobal(RulesClass::Instance->GUIBuildSound, 0x2000, 1.0);
            auto* super = owner->Supers[cameo.ItemIndex];
            if (super->CanFire()) {
                if (super->Type->Action == ::Action::None) {
                    EventClass event(owner->ArrayIndex, EventType::SpecialPlace, super->Type->ArrayIndex,
                                     CellStruct{0, 0});
                    EventClass::OutList.Add(event);
                } else {
                    display.CurrentBuilding = nullptr;
                    display.CurrentBuildingType = nullptr;
                    display.CurrentBuildingOwnerArrayIndex = -1;
                    display.SetActiveFoundation(nullptr);
                    display.SetRepairMode(0);
                    display.SetSellMode(0);
                    display.PowerToggleMode = display.PlanningMode = display.PlaceBeaconMode = false;
                    display.CurrentSWTypeIndex = cameo.ItemIndex;
                    MapClass::UnselectAll();
                    if (!game::type_resources().audio_unavailable) VoxClass::Play("EVA_SelectTarget", -1, -1);
                }
            }
        }
        ControlClass::Action(static_cast<GadgetFlag>(bits), key, KeyModifier(0));
        return true;
    }
    auto* type = TechnoTypeClass::GetByTypeAndIndex(cameo.ItemType, cameo.ItemIndex);
    if (!owner || !type) return false;
    const auto bits = static_cast<unsigned>(flags);
    if (bits & (static_cast<unsigned>(GadgetFlag::LeftPress) | static_cast<unsigned>(GadgetFlag::RightPress))) {
        const auto category = type->WhatAmI() == AbstractType::BuildingType
                                  ? static_cast<BuildingTypeClass*>(type)->BuildCat
                                  : BuildCat(0);
        auto* factory = owner->GetPrimaryFactory(cameo.ItemType, type->Naval, category);
        EventClass event;
        event.HouseIndex = static_cast<char>(owner->ArrayIndex);
        event.Frame = Unsorted::CurrentFrame;
        if (bits & static_cast<unsigned>(GadgetFlag::LeftPress)) {
            if (factory && factory->Object && factory->Object->GetTechnoType() == type && factory->IsDone() &&
                type->WhatAmI() == AbstractType::BuildingType) {
                auto* builder = factory->Object->FindFactory(false, false);
                if (builder) owner->EnterPlacement(builder, static_cast<BuildingClass*>(factory->Object));
                return ControlClass::Action(flags, key, modifier);
            }
            if (type->WhatAmI() == AbstractType::BuildingType && factory && factory->Object &&
                factory->Object->GetTechnoType() != type)
                return true;
            if (owner->CanBuild(type, false, true) != CanBuildResult::Buildable) return true;
            event.Type = EventType::Produce;
            event.Produce = {cameo.ItemType, cameo.ItemIndex, type->Naval};
        } else {
            if (!factory || !factory->CountTotal(type)) return true;
            const bool cancel = factory->IsSuspended || factory->IsQueued(type);
            event.Type = cancel ? EventType::Abandon : EventType::Suspend;
            if (cancel)
                event.Abandon = {cameo.ItemType, cameo.ItemIndex, type->Naval};
            else
                event.Suspend = {cameo.ItemType, cameo.ItemIndex, type->Naval};
        }
        EventClass::OutList.Add(event);
    }
    return ControlClass::Action(flags, key, modifier);
}

// 0x6A5030 and Strip::Init_Clear 0x6A81B0: discard the previous scenario's
// cameos/factory links and scrolling state before reconnecting its buttons.
void SidebarClass::Init_Clear() {
    PowerClass::Init_Clear();
    RepairMode = SellMode = false;
    CurrentSWTypeIndex = -1;
    ToggleRepairButton.IsOn = ToggleSellButton.IsOn = false;
    ToggleRepairButton.IsPressed = ToggleSellButton.IsPressed = false;
    SidebarNeedsRedraw = true;
    unknown_bool_5514 = unknown_bool_5515 = false;
    for (auto& strip : Tabs) {
        strip.IsScrollingDown = strip.IsScrolling = strip.IsBuilding = false;
        strip.Flasher = -1;
        strip.TopRowIndex = strip.Slid = strip.CameoCount = 0;
        strip.NeedsRedraw = true;
        for (auto& cameo : strip.Cameos) {
            cameo = BuildType(0, AbstractType::None);
            cameo.Progress.Start(0);
        }
    }
    ActiveTabIndex = 0;
    unknown_53A0 = 0;
}
