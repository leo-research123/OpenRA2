#include "tactical_spatial.hpp"
#include "techno_spatial.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 display.cpp::Submit/Remove; YR 0x4A9720/0x4A9770.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DisplayClass.h"
#include "yrpp/ObjectClass.h"
#include <cstdlib>

void YRPP_STDCALL DisplayClass::Remove(ObjectClass* object) {
    if (!object || object->LastLayer == Layer::None) return;
    const int previous = static_cast<int>(object->LastLayer);
    if (previous < 0 || previous >= 5) std::abort();
    auto& layer = ObjectsInLayers[previous];
    const int index = layer.FindItemIndex(object);
    if (index != -1 && index < layer.Count && layer.RemoveItem(index)) {
        game::tactical_spatial_remove(&layer, index);
        object->LastLayer = Layer::None;
        game::techno_spatial_dirty(object);
    }
    if (object->LastLayer != Layer::None) {
        // Only a stale remembered layer triggers the full duplicate sweep.
        for (auto& candidate : ObjectsInLayers) {
            for (int i = candidate.FindItemIndex(object); i != -1 && i < candidate.Count;
                 i = candidate.FindItemIndex(object)) {
                candidate.RemoveItem(i);
                game::tactical_spatial_remove(&candidate, i);
            }
        }
        object->LastLayer = Layer::None;
        game::techno_spatial_dirty(object);
    }
}
void YRPP_STDCALL DisplayClass::Submit(ObjectClass* object) {
    if (!object) return;
    if (object->LastLayer != Layer::None) Remove(object);
    const auto next = object->InWhichLayer();
    if (next == Layer::None) return;
    const int index = static_cast<int>(next);
    if (index < 0 || index >= 5) std::abort();
    if (ObjectsInLayers[index].LayerClass::AddObject(object, next == Layer::Ground)) object->LastLayer = next;
    game::techno_spatial_dirty(object);
}
