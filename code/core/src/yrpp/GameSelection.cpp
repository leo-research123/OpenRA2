// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 init.cpp selection commands; YR-specific UICommands helpers
// calibrated to 0x00732600/0x007327D0, 0x00732770, and the type-selection
// state machine at 0x00732950..0x00732D10. Uses the original selection arrays.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/SessionClass.h"
#include "debug/debug_state.hpp"
#include <bit>
#include <cstring>
#include <cstdint>
#include <new>

void YRPP_FASTCALL Game::SetSelectionCommandMode(int mode) noexcept {
    SelectionCommandMode = mode;
    AttackMoveMode = false;
}
#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<TechnoClass*>* selection_candidates = nullptr;
}
DynamicVectorClass<TechnoClass*>*& Game::SelectionCandidates = selection_candidates;
void YRPP_FASTCALL Game::PointerGotInvalid(AbstractClass* object) noexcept {
    if (!SelectionCandidates) return;
    // 0x733160 searches backwards and removes only the last matching slot.
    for (int i = SelectionCandidates->Count - 1; i >= 0; --i)
        if ((*SelectionCandidates)[i] == object) {
            SelectionCandidates->RemoveItem(i);
            return;
        }
}
bool Game::IsTypeSelecting() noexcept { return TypeSelectionActive; }
#endif
namespace {
bool controlled_alive(TechnoClass* unit) {
    if (!unit || !unit->IsAlive) return false;
    if (game::debug_selection_expanded())
        return unit->WhatAmI() != AbstractType::Building && !unit->InLimbo && game::debug_can_select_house(unit->Owner);
    const bool controlled = static_cast<int>(SessionClass::Instance.GameMode) != 0
                                ? unit->Owner->IsControlledByCurrentPlayer()
                                : unit->Owner->IsInPlayerControl;
    return controlled;
}
bool matches(TechnoClass* unit, const char* name) {
    return controlled_alive(unit) && !_strcmpi(name, unit->GetType()->ID);
}
template <class T>
void append(DynamicVectorClass<T>& list, T value) {
    try {
        list.AddItem(value);
    } catch (const std::bad_alloc&) {
        list.IsInitialized = true;
    }
}
DynamicVectorClass<TechnoClass*>* collect(const char* name) {
    // The original dereferences this allocation even when it fails. Native
    // allocation failure therefore remains fatal at the noexcept entry rather
    // than propagating an exception or reporting a successful empty selection.
    auto* found = new DynamicVectorClass<TechnoClass*>;
    const auto append = [&](TechnoClass* unit) {
        try {
            found->AddItem(unit);
        }
        // Original vector growth returns false and retains its old storage;
        // restore the flag that SetCapacity clears before a native allocation.
        catch (const std::bad_alloc&) {
            found->IsInitialized = true;
        }
    };
    if (Game::TypeSelectionIncludesMap) {
        const int count = TechnoClass::Array.Count;
        for (int i = 0; i < count; ++i) {
            auto* unit = TechnoClass::Array[i];
            if (name ? matches(unit, name) : controlled_alive(unit)) append(unit);
        }
    } else {
        const int count = TacticalClass::Instance->SelectableCount;
        for (int i = 0; i < count; ++i) {
            auto* object = TacticalClass::SelectableObjects[i].Object;
            if (object && (object->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None &&
                (name ? matches(static_cast<TechnoClass*>(object), name)
                      : controlled_alive(static_cast<TechnoClass*>(object))))
                append(static_cast<TechnoClass*>(object));
        }
    }
    return found;
}
bool contains(const DynamicVectorClass<const char*>& names, const char* name) {
    for (int i = 0; i < names.Count; ++i)
        if (!std::strcmp(name, names[i])) return true;
    return false;
}
DynamicVectorClass<const char*>* selected_types() {
    auto* names = new DynamicVectorClass<const char*>;
    const int count = ObjectClass::CurrentObjects.Count;
    for (int i = 0; i < count; ++i) {
        auto* object = ObjectClass::CurrentObjects[i];
        const char* name = object->GetType()->ID;
        if (contains(*names, name)) continue;
        append(*names, name);
        if (object->GetTechnoType()->DeploysInto) {
            name = object->GetTechnoType()->DeploysInto->ID;
            if (!contains(*names, name)) append(*names, name);
        }
        if (object->GetTechnoType()->UndeploysInto) {
            name = object->GetTechnoType()->UndeploysInto->ID;
            if (!contains(*names, name)) append(*names, name);
        }
    }
    return names;
}
bool matches_types(TechnoClass* unit, const DynamicVectorClass<const char*>& names) {
    const int count = names.Count;
    for (int i = 0; i < count; ++i)
        if (!_strcmpi(unit->GetType()->ID, names[i])) {
            if (unit->CanBeSelectedNow() ||
                (unit->WhatAmI() == AbstractType::Building && unit->GetTechnoType()->UndeploysInto))
                return true;
        }
    return false;
}
// Tactical's sole foreign-selection cleanup, 0x6DA770.
void deselect_foreign() {
    if (game::debug_selection_expanded()) return;
    if (ObjectClass::CurrentObjects.Count != 1) return;
    auto* object = ObjectClass::CurrentObjects[0];
    if (!object->GetOwningHouse()) return;
    const bool controlled = static_cast<int>(SessionClass::Instance.GameMode) != 0
                                ? object->GetOwningHouse()->IsControlledByCurrentPlayer()
                                : object->GetOwningHouse()->IsInPlayerControl;
    if (!controlled) object->Deselect();
}
}
#if !defined(RA2_YRPP_GAME)
void Game::BeginTypeSelection() noexcept {
    if (TypeSelectionActive) return;
    TypeSelectionActive = true;
    TypeSelectionStarted = SystemTimer::GetMilliseconds();
}
void Game::EndTypeSelection() noexcept {
    if (!TypeSelectionActive) return;
    TypeSelectionActive = false;
    const std::uint32_t elapsed = SystemTimer::GetMilliseconds() - TypeSelectionStarted;
    const std::uint32_t magnitude = (elapsed & 0x80000000u) ? 0u - elapsed : elapsed;
    if (std::bit_cast<std::int32_t>(magnitude) <= 500) {
        TypeSelect();
        MapClass::Instance.GScreenClass::MarkNeedsRedraw(1);
    }
}
void Game::TypeSelect() noexcept {
    if (Unsorted::MuteSWLaunches) return;
    deselect_foreign();
    if (SelectionCommandMode != 2) TypeSelectionIncludesMap = false;
    auto* names = selected_types();
    auto* candidates = collect(nullptr);
    if (!TypeSelectionIncludesMap) {
        bool remaining = false;
        const int count = candidates->Count;
        for (int i = 0; i < count; ++i)
            if (matches_types((*candidates)[i], *names) && !(*candidates)[i]->IsSelected) {
                remaining = true;
                break;
            }
        if (!remaining) {
            TypeSelectionIncludesMap = true;
            delete candidates;
            candidates = collect(nullptr);
        }
    }
    const int count = candidates->Count;
    for (int i = 0; i < count; ++i)
        if (matches_types((*candidates)[i], *names)) (*candidates)[i]->Select();
    const char* message = !ObjectClass::CurrentObjects.Count ? "MSG:NothingSelected"
                          : TypeSelectionIncludesMap         ? "MSG:SelAcrossMap"
                                                             : "MSG:SelAcrossScreen";
    ShowMessage(StringTable::LoadString(message), 240);
    SetSelectionCommandMode(2);
    delete names;
    delete candidates;
}
void YRPP_FASTCALL Game::UICommands_TypeSelect_7327D0(const char* name) noexcept {
    const bool voice = Unsorted::MoveFeedback;
    Unsorted::MoveFeedback = false;
    auto* found = collect(name);
    const int count = found->Count;
    for (int i = 0; i < count; ++i)
        found->Items[i]->Select();
    delete found;
    Unsorted::MoveFeedback = voice;
}
#endif
void YRPP_FASTCALL Game::UICommands_TypeDeselect(const char* name) noexcept {
    auto* found = collect(name);
    const int count = found->Count;
    for (int i = 0; i < count; ++i)
        found->Items[i]->Deselect();
    delete found;
}
