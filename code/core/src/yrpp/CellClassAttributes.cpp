// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 cell.cpp Fixup_LAT/Recalc_Attributes; YR 0x47CA80/0x47D2B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"

void CellClass::UpdateThreat(unsigned int sourceHouse,int amount) {
    const int region=MapClass::CellRegion(MapCoords);
    for(int i=0;i<HouseClass::Array.Count;++i) {
        auto* house=HouseClass::Array[i];
        if(house && static_cast<unsigned>(house->ArrayIndex)!=sourceHouse
            && !house->IsAlliedWith(static_cast<int>(sourceHouse)))house->AdjustThreat(region,amount);
    }
}

void CellClass::RevealCellObjects() {
    for(auto* object=FirstObject;object;object=object->NextObject)object->Reveal();
}

void CellClass::ActivateVeins() {
    if(OverlayTypeIndex!=126 || OverlayData<0x30 || SlopeIndex || (static_cast<unsigned>(Flags)&0x20000u))return;
    for(auto* object=FirstObject;object;object=object->NextObject) {
        if(object->GetHeight()>5 || !(static_cast<unsigned>(object->AbstractFlags)&1u))continue;
        auto* techno=static_cast<TechnoClass*>(object);
        if(techno->GetTechnoType()->ImmuneToVeins || techno->HasAbility(Ability::VeinProof))continue;
        GameCreate<AnimClass>(RulesClass::Instance->VeinAttack,
            CoordStruct{MapCoords.X*256,MapCoords.Y*256,GetFloorHeight({128,128})},0,1,0x600,0,false);
        Flags=static_cast<CellFlags>(static_cast<unsigned>(Flags)|0x20000u);
    }
}

bool CellClass::SetupLAT() {
    using Tile=IsometricTileTypeClass;
    const int old=IsoTileTypeIndex;
    const auto range=[](int value,int start,int count){return value>=start && value<=(start==-1?-1:start+count-1);};
    const auto fix=[&](int base,int start,int family){
        if(family && start==-1)return;
        if(IsoTileTypeIndex!=base && !range(IsoTileTypeIndex,start,16))return;
        int mask=0;
        for(int i=0;i<4;++i){const int other=GetNeighbourCell(static_cast<FacingType>(i*2))->IsoTileTypeIndex;
            bool match=other==base || range(other,start,16);
            if(family==2)match=match || range(other,Tile::ShorePieces,42) || range(other,Tile::WaterBridge,2);
            if(family==3)match=match || range(other,Tile::MiscPaveTile,14) || range(other,Tile::Medians,14) || range(other,Tile::PavedRoads,21);
            if(!match)mask|=1<<i;
        }
        IsoTileTypeIndex=mask?start+mask:base;
    };
    fix(Tile::RoughTile,Tile::ClearToRoughLat,0);
    fix(Tile::SandTile,Tile::ClearToSandLat,1);
    fix(Tile::GreenTile,Tile::ClearToGreenLat,2);
    fix(Tile::PaveTile,Tile::ClearToPaveLat,3);
    if(range(IsoTileTypeIndex,Tile::RampBase,20) || range(IsoTileTypeIndex,Tile::RampSmooth,12)){
        int mask=0;
        constexpr int directions[4][2]{{6,2},{0,4},{2,6},{4,0}};
        if(SlopeIndex>=1 && SlopeIndex<=4){
            for(int i=0;i<2;++i)if(!GetNeighbourCell(static_cast<FacingType>(directions[SlopeIndex-1][i]))->SlopeIndex)mask|=1<<i;
            if(mask)IsoTileTypeIndex=Tile::RampSmooth+3*(SlopeIndex-1)+mask-1;
        }
        if(!mask)IsoTileTypeIndex=Tile::RampBase+SlopeIndex-1;
    }
    if(old!=IsoTileTypeIndex){auto* tile=Tile::Array[IsoTileTypeIndex];if(!tile->GetImage() && !tile->Image && tile->unk_2F4)tile->LoadTMP();}
    return old!=IsoTileTypeIndex;
}

void CellClass::RecalcAttributes(int cellLevel) {
    if(this==&MapClass::InvalidCell)return;
    using Tile=IsometricTileTypeClass;
    auto& map=MapClass::Instance;
    auto& zone=map.LevelAndPassability[map.GetCellZoneIndex(MapCoords)];
    auto& subzone=map.LevelAndPassabilityStruct2pointer_70[map.GetCellZoneIndex(MapCoords)];
    const auto finish=[&]{RecalcPassability();zone.CellLevel=Level;zone.CellPassability=static_cast<char>(Passability);subzone.CellLevel=Level;};
    const auto cliff=[&](bool change){
        const auto setting=RulesClass::Instance->CliffBackImpassability;
        if(!setting)return;
        constexpr CellStruct neighbours[]{{0,-1},{-1,0},{2,2},{1,1},{-1,1},{1,-1}};
        for(auto offset:neighbours){const auto at=CellStruct{short(MapCoords.X+offset.X),short(MapCoords.Y+offset.Y)};
            if(static_cast<signed char>(Level)+4<=static_cast<signed char>(map.GetCellAt(at)->Level)){
                if(setting==2 && change)LandType=::LandType::Rock;break;
            }
        }
    };
    const int overlay=OverlayTypeIndex;
    if(overlay!=-1){auto* type=OverlayTypeClass::Array[overlay];LandType=type->LandType;
        if(LandType==::LandType::Wall || LandType==::LandType::Railroad || type->NoUseTileLandType){
            if(IsoTileTypeIndex!=0xFFFF && IsoTileTypeIndex<Tile::Array.Count)SlopeIndex=static_cast<BYTE>(Tile::Array[IsoTileTypeIndex]->GetSlopeIndex(static_cast<BYTE>(Height)));
            if(SlopeIndex && type->Tiberium){OverlayTypeIndex=-1;OverlayData=0;}
            cliff(true);SetupLAT();finish();return;
        }
    }
    if(IsoTileTypeIndex>=Tile::Array.Count)IsoTileTypeIndex=0xFFFF;
    if(IsoTileTypeIndex!=0xFFFF){
        auto* type=Tile::Array[IsoTileTypeIndex];
        bool valid=type->IsTileIndexValid(static_cast<BYTE>(Height),false);
        if(SlopeIndex && !valid)valid=type->IsTileIndexValid(static_cast<BYTE>(Height),true);
        if(!valid){IsoTileTypeIndex=0xFFFF;Height=0;LandType=::LandType::Clear;SlopeIndex=0;cliff(LandType==::LandType::Clear);finish();return;}
        SlopeIndex=static_cast<BYTE>(type->GetSlopeIndex(static_cast<BYTE>(Height)));
        SetupLAT();
        if(OverlayTypeIndex!=-1){
            if(GetContainedTiberiumIndex()!=-1){
                if(SlopeIndex>4){LandType=type->GetLandType(static_cast<BYTE>(Height));OverlayTypeIndex=-1;OverlayData=0;}
                else if(OverlayTypeClass::Array[OverlayTypeIndex]->LandType==::LandType::Clear)LandType=::LandType::Tiberium;
            }else if(!OverlayTypeClass::Array[OverlayTypeIndex]->NoUseTileLandType)LandType=type->GetLandType(static_cast<BYTE>(Height));
        }else LandType=type->GetLandType(static_cast<BYTE>(Height));
        if(LandType==::LandType::Tunnel && (TubeIndex<0 || TubeIndex>=TubeClass::Array.Count)){
            const int starts[]{Tile::Tunnels,Tile::TrackTunnels,Tile::DirtTunnels,Tile::DirtTrackTunnels};
            constexpr int facings[]{2,4,6,0};
            for(int start:starts)if(IsoTileTypeIndex>=start && IsoTileTypeIndex<=start+3){GameCreate<TubeClass>(&MapCoords,facings[IsoTileTypeIndex-start]);break;}
        }
        if(cellLevel!=-1)Level=static_cast<char>(cellLevel);
        int width=0,height=0;type->GetTileDimensions(static_cast<BYTE>(Height),width,height);unknown_11D=static_cast<BYTE>((height-30)/15);
        if(!(static_cast<unsigned>(Flags)&0x20000u) && type->TileAnimIndex!=-1 && type->TileAttachesTo==static_cast<BYTE>(Height)){
            CoordStruct offset;TacticalClass::Instance->PixelToCoordsAbsolute(&offset,{type->TileXOffset,type->TileYOffset});
            const CoordStruct location{MapCoords.X*256+128+offset.X,MapCoords.Y*256+128+offset.Y,
                static_cast<signed char>(Level)*Unsorted::LevelHeight+offset.Z};
            auto* anim=GameCreate<AnimClass>(AnimTypeClass::Array[type->TileAnimIndex],location,0,-1,0x1600,0,false);
            anim->UseCellLightConvert=true;anim->ZAdjust=type->TileZAdjust;anim->DeleteOnMapCleanup=true;
            Flags=static_cast<CellFlags>(static_cast<unsigned>(Flags)|0x20000u);
        }
        if(type->ShadowCaster){auto* list=type->ShadowCasterList();
            if(list)for(;*list!=CellStruct{0x7FFF,0x7FFF} && *list!=CellStruct::Empty;++list){
                auto* cell=map.GetCellAt(CellStruct{short(MapCoords.X+list->X),short(MapCoords.Y+list->Y)});
                cell->Flags=static_cast<CellFlags>(static_cast<unsigned>(cell->Flags)|0x10000u);
            }
        }
    }else{
        LandType=overlay==-1 || OverlayTypeClass::Array[overlay]->NoUseTileLandType?::LandType::Clear:OverlayTypeClass::Array[overlay]->LandType;
        SlopeIndex=0;
    }
    cliff(LandType==::LandType::Clear || LandType==::LandType::Water || LandType==::LandType::Beach || LandType==::LandType::Ice);
    finish();
}
