// Original selection contract: 0x5F6C30 / 0x5F6C70 / 0x5F4520 / 0x5F44A0.
#include "yrpp/ObjectClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/Unsorted.h"
#include "map_world.hpp"
#include "debug/debug_state.hpp"
#include <exception>
#include <new>

bool ObjectClass::CanBeSelected() const {
    if (IsDisguisedAs(HouseClass::CurrentPlayer) && !GetDisguiseHouse(true)) return false;
    return GetType()->Selectable;
}
bool ObjectClass::CanBeSelectedNow() const { return CanBeSelected(); }
bool ObjectClass::IsCrushable(TechnoClass* crusher) {
    const bool techno = (AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None;
    if (crusher && crusher->GetTechnoType()->OmniCrusher && techno && !GetTechnoType()->OmniCrushResistant &&
        WhatAmI() != AbstractType::Building && !crusher->Owner->IsAlliedWith(this) && !IsIronCurtained())
        return true;
    return GetType()->Crushable && techno && !static_cast<TechnoClass*>(this)->Uncrushable &&
           !crusher->Owner->IsAlliedWith(this) && !IsIronCurtained();
}
bool ObjectClass::Select() {
    try {
        if (!Unsorted::ArmageddonMode && (InLimbo || IsSelected || !CanBeSelected())) return false;
        TechnoClass* unit = nullptr;
        switch (WhatAmI()) {
            case AbstractType::Unit:
            case AbstractType::Aircraft:
            case AbstractType::Building:
            case AbstractType::Infantry:
                unit = static_cast<TechnoClass*>(this);
                break;
            default:
                break;
        }
        if (!Unsorted::ArmageddonMode && IsControllable() && unit && unit->IsALoaner) return false;
        if (unit && unit->IsBeingWarpedOut()) return false;
        auto& display = DisplayClass::Instance;
        if (display.CurrentBuildingType) return false;
        auto& selected = CurrentObjects;
        if (selected.Count > 0) {
            auto* owner = GetOwningHouse();
            auto* previous = selected[0]->GetOwningHouse();
            const bool controlled = previous->IsControlledByCurrentPlayer();
            if ((game::debug_selection_expanded() &&
                 (WhatAmI() == AbstractType::Building || selected[0]->WhatAmI() == AbstractType::Building)) ||
                (!game::debug_can_mix_selection(*selected[0], *this) &&
                 (controlled != owner->IsControlledByCurrentPlayer() || !previous->IsControlledByCurrentPlayer())))
                display.UnselectAll();
        }
        const bool front = (AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None &&
                           static_cast<TechnoTypeClass*>(GetType())->HasPrimary;
        bool appended = false;
        try {
            appended = selected.AddItem(this);
        } catch (const std::bad_alloc&) {
            selected.IsInitialized = true;
        }
        if (front && appended) {
            for (int i = selected.Count - 1; i > 0; --i)
                selected.Items[i] = selected.Items[i - 1];
            selected.Items[0] = this;
        }
        // Original growth failure still reaches this non-atomic success tail.
        Game::SetSelectionCommandMode(0);
        IsSelected = true;
        game::map_object_changed();
        return true;
    } catch (...) {
        std::terminate();
    }  // Original virtual callbacks cannot unwind across the host.
}
void ObjectClass::Deselect() {
    if (!IsSelected) return;
    CurrentObjects.Remove(this);
    IsSelected = false;
    auto& display = DisplayClass::Instance;
    // Display.GetFollowObject (0x4AEB10) / SetFollowObject (0x4AEB30).
    if (display.FollowObject && display.ObjectToFollow == this) {
        display.ObjectToFollow = nullptr;
        display.FollowObject = false;
    }
    game::map_object_changed();
}
