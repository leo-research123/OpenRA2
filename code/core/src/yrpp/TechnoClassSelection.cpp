// Original Techno selection override and its ordered side effects (0x6FBFA0).
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/Unsorted.h"
#include "map_runtime.hpp"
#include "debug/debug_state.hpp"
#include <exception>

bool TechnoClass::Select() {
    try {
        const auto& runtime = game::map_runtime();
        if (!game::debug_fog_hidden() && !DiscoveredByCurrentPlayer && !Owner->IsControlledByCurrentPlayer() &&
            runtime.has_window && runtime.has_window())
            return false;
        if (!ObjectClass::Select()) return false;
        if (AttachedTag)
            AttachedTag->RaiseEvent(static_cast<TriggerEvent>(0x21), this, CellStruct::Empty, false, nullptr);
        if ((Owner->IsControlledByCurrentPlayer() || Owner->Type->MultiplayPassive) && Unsorted::MoveFeedback)
            VoiceSelect();
        Game::PlanningManager_Select(this);
        return true;
    } catch (...) {
        std::terminate();
    }
}
