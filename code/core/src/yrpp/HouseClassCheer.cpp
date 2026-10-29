// Original house cheer dispatch, 0x0050C8C0.
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include <cstdlib>

void HouseClass::Cheer() noexcept {
    try {
        const int count = TechnoClass::Array.Count;
        for (int i = 0; i < count; ++i)
            if (auto* object = TechnoClass::Array[i]; object && object->Owner == this) object->Cheer(false);
        VocClass::PlayGlobal(RulesClass::Instance->CheerSound, 0x2000, 1.0f, nullptr);
    } catch (...) {
        std::abort();
    }
}
