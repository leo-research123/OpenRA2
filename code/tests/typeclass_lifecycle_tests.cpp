#include "support/test_support.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/CampaignClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TActionClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <cstring>

namespace {

template<class T> void identity(T& value) {
    EXPECT_TRUE((value.WhatAmI() == T::AbsID)) << "real local virtual identity";
    EXPECT_TRUE((value.Size() == sizeof(T))) << "host object size";
    CLSID clsid{};
    EXPECT_TRUE((value.GetClassID(&clsid) == 0 && value.GetClassID(nullptr) < 0)) << "identity result";
    EXPECT_TRUE((T::Find(value.ID) == &value)) << "real registry identity";
}
void lifetimes() {
    static_assert(std::is_abstract_v<AbstractClass> && std::is_abstract_v<AbstractTypeClass>);
    static_assert(std::is_abstract_v<ObjectTypeClass> && std::is_abstract_v<TechnoTypeClass>);
    const int types = AbstractTypeClass::Array.Count;
    const int listeners = AbstractClass::TypeExpirationListeners.Count;
    {
        InfantryTypeClass infantry("GI"); UnitTypeClass unit("MTNK"); AircraftTypeClass aircraft("ORCA");
        // Explicit fixtures are not asserted to be the missing target fixed-data values.
        const BuildingTypeClass::ConstructionDefaults defaults{{11,22,33},{44,55,66,77}};
        BuildingTypeClass building("POWER", defaults);
        AnimTypeClass anim("EXPLODE"); VoxelAnimTypeClass voxel("DEBRIS"); BulletTypeClass bullet("SHELL");
        ParticleTypeClass particle("SMOKE"); ParticleSystemTypeClass system("SMOKESYS");
        WeaponTypeClass weapon("GUN"); WarheadTypeClass warhead("AP"); SuperWeaponTypeClass super("NUKE");
        HouseTypeClass country("America"); CampaignClass campaign("ALLIED", L"explicit fixture");
        TeamTypeClass team("TEAM"); AITriggerTypeClass ai("AI"); TriggerTypeClass trigger("TRIGGER");
        TagTypeClass tag("TAG"); TiberiumClass tiberium("ORE");
        identity(infantry); identity(unit); identity(aircraft); identity(building); identity(anim);
        identity(voxel); identity(bullet); identity(particle); identity(system); identity(weapon);
        identity(warhead); identity(super); identity(country); identity(campaign); identity(team);
        identity(ai); identity(trigger); identity(tag); identity(tiberium);
        EXPECT_TRUE((AbstractTypeClass::Array.Count == types + 19)) << "base registration";
        EXPECT_TRUE((TechnoTypeClass::Array.Count == 4)) << "four actual concrete techno types";
        EXPECT_TRUE((infantry.Sequence != nullptr)) << "owned sequence constructed";
        for (const auto& sequence : infantry.Sequence->Sequences)
            EXPECT_TRUE((sequence.Facing == static_cast<SequenceFacing>(-1) && sequence.SoundCount == 0)) << "sequence defaults";
        for (int index : unit.TurretWeapon) EXPECT_TRUE((index == -1)) << "all eighteen turret slots initialized";
        EXPECT_TRUE((infantry.SpeedType == static_cast<SpeedType>(0) && unit.SpeedType == static_cast<SpeedType>(-1)
            && aircraft.SpeedType == static_cast<SpeedType>(4))) << "base speed construction argument";
        EXPECT_TRUE((building.HalfDamageSmokeLocation1 == defaults.coordinate && building.MuzzleFlash[9].X == 11)) << "supplied constructor globals propagated";
        EXPECT_TRUE((building.DockingOffsets.Capacity == 1 && building.DockingOffsets[0] == CoordStruct::Empty)) << "building dock vector owns one coordinate";
        for (auto& a : building.BuildingAnim) EXPECT_TRUE((a.Powered)) << "all building animation slots initialized";
        for (auto& p : building.RemoveOccupy) EXPECT_TRUE((p.X == 65535 && p.Y == 65535)) << "occupation sentinel";
        for (double verse : warhead.Verses) EXPECT_TRUE((verse == 1.0)) << "all eleven verses defaults";
        EXPECT_TRUE((country.FirepowerMult == 1.0 && country.ArrayIndex == country.ArrayIndex2 && country.Prefix == 'A')) << "country constructor does not require invented default building";
        EXPECT_TRUE((std::wcscmp(campaign.Description, L"explicit fixture") == 0)) << "campaign caller-provided description";
        EXPECT_TRUE((tiberium.GrowthPercentage == 0.1 && tiberium.SpreadPercentage == 0.1)) << "exact target double defaults";
        EXPECT_TRUE((!tiberium.GrowthLogic.Queue && !tiberium.SpreadLogic.Nodes)) << "empty growth state";
        TEventClass event; TActionClass action;
        EXPECT_TRUE((event.WhatAmI() == AbstractType::Event && action.WhatAmI() == AbstractType::Action)) << "node identities";
        EXPECT_TRUE((TEventClass::Array.Count == 1 && TActionClass::Array.Count == 1)) << "node registration";
        event.TeamType = &team; action.TeamType = &team;
        event.PointerExpired(&team, true); action.PointerExpired(&team, true);
        EXPECT_TRUE((!event.TeamType && !action.TeamType)) << "node type references expire locally";
    }
    EXPECT_TRUE((AbstractTypeClass::Array.Count == types && AbstractClass::TypeExpirationListeners.Count == listeners)) << "all native type registries restored on destruction";
    EXPECT_TRUE((TechnoTypeClass::Array.Count == 0 && TEventClass::Array.Count == 0 && TActionClass::Array.Count == 0)) << "no residual concrete or node registrations";
}
void world_nodes() {
    const int tags=TagClass::Array.Count, triggers=TriggerClass::Array.Count;
    const int pending=AbstractClass::PendingDeletes.Count;
    {
        TagTypeClass type("NODE-OWNER");
        type.FirstTrigger=GameCreate<TriggerTypeClass>("NODE-TRIGGER");
        type.FirstTrigger->Enabled=false;
        {
            TagClass instance(&type, CellStruct{12,34});
            EXPECT_TRUE((instance.Type==&type && instance.DefaultCoords==CellStruct{12,34})) << "explicit tag placement state";
            EXPECT_TRUE((instance.FirstTrigger && instance.FirstTrigger->Type==type.FirstTrigger)) << "real trigger instance chain";
            EXPECT_TRUE((!instance.FirstTrigger->Enabled)) << "disabled trigger needs no invented Scenario";
            EXPECT_TRUE((TagClass::Array.Count==tags+1 && TriggerClass::Array.Count==triggers+1)) << "instance registries";
        }
        EXPECT_TRUE((AbstractClass::PendingDeletes.Count==pending+1)) << "Tag destruction defers trigger deletion";
        auto* trigger=static_cast<TriggerClass*>(AbstractClass::PendingDeletes[pending]);
        AbstractClass::PendingDeletes.Remove(trigger);
        GameDelete(trigger);
    }
    EXPECT_TRUE((TagClass::Array.Count==tags && TriggerClass::Array.Count==triggers &&
        AbstractClass::PendingDeletes.Count==pending)) << "instance and pending-list cleanup";
}
void owned_chains_and_indices() {
    {
        TriggerTypeClass trigger("OWNED");
        trigger.FirstEvent = GameCreate<TEventClass>();
        trigger.FirstEvent->NextEvent = GameCreate<TEventClass>();
        trigger.FirstAction = GameCreate<TActionClass>();
        trigger.FirstAction->NextAction = GameCreate<TActionClass>();
    }
    EXPECT_TRUE((TEventClass::Array.Count == 0 && TActionClass::Array.Count == 0)) << "owned event/action chains released";
    ScriptTypeClass script("SCRIPT");
    auto* first = GameCreate<TeamTypeClass>("FIRST");
    auto* removed = GameCreate<TeamTypeClass>("REMOVED");
    script.ActionsCount = 3;
    script.ScriptActions[0] = {18, 0}; script.ScriptActions[1] = {18, 1}; script.ScriptActions[2] = {18, 2};
    first->ScriptType = &script;
    GameDelete(removed);
    EXPECT_TRUE((script.ScriptActions[0].Argument == 0 && script.ScriptActions[1].Argument == 0 && script.ScriptActions[2].Argument == 1)) << "action 18 indices repaired before TeamType removal";
    first->ScriptType = nullptr; GameDelete(first);
    const auto& runtime = game::native_rules_runtime();
    AbstractTypeClass* value = nullptr;
    EXPECT_TRUE((runtime.resolve(nullptr, AbstractType::InfantryType, "NATIVE", value) && value)) << "native resolver normally constructs infantry";
    EXPECT_TRUE((value->WhatAmI() == AbstractType::InfantryType)) << "no proxy object"; GameDelete(value);
}
}

TEST(TypeclassLifecycle, Contracts) {
    lifetimes(); world_nodes(); owned_chains_and_indices();
}
