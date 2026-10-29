#include "debug/debug_state.hpp"
// YR 0x0062BD50 / 0x0062D2A0 / 0x0062D5E0.
#include "yrpp/ParticleClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>

void ParticleClass::GasAI() {
    if (!(Unsorted::CurrentFrame & 1)) {
        auto& random = ScenarioClass::Instance->Random;
        const auto absolute = [](int v) { return std::bit_cast<int>(v < 0 ? 0u - unsigned(v) : unsigned(v)); };
        if ((absolute(random.Random()) & 7) == 0) {
            const bool y = absolute(random.Random()) & 1;
            const int delta = absolute(random.Random()) % 3 - 1;
            (y ? Velocity.Y : Velocity.X) += delta;
            Velocity.X = std::clamp(Velocity.X, -2, 2);
            Velocity.Y = std::clamp(Velocity.Y, -2, 2);
        }
        GasVelocity = {0, 0, float(-2.0 - RulesClass::Instance->Gravity)};
        Vector3D<float> candidate{float(Location.X), float(Location.Y), float(Location.Z)};
        candidate += GasVelocity;
        const CoordStruct at{rule_integer(candidate.X), rule_integer(candidate.Y), rule_integer(candidate.Z)};
        auto& map = MapClass::Instance;
        auto* cell = map.GetCellAt(at);
        const int floor = map.GetCellFloorHeight(at), deck = floor + CellClass::BridgeHeight;
        const bool bridge = (unsigned(cell->Flags) & 0x100u) || (unsigned(map.GetCellAt(Location)->Flags) & 0x100u);
        const bool top = bridge && at.Z < deck && Location.Z >= deck,
                   bottom = bridge && at.Z >= deck && Location.Z < deck;
        bool obstacle = false;
        if (!top && !bottom && candidate.Z >= floor && double(candidate.Z) - 150.0 < floor) {
            auto* building = cell->GetBuilding();
            obstacle = building || cell->ConnectsToOverlay(-1, -1);
            if (building && ((building->Type->LaserFence && building->LaserFenceFrame >= 8) || building->IsStrange()))
                obstacle = false;
        }
        if (candidate.Z < floor || top || bottom || obstacle) {
            const auto& slope = Matrix3D::VoxelRampMatrix[cell->SlopeIndex];
            const auto inverse = Matrix3D::TransposeMatrix(slope);
            auto velocity = inverse.RotateVector({GasVelocity.X, -GasVelocity.Y, GasVelocity.Z});
            velocity.Z = -velocity.Z;
            velocity = slope.RotateVector(velocity);
            GasVelocity = {velocity.X, -velocity.Y, velocity.Z};
        }
        if (GetHeight() > 5)
            Velocity.Z = std::max(-5, Velocity.Z - 1);
        else
            Velocity.Z = std::max(0, Velocity.Z);
    }
    if (!--RemainingDC && Type->Damage) {
        RemainingDC = WORD(Type->MaxDC);
        for (auto* object = MapClass::Instance.GetCellAt(Location)->FirstObject; object;) {
            auto* next = object->NextObject;
            if (object->IsAlive && object->Health > 0) {
                const int distance =
                    std::abs(Location.X - object->Location.X) + std::abs(Location.Y - object->Location.Y);
                int damage = game::debug_attack_damage(ParticleSystem ? ParticleSystem->Owner : nullptr, Type->Damage,
                                                       Type->Warhead);
                object->ReceiveDamage(&damage, TacticalClass::AdjustForZ(distance), Type->Warhead, nullptr, false,
                                      false, ParticleSystem ? ParticleSystem->OwnerHouse : nullptr);
            }
            object = next;
        }
    }
    const int interval = static_cast<signed char>(Type->StateAIAdvance) + (UniqueID & 1);
    if (interval && (Type->MaxEC - int(RemainingEC) + static_cast<int>(UniqueID)) % interval == 0) ++StartStateAI;
    if (StartStateAI == Type->EndStateAI) {
        if (Type->DeleteOnStateLimit)
            IsToDie = true;
        else
            StartStateAI = 0;
    }
}
void ParticleClass::GasMotionAI() {
    constexpr int windX[]{0, 2, 2, 1, 0, -2, -2, -2}, windY[]{-2, -2, 0, 2, 2, 2, 0, -2};
    auto at = Location;
    auto& map = MapClass::Instance;
    const int interval = Type->WindEffect > 0 ? 10 / Type->WindEffect : 0;
    if (interval > 0 && Unsorted::CurrentFrame % interval == 0) {
        const int wind = RulesClass::Instance->WindDirection & 7;
        at.X += windX[wind];
        at.Y += windY[wind];
    }
    if (Unsorted::CurrentFrame & 1) {
        at.X = rule_integer(double(at.X) + GasVelocity.X);
        at.Y = rule_integer(double(at.Y) + GasVelocity.Y);
        const int floor = map.GetCellFloorHeight(at);
        if (at.Z > floor + 5) at.Z -= std::min(2, at.Z - floor - 5);
        at += Velocity;
        at.Z = std::max(at.Z, map.GetCellFloorHeight(at) + 5);
    }
    SetLocation(at);
}
void ParticleClass::MotionAI() {
    switch (Type->BehavesLike) {
        case ParticleBehavior::Gas:
            GasMotionAI();
            break;
        case ParticleBehavior::Smoke:
            SmokeMotionAI();
            break;
        case ParticleBehavior::Fire:
            FireMotionAI();
            break;
        // SparkAI and RailgunAI already update their positions in BehaviorAI.
        case ParticleBehavior::Spark:
        case ParticleBehavior::Railgun:
            break;
        default:
            break;
    }
}
