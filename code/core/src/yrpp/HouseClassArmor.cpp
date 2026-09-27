// Original HouseClass::GetArmorMultiplier, YR 0x50BD30.
#include "yrpp/HouseClass.h"
#include <bit>
#include "yrpp/BuildingClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/TagClass.h"
#include "type_resources.hpp"
#include "scenario_runtime.hpp"

// YR 0x4F93E0: notification and attached-house triggers, not AI recruitment.
void HouseClass::BuildingUnderAttack(BuildingClass* building) {
    if(building&&building->IsStrange())return;
    const auto& runtime=game::scenario_runtime();const bool campaign=runtime.session_mode(runtime.context)==int(GameMode::Campaign);
    const bool ours=IsControlledByCurrentPlayer();
    const bool ally=CurrentPlayer&&CurrentPlayer!=this&&IsAlliedWith(CurrentPlayer)
        &&(campaign||!building->Owner->Type->MultiplayPassive);
    if(building&&(ours||ally)) {
        const bool miner=ours&&building->Type->UndeploysInto&&building->Type->ResourceGatherer;
        const auto event=miner?RadarEventType::HarvesterAttacked:ours?RadarEventType::BaseAttacked:RadarEventType::AllyBaseAttacked;
        if(RadarEventClass::Create(event,building->GetMapCoords())&&!game::type_resources().audio_unavailable) {
            VoxClass::Play(miner?"EVA_OreMinerUnderAttack":ours?"EVA_OurBaseIsUnderAttack":"EVA_OurAllyIsUnderAttack");
            if(!miner)VocClass::PlayGlobal(RulesClass::Instance->BaseUnderAttackSound,0x2000,1.0f,nullptr);
        }
    }
    for(auto* tag:RelatedTags)tag->RaiseEvent(static_cast<TriggerEvent>(6),nullptr,CellStruct::Empty,false,nullptr);
}

void HouseClass::RegisterDamage(int amount,HouseClass* source) {
    for(auto& node:AngerNodes)if(node.House==source)
        node.AngerLevel=std::bit_cast<int>(unsigned(node.AngerLevel)+unsigned(amount));
    int highest=0;HouseClass* enemy=nullptr;
    for(const auto& node:AngerNodes)if(node.AngerLevel>highest && !node.House->Defeated && !IsAlliedWith(node.House)) {
        highest=node.AngerLevel;enemy=node.House;
    }
    EnemyHouseIndex=enemy?enemy->ArrayIndex:-1;
}
#include "yrpp/HouseTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
double HouseClass::GetArmorMultiplier(TechnoTypeClass* type) {
    switch(type->WhatAmI()) {
    case AbstractType::AircraftType:return Type->ArmorAircraftMult;
    case AbstractType::BuildingType:return static_cast<BuildingTypeClass*>(type)->BuildCat==BuildCat::Combat?Type->ArmorDefensesMult:Type->ArmorBuildingsMult;
    case AbstractType::InfantryType:return Type->ArmorInfantryMult;
    case AbstractType::UnitType:return Type->ArmorUnitsMult;
    default:return 1.0;
    }
}
