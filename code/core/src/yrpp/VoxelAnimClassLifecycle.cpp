// YR 0x007493B0 / 0x007499F0. Bounce state uses the existing original class.
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/VocClass.h"
#include "RulesClassReaders.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>

namespace {
DynamicVectorClass<VoxelAnimClass*> objects;
}
DynamicVectorClass<VoxelAnimClass*>& VoxelAnimClass::Array = objects;
HRESULT YRPP_STDCALL VoxelAnimClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr unsigned data[]{0x0E2731C1u, 0x11D19C0Fu, 0xA00009B7u, 0xD1AFDD24u};
    static_assert(sizeof(data) == sizeof(CLSID));
    std::memcpy(output, data, sizeof(data));
    return 0;
}
VoxelAnimClass::VoxelAnimClass(VoxelAnimTypeClass* type, CoordStruct* location, HouseClass* house) noexcept
    : ObjectClass(), Bounce{}, unknown_int_100(-1), Type(type), AttachedSystem(nullptr), OwnerHouse(house),
      TimeToDie(false), Audio3{}, Audio4{}, Invisible(false), Duration(type->Duration) {
    Create_ID();
    if (!Array.AddItem(this)) std::abort();
    auto& random = ScenarioClass::Instance->Random;
    const auto absolute = [](int v) { return std::bit_cast<int>(v < 0 ? 0u - unsigned(v) : unsigned(v)); };
    const auto modulo = [](int v, int divisor) { return divisor ? v % divisor : 0; };
    Vector3D<float> velocity;
    CoordStruct start;
    double angular;
    if (!Type->IsMeteor) {
        ObjectClass::Unlimbo(*location, DirType::North);
        start = GetCoords();
        start.Z += 10;
        const int spin = random.Random(), z = random.Random(), y = random.Random(), x = random.Random();
        const int xyRange = rule_integer(Type->MaxXYVel * 2.0);
        velocity.Y = float(modulo(absolute(y), xyRange) - Type->MaxXYVel);
        velocity.Z = float(modulo(absolute(z), rule_integer(Type->MaxZVel - Type->MinZVel + 1.0)) + Type->MinZVel);
        velocity.X = float(modulo(absolute(x), xyRange) - Type->MaxXYVel);
        angular = modulo(absolute(spin), rule_integer(Type->MaxAngularVelocity - Type->MinAngularVelocity + 1.0)) +
                  Type->MinAngularVelocity;
    } else {
        const int y = random.Random(), x = random.Random(), range = rule_integer(Type->MinZVel);
        velocity.Y = float(modulo(absolute(y), range) - Type->MaxXYVel);
        velocity.X = float(modulo(absolute(x), range) - Type->MaxXYVel);
        velocity.Z = float(Type->MinZVel);
        if (velocity.X < -velocity.Y) {
            velocity.X = -velocity.X;
            velocity.Y = -velocity.Y;
        }
        Duration -= absolute(random.Random() % 20);
        start = {rule_integer(double(location->X) - double(Duration) * velocity.X),
                 rule_integer(double(location->Y) - double(Duration) * velocity.Y),
                 rule_integer(double(location->Z) - double(Duration) * velocity.Z)};
        ObjectClass::Unlimbo(start, DirType::North);
        angular =
            modulo(absolute(random.Random()), rule_integer(Type->MaxAngularVelocity - Type->MinAngularVelocity + 1.0)) +
            Type->MinAngularVelocity;
        start = GetCoords();
    }
    Bounce.Initialize(start, Type->Elasticity, 1.4, 0, velocity, angular);
    if (Type->StartSound != -1)
        VocClass::PlayAt(Type->StartSound, *location, &Audio3);
    else
        Audio3.EndLooping();
    Audio4.End();
    if (Type->AttachedSystem)
        if (auto* memory = YRMemory::Allocate(sizeof(ParticleSystemClass)))
            AttachedSystem = ::new (memory)
                ParticleSystemClass(Type->AttachedSystem, *location, MapClass::Instance.GetCellAt(*location), this,
                                    CoordStruct::Empty, nullptr);
}
VoxelAnimClass::~VoxelAnimClass() {
    NotifyObjectExpired(true);
    Array.Remove(this);
    if (AttachedSystem) {
        AttachedSystem->UnInit();
        AttachedSystem = nullptr;
    }
    ObjectClass::Limbo();
    while (PendingDeletes.Remove(this)) {
    }
    Audio3.EndLooping();
    Audio4.EndLooping();
    Audio3.~AudioController();
    Audio4.~AudioController();
    Type = nullptr;
}
