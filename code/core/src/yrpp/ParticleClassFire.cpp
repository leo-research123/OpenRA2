#include "debug/debug_state.hpp"
// YR FireAI 0x0062CB10 and the fire branch of MotionAI 0x0062D5E0.
#include "yrpp/ParticleClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "x87_integer.hpp"
#include <cstdlib>

void ParticleClass::FireAI() {
#if defined(RA2_YRPP_GAME)
    JMP_THIS(0x0062CB10);
#else
    if (!(Speed > 0.0f)) {
        Speed = 0.0f;
        IsToDie = true;
        return;
    }

    // Arithmetic follows the original's steady-state x87 control word, 0x0E7F.
    const double scale = game::x87_truncated_sum(
        game::x87_truncated_product(double(ScenarioClass::Instance->Random.Random() % 10 - 5), 0.02), 1.0);
    // The original spills X/Y to float, but retains Z on the x87 stack.
    const float x = game::x87_double_to_float(game::x87_truncated_product(scale, MovementDirection.X));
    const float y = game::x87_double_to_float(game::x87_truncated_product(scale, MovementDirection.Y));
    const double z = game::x87_truncated_product(scale, MovementDirection.Z);
    unknown_coords_100 = {game::x87_integer(game::x87_truncated_product(x, Speed)),
                          game::x87_integer(game::x87_truncated_product(y, Speed)),
                          game::x87_integer(game::x87_truncated_product(z, Speed))};

    if (static_cast<signed char>(StartStateAI) < static_cast<signed char>(Type->EndStateAI)) {
        const int interval = static_cast<signed char>(StateAIAdvance) + (UniqueID & 1);
        const int elapsed = std::bit_cast<int>(unsigned(Type->MaxEC) - unsigned(RemainingEC) + unsigned(UniqueID));
        // Preserve fail-fast division semantics without host integer UB.
        if (!interval || (elapsed == (-0x7FFFFFFF - 1) && interval == -1)) std::abort();
        if (elapsed % interval == 0) {
            ++StartStateAI;
            if (static_cast<signed char>(StartStateAI) == int(Type->Translucent25State)) Translucency = 25;
            if (static_cast<signed char>(StartStateAI) == int(Type->Translucent50State)) Translucency = 50;
        }
        if (StartStateAI == Type->EndStateAI && Type->DeleteOnStateLimit) {
            IsToDie = true;
            return;
        }
    }

    Speed = game::x87_double_to_float(game::x87_truncated_difference(Speed, Type->Deacc));
    if (--RemainingDC || !Type->Damage ||
        static_cast<signed char>(StartStateAI) > static_cast<signed char>(Type->FinalDamageState))
        return;
    RemainingDC = WORD(Type->MaxDC);

    auto* cell = MapClass::Instance.GetCellAt(Location);
    const bool aboveBridge =
        cell->ContainsBridge() && Location.Z >= (int(cell->Level) + CellClass::BridgeLevels) * Unsorted::LevelHeight;
    for (auto* object = aboveBridge ? cell->AltObject : cell->FirstObject; object; object = object->NextObject) {
        CoordStruct buffer;
        const auto origin = *GetCoords(&buffer);
        const auto delta = origin - object->Location;
        const double squared = double(delta.X) * delta.X + double(delta.Z) * delta.Z + double(delta.Y) * delta.Y;
        const int distance = game::x87_integer(Math::sqrt(squared));
        int damage =
            game::debug_attack_damage(ParticleSystem ? ParticleSystem->Owner : nullptr, Type->Damage, Type->Warhead);
        if (object->Health > 0 && object->IsAlive && object != ParticleSystem->Owner) {
            object->ReceiveDamage(&damage, distance / 10, Type->Warhead, nullptr, false, false, nullptr);
        }
    }
#endif
}

void ParticleClass::FireMotionAI() {
    auto at = Location;
    if (Speed > 0.0f) {
        at += unknown_coords_100;
        const int floor = MapClass::Instance.GetCellFloorHeight(Location);
        if (MapClass::Instance.GetCellFloorHeight(at) > floor) {
            unknown_12D = 1;
            IsToDie = true;
        }
    }
    SetLocation(at);
}
