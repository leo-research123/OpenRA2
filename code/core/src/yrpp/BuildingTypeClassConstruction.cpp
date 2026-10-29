// YR 0x45E880 / 0x465960 / 0x465AF0: production objects and buildup ownership.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/RulesClass.h"
#include "type_resources.hpp"
#include "x87_integer.hpp"
#include <bit>

ObjectClass* BuildingTypeClass::CreateObject(HouseClass* owner) {
    // Allocation failure is an empty Factory product, not an ABI exception.
    try {
        return GameCreate<BuildingClass>(this, owner);
    } catch (...) {
        return nullptr;
    }
}

SHPStruct* BuildingTypeClass::LoadBuildup() {
    if (Buildup || !DemandLoadBuildup || !BuildupFile[0]) return Buildup;
    try {
        char filename[260]{};
        if (!game::building_shape_filename(BuildupFile, false, filename, sizeof(filename))) return nullptr;
        if (!game::load_owned_type_shape(filename, Buildup)) return nullptr;
        if (!Buildup) {
            filename[1] = 'G';
            if (!game::load_owned_type_shape(filename, Buildup)) return nullptr;
        }
    } catch (...) {
        return nullptr;
    }
    if (Buildup) {
        BuildupLoaded = true;
        auto& sequence = BuildingAnimFrame[0];
        sequence.dwUnknown = 0;
        sequence.FrameCount = Gate ? std::bit_cast<int>(DWORD(GateStages) + 1) : Buildup->Frames / 2;
        sequence.FrameDuration =
            sequence.FrameCount > 0
                ? game::x87_scaled_quotient_to_integer(RulesClass::Instance->BuildupTime, 900.0, sequence.FrameCount)
                : 1;
    }
    return Buildup;
}

void BuildingTypeClass::ClearBuildUp() {
    if (FreeBuildup && Buildup) {
        // The native loader allocates this type-owned SHP in the YR domain.
        YRMemory::Deallocate(Buildup);
        Buildup = nullptr;
        BuildupLoaded = false;
    }
}
