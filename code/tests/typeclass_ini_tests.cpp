#include "support/test_support.hpp"
#include "api/filesystem.hpp"
#include "api/type_resources.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <cstring>
namespace {

struct Fixture {
    CCINIClass art;
    TheaterType theater = TheaterType::Temperate;
    CellStruct foundation[2]{{0,0},{0x7fff,0x7fff}};
    game::TypeResourceServices resources;
    Fixture() {
        resources.context = this; resources.art = &art; resources.theater = &theater;
        resources.terrain_foundation = [](void* raw, int index, CellStruct*& output) noexcept {
            if (index != 0) return false;
            output = static_cast<Fixture*>(raw)->foundation; return true;
        };
    }
};
void type_operation(void* raw) {
    auto& fixture = *static_cast<Fixture*>(raw);
    CCINIClass ini;
    SmudgeTypeClass crater("CRATER");
    ini.WriteString("CRATER", "Image", "CRTEST");
    ini.WriteString("CRATER", "Width", "2"); ini.WriteString("CRATER", "Height", "3");
    ini.WriteString("CRATER", "Crater", "yes"); ini.WriteString("CRATER", "Strength", "77");
    fixture.art.WriteString("CRTEST", "Theater", "yes");
    EXPECT_TRUE((crater.LoadFromINI(&ini) && crater.Image)) << "owned theater SHP without software backend";
    EXPECT_TRUE((crater.Width == 2 && crater.Height == 3 && crater.Crater && crater.Strength == 77)) << "common and derived fields";
    EXPECT_TRUE((crater.Image->Type != 0xffff && crater.Image->Width == 2)) << "private raw image, not cached borrowed reference";
    EXPECT_TRUE((crater.LoadFromINI(&ini) && crater.Image)) << "repeat owned read";
    fixture.theater = TheaterType::Snow;
    EXPECT_TRUE((crater.LoadFromINI(&ini) && crater.Image && crater.Image->Width == 3)) << "theater replacement";
    fixture.theater = TheaterType::Temperate;
    OverlayTypeClass overlay("ORE");
    ini.WriteString("ORE", "Image", "ORETEST"); ini.WriteString("ORE", "Tiberium", "yes");
    ini.WriteString("ORE", "CellAnim", "cell_anim"); ini.WriteString("ORE", "Strength", "91");
    ini.WriteString("ORE", "Land", "Clear"); ini.WriteString("ORE", "RadarColor", "10,20,30");
    fixture.art.WriteString("ORETEST", "DamageLevels", "4");
    EXPECT_TRUE((overlay.LoadFromINI(&ini) && overlay.Image)) << "overlay common and SHP loading";
    EXPECT_TRUE((overlay.Image->Type == 0xffff && overlay.DamageLevels == 4 && overlay.CellAnim)) << "shared SHP reference and real AnimType allocation";
    EXPECT_TRUE((static_cast<int>(overlay.LandType) == 5 && static_cast<int>(overlay.Armor) == 6 &&
        overlay.Strength == 91 && overlay.ObjectTypeClass::Strength == 91 && overlay.RadarColor.G == 20)) << "Tiberium overrides and two distinct Strength fields";
    EXPECT_TRUE((overlay.LoadFromINI(&ini) && AnimTypeClass::Array.Count == 1)) << "repeat reference resolution";
    OverlayTypeClass demand("GATEST");
    demand.NewTheater = true; demand.ImageLoaded = true;
    EXPECT_TRUE((demand.GetImage() && demand.ImageAllocated && demand.Image->Type != 0xffff)) << "NewTheater demand loads owned GTTEST.shp";
    EXPECT_TRUE((demand.GetImage() == demand.Image)) << "demand cache reuse";
    TerrainTypeClass tree("TREE");
    ini.WriteString("TREE", "Image", "TREEIMG"); ini.WriteString("TREE", "Strength", "123");
    ini.WriteString("TREE", "AnimationProbability", "0.25");
    ini.WriteString("TREE", "IsAnimated", "yes");
    fixture.art.WriteString("TREEIMG", "Foundation", "1x1");
    EXPECT_TRUE((tree.LoadFromINI(&ini) && tree.Image && tree.FoundationData == fixture.foundation)) << "real Terrain with explicitly provided missing fixed table";
    EXPECT_TRUE((tree.RadarColor.R == 50 && tree.RadarColor.B == 70 && tree.AnimationProbability == 0.25f)) << "radar metadata and animation parameters";
    ini.WriteString("TREE", "RadarColor", "1,2,3");
    EXPECT_TRUE((tree.LoadFromINI(&ini) && tree.RadarColor.B == 3)) << "radar override and owned reload";
    InfantryTypeClass infantry("GI"); UnitTypeClass unit("TANK"); AircraftTypeClass air("JET");
    TaskForceClass task("FORCE");
    ini.WriteString("FORCE", "0", "2,GI"); ini.WriteString("FORCE", "1", "3,missing");
    ini.WriteString("FORCE", "3", "4,TANK"); ini.WriteString("FORCE", "5", "1,JET");
    ini.WriteString("FORCE", "Group", "9");
    EXPECT_TRUE((task.LoadFromINI(&ini) && task.CountEntries == 3 && task.Group == 9)) << "TaskForce compacts resolved types";
    EXPECT_TRUE((task.Entries[0].Type == &infantry && task.Entries[1].Type == &unit && task.Entries[2].Type == &air)) << "TaskForce resolves actual class registries";
    EXPECT_TRUE((task.SaveToINI(&ini))) << "TaskForce export";
    char text[128]{}; ini.ReadString("FORCE", "1", "", text, sizeof(text));
    EXPECT_TRUE((!std::strcmp(text,"4,TANK") && !ini.Exists("FORCE", "5"))) << "compact export removes stale entries";
    ini.WriteString("FORCE", "0", "broken"); EXPECT_TRUE((!task.LoadFromINI(&ini))) << "malformed entry cannot fake success";
    HouseTypeClass country("COUNTRY"); std::strcpy(country.Name, "Country display name");
    EXPECT_TRUE((HouseTypeClass::FindIndexOfName("country DISPLAY NAME") == country.ArrayIndex2)) << "country Name lookup, not only ID";
    RulesClass actual_rules;
    auto* rules = &actual_rules;
    ini.WriteString("Sides", "ALLIED", "COUNTRY");
    EXPECT_TRUE((rules->Read_Sides(&ini) && SideClass::Find("ALLIED") &&
        SideClass::Find("ALLIED")->HouseTypes[0] == country.ArrayIndex)) << "Rules side fill uses real country registry";
    GameDelete(SideClass::Find("ALLIED"));
    GameDelete(AnimTypeClass::Find("cell_anim"));
}
void file_operation(void*) {
    Fixture fixture;
    const auto result = game::with_type_resources(fixture.resources, [](void* p) {
        try { type_operation(p); } catch (const std::exception& e) { std::cerr << "operation: " << e.what() << "\n"; throw; }
    }, &fixture);
    EXPECT_TRUE((result == game::TypeResourceStatus::complete)) << "type/resource operation status";
    // Failure then success must not leak a dangling scope or stale return state.
    SmudgeTypeClass absent("MISSING"); CCINIClass ini;
    ini.WriteString("MISSING", "Name", "missing file is allowed by original loader");
    TerrainTypeClass no_table("NOTABLE");
    EXPECT_TRUE((!no_table.LoadFromINI(&ini))) << "missing foundation dependency does not run original EXE";
    EXPECT_TRUE((absent.LoadFromINI(&ini) && !absent.Image)) << "later independent load has its own success result";
    TheaterType invalid_theater=static_cast<TheaterType>(6);
    game::TypeResourceServices invalid_resources{};invalid_resources.theater=&invalid_theater;
    absent.Theater=true;
    struct InvalidCall { SmudgeTypeClass* type; CCINIClass* ini; } call{&absent,&ini};
    const auto invalid_result=game::with_type_resources(invalid_resources,[](void* opaque){
        auto& c=*static_cast<InvalidCall*>(opaque);
        EXPECT_TRUE((!c.type->LoadFromINI(c.ini))) << "out-of-range theater is rejected before original unchecked indexing";
    },&call);
    EXPECT_TRUE((invalid_result==game::TypeResourceStatus::unavailable)) << "invalid theater context status";
}
void write_shape(const std::filesystem::path& path, int width) {
    unsigned char data[36]{};
    data[2] = static_cast<unsigned char>(width); data[4] = 2; data[6] = 1;
    data[12] = 2; data[14] = 2; data[16] = 1;
    data[20] = 50; data[21] = 60; data[22] = 70; data[28] = 32;
    data[32] = 1; data[33] = 2; data[34] = 3; data[35] = 4;
    std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<char*>(data), sizeof(data));
}
}

TEST(TypeclassIni, Contracts) {
    const auto path = std::filesystem::temp_directory_path()/
        ("ra2-type-ini-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    game::ResourceHandle* files = nullptr;
    const auto cleanup = ra2::test::scope_exit([&] {
        FileSystem::ClearNameCache(); Destroy_All_Shapes(); Unload_All_Shapes();
        game::destroy_resources(files);
        std::error_code ec; std::filesystem::remove_all(path,ec);
    });

        std::filesystem::create_directories(path);
        write_shape(path/"CRTEST.TEM",2); write_shape(path/"CRTEST.SNO",3);
        write_shape(path/"ORETEST.shp",2); write_shape(path/"TREEIMG.shp",2); write_shape(path/"GTTEST.shp",2);
        const auto root = path.u8string(); std::string error;
        EXPECT_TRUE((game::create_resources({reinterpret_cast<const char*>(root.data()),root.size()},files,error))) << error.c_str();
        EXPECT_TRUE((game::with_resources(*files,file_operation,nullptr,error))) << error.c_str();
}
