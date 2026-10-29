// Scenario [Aircraft] reader, YR 0x0041B110.
#include "yrpp/AircraftClass.h"
#include "techno_spatial.hpp"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MissionClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include "x87_double.hpp"
#include "x87_integer.hpp"
#include <cstdlib>
#include <cstring>

#if !defined(RA2_YRPP_GAME)
namespace {
const char* next_field() { return std::strtok(nullptr, ","); }
// The original passes the mandatory fields to atoi unchecked and faults on a
// short record; a missing field reads as zero here instead.
int number(const char* field) { return field ? std::atoi(field) : 0; }
}

void YRPP_FASTCALL AircraftClass::ReadINI(CCINIClass* ini) {
    ini->Reset();
    const int count = ini->GetKeyCount("Aircraft");
    for (int i = 0; i < count; ++i) {
        char text[128];
        ini->ReadString("Aircraft", ini->GetKeyName("Aircraft", i), nullptr, text, sizeof(text));
        const int house = HouseClass::FindIndexByName(std::strtok(text, ","));
        if (house == -1) continue;
        const int type = AircraftTypeClass::FindIndex(next_field());
        if (type == -1) continue;
        auto* aircraft = GameCreate<AircraftClass>(AircraftTypeClass::Array[type], HouseClass::Array[house]);
        if (!aircraft) continue;
        const int health = number(next_field());
        CellStruct cell;
        if (ScenarioClass::NewINIFormat < 4) {
            // Pre-4 maps pack the cell as X + Y * 128.
            const int packed = number(next_field());
            cell = {short(packed % 128), short(packed / 128)};
        } else {
            const short x = short(number(next_field()));
            cell = {x, short(number(next_field()))};
        }
        const CoordStruct coords{cell.X * 256 + 128, cell.Y * 256 + 128, 0};
        const int facing = number(next_field());
        const Mission mission = MissionControlClass::FindIndex(next_field());
        if (auto* tag_type = TagTypeClass::FindByNameOrID(next_field()))
            if (auto* tag = TagClass::GetInstance(tag_type)) aircraft->AttachTrigger(tag);
        if (const char* field = next_field())
            aircraft->Veterancy.Veterancy =
                game::x87_double_to_float(game::x87_multiply(double(std::atoi(field)), 0.01));
        if (const char* field = next_field()) aircraft->Group = std::atoi(field);
        if (const char* field = next_field()) aircraft->RecruitableA = std::atoi(field) != 0;
        if (const char* field = next_field()) aircraft->RecruitableB = std::atoi(field) != 0;
        if (!aircraft->Unlimbo(coords, static_cast<DirType>(facing))) {
            GameDelete(aircraft);
            continue;
        }
        const int strength = aircraft->Type->Strength;
        aircraft->Health = game::x87_integer(double(health) * double(strength) * 0.00390625);
        if (aircraft->Health > strength - 3) aircraft->Health = strength;
        if (!aircraft->Health) aircraft->Health = 1;
        aircraft->EstimatedHealth = aircraft->Health;
        game::techno_spatial_dirty(aircraft);
        aircraft->QueueMission(mission, false);
        aircraft->NextMission();
    }
}
#endif
