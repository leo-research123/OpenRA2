#include "support/test_support.hpp"
#include "api/filesystem.hpp"
#include "map_runtime.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/Straws.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using Tile = IsometricTileTypeClass;

void parse(CCINIClass& ini, const char* text) {
    BufferStraw input(const_cast<char*>(text),static_cast<int>(std::strlen(text)));
    EXPECT_TRUE((ini.ReadStraw(input)>0)) << "read catalog fixture";
}
struct Cleanup {
    int animations=AnimTypeClass::Array.Count;
    ~Cleanup() {
        Tile::ClearTileSetCatalog();
        while (AnimTypeClass::Array.Count>animations) GameDelete(AnimTypeClass::Array[AnimTypeClass::Array.Count-1]);
    }
};
bool callback_order=true;
int queries=0;
bool fixture_exists(const char* name) noexcept {
    ++queries;
    if (!std::strcmp(name,"A01.SNO"))
        callback_order = callback_order && Tile::Array.Count==1 &&
            Tile::Array[0]->TileAnimIndex>=0 && AnimTypeClass::Find("WATERFX");
    for (auto* present : {"A01.SNO","A01a.MMS","A01b.TEM","A01d.SNO","A02.URB",
            "B01.SNO","B02.MMS","C01.SNO"})
        if (!std::strcmp(name,present)) return true;
    return false;
}
void synthetic() {
    Cleanup cleanup;
    const int abstract_count=AbstractTypeClass::Array.Count;
    const int object_count=ObjectTypeClass::Array.Count;
    const int listener_count=AbstractClass::TypeExpirationListeners.Count;
    CCINIClass ini;
    parse(ini,R"ini([General]
ClearTile=0
WaterSet=1
ShorePieces=1
CliffSet=2
BridgeSet=2
WaterBridge=2
WoodBridgeSet=2
WaterCliffs=2
PaveTile=3
BridgeTopLeft1=17
[TileSet0000]
SetName=Water
FileName=A
TilesInSet=3
LastTilesInSet=2
MarbleMadness=1
Morphable=yes
ShadowCaster=yes
ShadowTiles=1
AllowToPlace=no
AllowBurrowing=no
AllowTiberium=yes
RequiredForRMG=yes
ToSnowTheater=22
ToTemperateTheater=23
[Water]
Tile01Anim=WATERFX
Tile01XOffset=-2
Tile01YOffset=4
Tile01AttachesTo=9
Tile01ZAdjust=-6
[TileSet0001]
FileName=B
TilesInSet=2
LastTilesInSet=1
NonMarbleMadness=0
[TileSet0002]
FileName=C
TilesInSet=1
)ini");
    auto services=game::default_map_runtime(); services.tile_file_exists=fixture_exists;
    EXPECT_TRUE((game::with_map_runtime(services,[](void* data) {
        auto& ini=*static_cast<CCINIClass*>(data);
        EXPECT_TRUE((Tile::LoadTileSetCatalog(ini,TheaterType::Snow))) << "build synthetic catalog";
        EXPECT_TRUE((callback_order)) << "base and animation registration precede file lookup";
        EXPECT_TRUE((Tile::Array.Count==6 && Tile::TileSetCount==4)) << "only bases count; includes set terminal marker";
        EXPECT_TRUE((Tile::TileSetStarts[0]==0 && Tile::TileSetStarts[1]==3 &&
            Tile::TileSetStarts[2]==5 && Tile::TileSetStarts[3]==6)) << "set positions preserve missing tiles";
        EXPECT_TRUE((Tile::ClearTile==0 && Tile::WaterSet==3 && Tile::CliffSet==5 &&
            Tile::PaveTile==6 && Tile::BridgeTopLeft1==17 && Tile::DestroyableCliffs==-2)) << "General set references versus direct fields";
        EXPECT_TRUE((Tile::TileInsertions.Count==2 && Tile::ConvertTileIndex(1)==1 &&
            Tile::ConvertTileIndex(2)==3 && Tile::ConvertTileIndex(3)==5 &&
            Tile::ConvertTileIndex(0xffff)==0xffff)) << "old tile indices cross both insertion boundaries";
        const auto* base=Tile::Array[0]; const auto* a=base->NextVariant; const auto* b=a ? a->NextVariant : nullptr;
        EXPECT_TRUE((a && b && !b->NextVariant && base->unk_2F0==3 && a->unk_2F0==2 && b->unk_2F0==1)) << "contiguous variants stop at missing c and ignore present d";
        EXPECT_TRUE((!std::strcmp(base->FileName,"A01.SNO") && !std::strcmp(a->FileName,"A01a.MMS") &&
            !std::strcmp(b->FileName,"A01b.TEM") && !std::strcmp(Tile::Array[1]->FileName,"A02.URB"))) << "original fallback order";
        EXPECT_TRUE((!Tile::Array[2]->FileName[0] && !Tile::Array[4]->FileName[0])) << "missing base remains; NonMarbleMadness zero disables fallback";
        EXPECT_TRUE((base->MarbleMadnessTile==3 && Tile::Array[1]->MarbleMadnessTile==4 && a->MarbleMadnessTile==1)) << "marble translation affects base catalog only";
        EXPECT_TRUE((base->Morphable && base->ShadowCaster && a->ShadowCaster && Tile::Array[2]->ShadowCaster &&
            !base->AllowToPlace && !base->AllowBurrowing && base->AllowTiberium && base->RequiredByRMG)) << "set flags and shadow span";
        EXPECT_TRUE((base->TileAnimIndex==AnimTypeClass::FindIndex("WATERFX") && base->TileXOffset==-2 &&
            base->TileYOffset==4 && base->TileAttachesTo==9 && base->TileZAdjust==-6 &&
            a->TileAnimIndex==-1 && a->ToSnowTheater==-1 && base->ToSnowTheater==22)) << "animation allocation and base-only metadata";
        EXPECT_TRUE((Tile::LoadTileSetCatalog(ini,TheaterType::Lunar))) << "switch catalog theater";
        EXPECT_TRUE((Tile::WaterSet==-1 && Tile::ShorePieces==-1 && Tile::CliffSet==-1 && Tile::WaterBridge==-1 &&
            Tile::BridgeSet==-1 && Tile::WoodBridgeSet==-1 && Tile::WaterCliffs==-1)) << "lunar suppresses original seven terrain fields";
        ini.WriteInteger("TileSet0002","MarbleMadness",200);
        EXPECT_TRUE((!Tile::LoadTileSetCatalog(ini,TheaterType::Snow) && !Tile::Array.Count && !Tile::TileInsertions.Count &&
            !Tile::TileSetCount && Tile::ClearTile==-1)) << "invalid catalog rolls back every tile and insertion";
    },&ini))) << "catalog runtime scope";
    const int added=AnimTypeClass::Array.Count-cleanup.animations;
    EXPECT_TRUE((added==1 && AbstractTypeClass::Array.Count==abstract_count+added &&
        ObjectTypeClass::Array.Count==object_count+added && AbstractClass::TypeExpirationListeners.Count==listener_count+added)) << "tile cleanup preserves the separately owned original animation registry";
    std::cout << "Catalog numbering, fallback, variants, animation, lunar and rollback passed (" << queries << " file queries)\n";
}
void real_files(const char* directory) {
    game::ResourceHandle* handle=nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(directory,handle,error))) << error.c_str();
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(handle,game::destroy_resources);
    EXPECT_TRUE((game::load_resources(*handle,{},error)==game::ResourceLoadResult::complete)) << error.c_str();
    EXPECT_TRUE((game::with_resources(*handle,[](void*) {
        Cleanup cleanup;
        for (int n=0;n<6;++n) {
            Tile::ClearTileSetCatalog();
            const auto id=static_cast<TheaterType>(n); const auto& theater=Theater::GetTheater(id);
            EXPECT_TRUE((Theater::MountResourceMixes(id))) << "mount original theater packages";
            EXPECT_TRUE((bool(MixFileClass::Generics.THEATER_TEMPERATMD)==(id==TheaterType::Snow))) << "snow-only MD package slot";
            CCINIClass ini; const auto filename=std::string(theater.ControlFileName)+"MD.INI";
            EXPECT_TRUE((ini.LoadFromFile(filename.c_str())>0)) << "read actual theater INI";
            EXPECT_TRUE((Tile::LoadTileSetCatalog(ini,id))) << "load real catalog through production class";
            int variants=0,loaded=0,missing=0;
            for (auto* base : Tile::Array) for (auto* tile=base;tile;tile=tile->NextVariant) {
                variants+=tile!=base;
                if (!tile->FileName[0]) { ++missing; continue; }
                if (!tile->GetImage()) throw std::runtime_error(std::string("TMP load failed: ")+std::string(tile->FileName,strnlen(tile->FileName,14)));
                ++loaded;
            }
            EXPECT_TRUE((loaded>0)) << "real catalog resolves TMPs";
            std::cout << theater.ID << ": " << Tile::TileSetCount-1 << " sets, " << Tile::Array.Count << " bases, "
                << variants << " variants, " << loaded << " loaded, " << missing << " absent bases, "
                << Tile::TileInsertions.Count << " insertions\n";
        }
        Tile::ClearTileSetCatalog();
        EXPECT_TRUE((Theater::UnmountResourceMixes())) << "close original theater packages";
        EXPECT_TRUE((!MixFileClass::Generics.THEATER_TEMPERAT && !MixFileClass::Generics.THEATER_TEMPERATMD &&
            !MixFileClass::Generics.THEATER_TEM && !MixFileClass::Generics.THEATER_ISOTEM && !MixFileClass::Generics.THEATER_ISOTEMP)) << "ownership ledger clears all original slots on close";
    },nullptr,error))) << error.c_str();
}
}

TEST(TheaterLoading, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();

         synthetic(); if (argc>1) real_files(argv[1]);
}
