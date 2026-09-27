#include "map_world_fixture.hpp"
// Explicit private test access: world identity and production object picking.
#include "map_world_internal.hpp"
#include "yrpp/DisplayClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/MouseClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/Surface.h"
#include <cstdlib>
#include <sstream>

namespace {
struct World {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("ra2-building-placement-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    game::ResourceHandle* resources = nullptr;
    game::MapViewHandle* view = nullptr;
    explicit World(const char* extra = "", bool commands = false) {
        std::filesystem::create_directories(root);
        map_fixture::fixtures(root);
        std::ofstream(root / "RULESMD.INI", std::ios::app) << extra;
        if(commands) {
            map_fixture::shp(root/"BLDGMK.SHP",8);
            std::ifstream input(root/"ARTMD.INI");std::string art((std::istreambuf_iterator<char>(input)),{});input.close();
            art.insert(art.find("[BLDG]\n")+7,"Buildup=BLDGMK\n");std::ofstream(root/"ARTMD.INI")<<art;
        }
        std::ofstream(root / "empty.map") <<
            "[Map]\nSize=0,0,8,12\nLocalSize=0,0,8,12\nLevel=0\nTheater=TEMPERATE\n"
            "[Basic]\nNewINIFormat=4\nPlayer=Neutral\n";
        std::string error;
        if (!game::create_resources(root.string(), resources, error) || !game::create_map_view(*resources, view)
            || !game::load_map_view(*view, "empty.map", 9)) throw std::runtime_error("placement fixture failed: " + error);
    }
    ~World() {
        game::destroy_map_view(view);
        FileSystem::ClearNameCache(); Destroy_All_Shapes(); Unload_All_Shapes();
        game::destroy_resources(resources);
        std::error_code error; std::filesystem::remove_all(root, error);
    }
};
// Controlled RTTI/content records for the cell instruction corpus. The real
// map/type/house logic remains active; no production placement is simulated.
struct Occupant : BuildingClass {
    AbstractType kind = AbstractType::Building;
    using BuildingClass::BuildingClass;
    AbstractType WhatAmI() const override { return kind; }
};
struct ProbeCell : CellClass { ProbeCell() : CellClass() {} };
}

TEST(BuildingPlacement, ReadsConstructionRulesAndKeepsDefaults) {
    World world("\n[BLDG]\nBuildCat=Combat\nAdjacent=6\nBaseNormal=no\nEligibileForAllyBuilding=yes\n"
                "ConstructionYard=yes\nFactory=BuildingType\nLaserFencePost=yes\n");
    const bool completed = game::with_map_view(*world.view, [](void*) {
        auto* type = BuildingTypeClass::Find("BLDG"); ASSERT_NE(type, nullptr);
        EXPECT_EQ(type->BuildCat, BuildCat::Combat); EXPECT_EQ(type->Adjacent, 6);
        EXPECT_FALSE(type->BaseNormal); EXPECT_TRUE(type->EligibileForAllyBuilding);
        EXPECT_TRUE(type->ConstructionYard); EXPECT_TRUE(type->LaserFencePost);
        EXPECT_EQ(type->Factory, AbstractType::BuildingType);
        EXPECT_EQ(SidebarClass::GetObjectTabIdx(AbstractType::BuildingType, type->ArrayIndex, 0), 1);
        auto* ordinary = BuildingTypeClass::Find("CATIME"); ASSERT_NE(ordinary, nullptr);
        EXPECT_EQ(ordinary->BuildCat, BuildCat::DontCare); EXPECT_EQ(ordinary->Adjacent, 3);
        EXPECT_TRUE(ordinary->BaseNormal); EXPECT_FALSE(ordinary->EligibileForAllyBuilding);
        EXPECT_FALSE(ordinary->ConstructionYard);
        EXPECT_EQ(SidebarClass::GetObjectTabIdx(AbstractType::BuildingType, ordinary->ArrayIndex, 0), 0);
    }, nullptr);
    ASSERT_TRUE(completed) << game::map_view_error(*world.view);
}

TEST(BuildingPlacement, OriginalGameRulesSeparateStructuresAndDefenses) {
    const char* data = std::getenv("RA2_GAME_DATA");
    if (!data) GTEST_SKIP() << "Set RA2_GAME_DATA for original rules and map assets";
    game::ResourceHandle* resources = nullptr;
    game::MapViewHandle* view = nullptr;
    auto cleanup = ra2::test::scope_exit([&] {
        game::destroy_map_view(view);
        FileSystem::ClearNameCache(); Destroy_All_Shapes(); Unload_All_Shapes();
        game::destroy_resources(resources);
    });
    std::string error;
    ASSERT_TRUE(game::create_resources(data, resources, error)) << error;
    ASSERT_EQ(game::load_resources(*resources, {}, error), game::ResourceLoadResult::complete) << error;
    ASSERT_TRUE(game::create_map_view(*resources, view));
    ASSERT_TRUE(game::load_map_view(*view, "ALL01UMD.MAP", 12)) << game::map_view_error(*view);
    const bool completed = game::with_map_view(*view, [](void*) {
        for (const char* id : {"GAPOWR", "NAPOWR", "YAPOWR"}) {
            SCOPED_TRACE(id);
            auto* type = BuildingTypeClass::Find(id); ASSERT_NE(type, nullptr);
            EXPECT_EQ(type->BuildCat, BuildCat::Power); EXPECT_EQ(type->Adjacent, 2);
            EXPECT_TRUE(type->BaseNormal);
            EXPECT_EQ(SidebarClass::GetObjectTabIdx(AbstractType::BuildingType, type->ArrayIndex, 0), 0);
        }
        for (const char* id : {"GAPILL", "NALASR", "YAGGUN"}) {
            SCOPED_TRACE(id);
            auto* type = BuildingTypeClass::Find(id); ASSERT_NE(type, nullptr);
            EXPECT_EQ(type->BuildCat, BuildCat::Combat); EXPECT_EQ(type->Adjacent, 4);
            EXPECT_FALSE(type->BaseNormal);
            EXPECT_EQ(SidebarClass::GetObjectTabIdx(AbstractType::BuildingType, type->ArrayIndex, 0), 1);
        }
        for (const char* id : {"GACNST", "NACNST", "YACNST"}) {
            SCOPED_TRACE(id);
            auto* type = BuildingTypeClass::Find(id); ASSERT_NE(type, nullptr);
            EXPECT_TRUE(type->ConstructionYard); EXPECT_TRUE(type->EligibileForAllyBuilding);
            EXPECT_EQ(type->Factory, AbstractType::BuildingType);
        }
    }, nullptr);
    ASSERT_TRUE(completed) << game::map_view_error(*view);
}

TEST(BuildingPlacement, OriginalCellTypeProximityAndShroudInstructions) {
    World world;
    const bool completed = game::with_map_view(*world.view, [](void* opaque) {
        auto& world = *static_cast<World*>(opaque);
        auto& map = MapClass::Instance;
        const auto init = Unsorted::ScenarioInit;
        const auto debug = Unsorted::ArmageddonMode;
        const bool active = Game::IsActive;
        const auto ground = GroundType::Array[0];
        const int water = IsometricTileTypeClass::WaterSet;
        const auto mapRect = map.MapRect, visibleRect = map.VisibleRect;
        auto* oldPlayer = HouseClass::CurrentPlayer;
        auto* oldTower = RulesClass::Instance->WallTower;
        const bool allyOption = world.view->session.Config.BuildOffAlly;
        auto globals = ra2::test::scope_exit([&] {
            Unsorted::ScenarioInit = init; Unsorted::ArmageddonMode = debug; Game::IsActive = active;
            GroundType::Array[0] = ground; IsometricTileTypeClass::WaterSet = water;
            HouseClass::CurrentPlayer = oldPlayer; RulesClass::Instance->WallTower = oldTower;
            world.view->session.Config.BuildOffAlly = allyOption;
            map.MapRect = mapRect; map.VisibleRect = visibleRect;
        });
        HouseClass owner(oldPlayer->Type), enemy(oldPlayer->Type);
        // Match the oracle's explicit geometry, before the session loader's
        // normal playable-border inset. The actual map/cell methods still run.
        map.MapRect = {0, 0, 8, 12}; map.VisibleRect = {0, 0, 8, 12};
        HouseClass::CurrentPlayer = &owner;
        BuildingTypeClass type("PLACEMENT_TYPE", {}), anchorType("PLACEMENT_ANCHOR", {});
        Occupant object(&anchorType, &owner);
        auto restoreObject = ra2::test::scope_exit([&] {
            object.kind = AbstractType::Building; object.AbstractFlags = static_cast<AbstractFlags>(3);
            object.Owner = &owner; object.Health = 100;
        });
        std::vector<std::unique_ptr<OverlayTypeClass>> overlays;
        while (OverlayTypeClass::Array.Count < 128) {
            const std::string id = "PLACEMENT_OV" + std::to_string(OverlayTypeClass::Array.Count);
            overlays.push_back(std::make_unique<OverlayTypeClass>(id.c_str()));
        }
        for (int n = 0; n < OverlayTypeClass::Array.Count; ++n)
            OverlayTypeClass::Array[n]->Wall = n == 0 || n == 2 || n == 26;
        auto* tile = IsometricTileTypeClass::Array[0];
        auto* cell = map.GetCellAt(CellStruct{8, 6});
        ASSERT_NE(cell, nullptr);
        IsometricTileTypeClass::WaterSet = 100;
        const int buildings = BuildingClass::Array.Count, factories = FactoryClass::Array.Count;
        const auto funds = owner.Balance;
        const auto beforeFoundation = DisplayClass::Instance.CurrentFoundation_Data;
        const auto beforeProduct = DisplayClass::Instance.CurrentBuilding;
        std::ifstream input(RA2_BUILDING_PLACEMENT_FIXTURE); ASSERT_TRUE(input.good());
        std::string line; unsigned count = 0;
        while (std::getline(input, line)) {
            SCOPED_TRACE(line);
            std::istringstream row(line); char section; row >> section;
            type.PlaceAnywhere = type.Naval = type.LaserFence = type.LaserFencePost = type.Gate = false;
            type.ToTile = nullptr; type.ToOverlay = nullptr; type.Bib = false;
            type.Foundation = static_cast<Foundation>(0); type.FoundationData = game::native_building_foundation(0);
            type.SpeedType = SpeedType::None; type.UndeploysInto = nullptr;
            tile->Morphable = true;
            Unsorted::ScenarioInit = 0; Unsorted::ArmageddonMode = 0; Game::IsActive = true;
            GroundType::Array[0].Buildable = true;
            for (auto& cost : GroundType::Array[0].Cost) cost = 1.0f;
            cell->MapCoords = {8, 6}; cell->Level = 0; cell->SlopeIndex = 0; cell->Flags = CellFlags{};
            cell->OccupationFlags = cell->AltOccupationFlags = 0;
            cell->OverlayTypeIndex = -1; cell->IsoTileTypeIndex = 0; cell->LandType = LandType::Clear;
            cell->FirstObject = cell->AltObject = nullptr;
            object.NextObject = nullptr; object.Owner = &owner; object.kind = AbstractType::Building;
            object.AbstractFlags = static_cast<AbstractFlags>(3); object.Health = 100;
            object.Location = {8 * 256 + 128, 6 * 256 + 128, 0}; object.IsOnMap = false;
            anchorType.LaserFence = false; RulesClass::Instance->WallTower = nullptr;
            int expected = 0; bool result = false;
            if (section == 'C') {
                int kind, objectKind, occupation, speed, flags, overlay, target, wallOwner, data;
                ASSERT_TRUE(bool(row >> kind >> objectKind >> occupation >> speed >> flags >> overlay >> target >> wallOwner >> data >> expected));
                type.LaserFence = kind == 1; type.LaserFencePost = kind == 2; type.Gate = kind == 3;
                type.ToTile = kind == 4 ? tile : nullptr;
                type.Naval = flags & 16; tile->Morphable = flags & 8;
                Unsorted::ScenarioInit = flags & 1; Game::IsActive = flags & 2; Unsorted::ArmageddonMode = bool(flags & 4);
                if (flags & 1024) cell->MapCoords = {0, 0};
                cell->IsoTileTypeIndex = flags & 512 ? 100 : 0;
                cell->OccupationFlags = occupation;
                cell->Flags = static_cast<CellFlags>((flags & 32 ? 0x100 : 0) | (flags & 64 ? 0x400 : 0));
                cell->SlopeIndex = bool(flags & 128);
                GroundType::Array[0].Buildable = !(flags & 256);
                for (auto& cost : GroundType::Array[0].Cost) cost = flags & 256 ? 0.0f : 1.0f;
                cell->OverlayTypeIndex = overlay; cell->OverlayData = data;
                cell->WallOwnerIndex = wallOwner ? enemy.ArrayIndex : owner.ArrayIndex;
                type.ToOverlay = OverlayTypeClass::Array.GetItemOrDefault(target);
                RulesClass::Instance->WallTower = flags & 4096 ? &type : nullptr;
                if (objectKind) {
                    constexpr AbstractType kinds[]{AbstractType::None, AbstractType::Infantry, AbstractType::Unit,
                        AbstractType::Aircraft, AbstractType::Building, AbstractType::Terrain, AbstractType::Building,
                        AbstractType::Building, AbstractType::Building, AbstractType::Building};
                    object.kind = kinds[objectKind];
                    object.AbstractFlags = static_cast<AbstractFlags>(objectKind == 5 ? 2 : 3);
                    object.Owner = objectKind == 7 ? &enemy : &owner;
                    object.Health = objectKind == 8 ? 0 : 100;
                    anchorType.LaserFence = objectKind == 6 || objectKind == 7;
                    (objectKind == 9 ? cell->AltObject : cell->FirstObject) = &object;
                }
                result = cell->CanThisExistHere(static_cast<SpeedType>(speed), &type, &owner);
            } else if (section == 'T') {
                int foundation, tileProduct, anywhere, bib, blocked, x, y;
                ASSERT_TRUE(bool(row >> foundation >> tileProduct >> anywhere >> bib >> blocked >> x >> y >> expected));
                type.Foundation = static_cast<Foundation>(foundation); type.FoundationData = game::native_building_foundation(foundation);
                type.ToTile = tileProduct ? tile : nullptr; type.PlaceAnywhere = anywhere; type.Bib = bib;
                std::vector<CellClass*> occupied;
                int n = 0;
                for (auto* off = type.FoundationData; *off != CellStruct{0x7FFF, 0x7FFF}; ++off, ++n) {
                    auto* at = map.GetCellAt(CellStruct{short(8 + off->X), short(6 + off->Y)});
                    at->Level = 0; at->SlopeIndex = 0; at->Flags = CellFlags{}; at->OverlayTypeIndex = -1;
                    at->IsoTileTypeIndex = 0; at->LandType = LandType::Clear; at->OccupationFlags = 0; at->FirstObject = nullptr;
                    if (blocked == n || blocked == 8) {
                        at->OccupationFlags = 0x20;
                        if (tileProduct) at->FirstObject = &object;
                    }
                    occupied.push_back(at);
                }
                auto cleanup = ra2::test::scope_exit([&] { for (auto* at : occupied) { at->OccupationFlags = 0; at->FirstObject = nullptr; } });
                CellStruct at{short(x), short(y)};
                const TechnoTypeClass* virtualType = &type;
                result = virtualType->CanCreateHere(at, &owner);
                EXPECT_EQ(type.CanPlaceHere(&at, &owner), result);
            } else if (section == 'P') {
                int foundation, adjacent, dx, dy, mode;
                ASSERT_TRUE(bool(row >> foundation >> adjacent >> dx >> dy >> mode >> expected));
                type.Foundation = static_cast<Foundation>(foundation); type.Adjacent = adjacent;
                anchorType.BaseNormal = mode == 0; anchorType.EligibileForAllyBuilding = mode >= 2;
                object.Owner = mode < 2 ? &owner : &enemy;
                owner.Allies.Clear(); enemy.Allies.Clear();
                if (mode == 3) enemy.Allies.Add(&owner);
                if (mode == 4) owner.Allies.Add(&enemy);
                world.view->session.Config.BuildOffAlly = mode >= 3;
                const CellStruct anchor{short(8 + dx), short(6 + dy)};
                // Supply this one exact cell even outside the usable diamond;
                // proximity itself does not apply the placement boundary test.
                ProbeCell anchorCell; anchorCell.MapCoords = anchor; anchorCell.FirstObject = &object;
                const int index = anchor.X + anchor.Y * 512;
                auto* saved = map.Cells.Items[index]; map.Cells.Items[index] = &anchorCell;
                auto cleanup = ra2::test::scope_exit([&] { map.Cells.Items[index] = saved; anchorCell.FirstObject = nullptr; });
                CellStruct at{8, 6};
                result = DisplayClass::Instance.PassesProximityCheck(&type, owner.ArrayIndex, type.FoundationData, &at);
            } else if (section == 'S') {
                int foundation, tileProduct, dx, dy;
                ASSERT_TRUE(bool(row >> foundation >> tileProduct >> dx >> dy >> expected));
                type.Foundation = static_cast<Foundation>(foundation); type.ToTile = tileProduct ? tile : nullptr;
                for (int x = 7; x < 12; ++x) for (int y = 5; y < 10; ++y) {
                    auto* at = map.GetCellAt(CellStruct{short(x), short(y)});
                    at->AltFlags = AltCellFlags::Mapped; at->Level = 0; at->SlopeIndex = 0;
                }
                auto* hidden = map.GetCellAt(CellStruct{short(8 + dx), short(6 + dy)});
                hidden->AltFlags = AltCellFlags{};
                CellStruct at{8, 6};
                result = DisplayClass::Instance.PassesShroudCheck(&type, owner.ArrayIndex, type.FoundationData, &at);
            } else FAIL() << "Unknown fixture record";
            EXPECT_EQ(result, bool(expected));
            cell->FirstObject = cell->AltObject = nullptr;
            EXPECT_EQ(BuildingClass::Array.Count, buildings); EXPECT_EQ(FactoryClass::Array.Count, factories);
            EXPECT_EQ(owner.Balance, funds); EXPECT_EQ(DisplayClass::Instance.CurrentBuilding, beforeProduct);
            EXPECT_EQ(DisplayClass::Instance.CurrentFoundation_Data, beforeFoundation);
            ++count;
        }
        EXPECT_EQ(count, 3714u);
        cell->MapCoords = {8, 6};
    }, &world);
    ASSERT_TRUE(completed) << game::map_view_error(*world.view);
}

TEST(BuildingPlacement, BuildingVirtualDispatchAndPlayerCheckBypasses) {
    World world;
    const bool completed = game::with_map_view(*world.view, [](void*) {
        auto* owner = HouseClass::CurrentPlayer;
        BuildingTypeClass type("PLACEMENT_DISPATCH", {});
        type.FoundationData = game::native_building_foundation(3); type.Foundation = static_cast<Foundation>(3);
        type.SpeedType = SpeedType::None;
        BuildingClass building(&type, owner);
        auto* cell = MapClass::Instance.GetCellAt(CellStruct{8, 6});
        auto* blocked = MapClass::Instance.GetCellAt(CellStruct{9, 7});
        const auto init = Unsorted::ScenarioInit; Unsorted::ScenarioInit = 0;
        const auto buildable = GroundType::Array[0].Buildable; GroundType::Array[0].Buildable = true;
        auto cleanup = ra2::test::scope_exit([&] { Unsorted::ScenarioInit = init; blocked->OccupationFlags = 0; building.IsOnMap = false; GroundType::Array[0].Buildable = buildable; });
        for (int x = 8; x <= 9; ++x) for (int y = 6; y <= 7; ++y) {
            auto* at = MapClass::Instance.GetCellAt(CellStruct{short(x), short(y)});
            at->OverlayTypeIndex = -1; at->OccupationFlags = 0; at->SlopeIndex = 0; at->Flags = CellFlags{};
            at->LandType = LandType::Clear; at->Level = 0;
        }
        blocked->OccupationFlags = 0x20;
        const ObjectClass* actor = &building;
        EXPECT_EQ(actor->IsCellOccupied(cell, FacingType::None, -1, nullptr, false), Move::No);
        type.PlaceAnywhere = true;
        EXPECT_EQ(actor->IsCellOccupied(cell, FacingType::None, -1, nullptr, false), Move::OK);
        type.PlaceAnywhere = false;
        Unsorted::ScenarioInit = 1;
        EXPECT_EQ(actor->IsCellOccupied(cell, FacingType::None, -1, nullptr, false), Move::OK);
        Unsorted::ScenarioInit = 0;
        // An already-down mobile building checks its single cell, not its footprint.
        UnitTypeClass mobile("PLACEMENT_MOBILE"); type.UndeploysInto = &mobile; building.IsOnMap = true;
        EXPECT_EQ(actor->IsCellOccupied(cell, FacingType::None, -1, nullptr, false), Move::OK);
        building.IsOnMap = false;
        CellStruct at{8, 6}; auto& display = DisplayClass::Instance;
        EXPECT_TRUE(display.PassesProximityCheck(&type, owner->ArrayIndex + 1, type.FoundationData, &at));
        EXPECT_TRUE(display.PassesShroudCheck(&type, owner->ArrayIndex + 1, type.FoundationData, &at));
        EXPECT_TRUE(display.PassesProximityCheck(&type, owner->ArrayIndex, nullptr, &at));
        at = {-1, -1};
        EXPECT_TRUE(display.PassesShroudCheck(&type, owner->ArrayIndex, type.FoundationData, &at));
    }, nullptr);
    ASSERT_TRUE(completed) << game::map_view_error(*world.view);
}

TEST(BuildingCommands, RepairChargesPerStepAndStopsOnFullOrNoFunds) {
    World world("\n[BLDG]\nCost=1000\nRepairable=yes\nClickRepairable=yes\nCrewed=no\n",true);
    ASSERT_TRUE(game::with_map_view(*world.view,[](void*) {
        auto* owner=HouseClass::CurrentPlayer;owner->IsHumanPlayer=true;owner->Balance=1000;
        auto* type=BuildingTypeClass::Find("BLDG");ASSERT_NE(type,nullptr);
        RulesClass::Instance->RepairStep=5;RulesClass::Instance->RepairPercent=0.2;
        RulesClass::Instance->RepairRate=0.02;
        auto building=std::make_unique<BuildingClass>(type,owner);
        building->Health=building->EstimatedHealth=88;
        EXPECT_TRUE(building->CanBeRepaired());EXPECT_EQ(type->GetRepairStepCost(),10);
        const TargetClass target(building.get());
        EventClass repair(owner->ArrayIndex,EventType::Repair,target.m_ID,target.m_RTTI);
        repair.Execute();EXPECT_TRUE(building->IsBeingRepaired);
        Unsorted::CurrentFrame=17;building->UpdateRepair();EXPECT_EQ(building->Health,88);EXPECT_EQ(owner->Balance,1000);
        Unsorted::CurrentFrame=18;building->UpdateRepair();EXPECT_EQ(building->Health,93);EXPECT_EQ(owner->Balance,990);
        repair.Execute();EXPECT_FALSE(building->IsBeingRepaired);building->UpdateRepair();EXPECT_EQ(building->Health,93);
        repair.Execute();Unsorted::CurrentFrame=36;building->UpdateRepair();
        Unsorted::CurrentFrame=54;building->UpdateRepair();
        EXPECT_EQ(building->Health,100);EXPECT_EQ(building->EstimatedHealth,100);
        EXPECT_EQ(owner->Balance,970);EXPECT_FALSE(building->IsBeingRepaired);EXPECT_FALSE(building->CanBeRepaired());
        building->Health=building->EstimatedHealth=70;owner->Balance=9;
        repair.Execute();Unsorted::CurrentFrame=72;building->UpdateRepair();
        EXPECT_EQ(building->Health,70);EXPECT_EQ(owner->Balance,9);EXPECT_FALSE(building->IsBeingRepaired);
        type->ClickRepairable=false;EXPECT_FALSE(building->CanBeRepaired());
        type->ClickRepairable=true;type->Repairable=false;EXPECT_FALSE(building->CanBeRepaired());
    },nullptr));
}

TEST(BuildingCommands, SidebarInputRepairCancelSellAndRetireBuilding) {
    World world("\n[BLDG]\nCost=1000\nRepairable=yes\nCrewed=no\nPower=100\n",true);
    ASSERT_TRUE(game::set_game_view_size(*world.view,800,600));
    struct TestState {World& world;BuildingClass* building=nullptr;game::MapObjectId id{};Point2D point{};int money=0;} state{world};
    ASSERT_TRUE(game::with_map_view(*world.view,[](void* raw) {
        auto& t=*static_cast<TestState*>(raw);auto* house=HouseClass::CurrentPlayer;house->IsHumanPlayer=true;house->Balance=2000;
        auto* type=BuildingTypeClass::Find("BLDG");ASSERT_NE(type,nullptr);ASSERT_NE(type->Buildup,nullptr);
        auto* building=new BuildingClass(type,house);t.building=building;
        ++Unsorted::ScenarioInit;const bool placed=building->Unlimbo({8*256+128,8*256+128,0},DirType::North);--Unsorted::ScenarioInit;
        ASSERT_TRUE(placed);building->Health=building->EstimatedHealth=70;building->NoCrew=true;
        building->DiscoveredByCurrentPlayer=true;building->IsOwnedByCurrentPlayer=true;
        auto& sidebar=SidebarClass::Instance;
        // Real controls and dispatch; only button art is supplied by this
        // format-valid synthetic fixture. MapRadar tests cover original art.
        ShapeButtonClass* buttons[]{&SidebarClass::ToggleRepairButton,&SidebarClass::ToggleSellButton};
        for(int i=0;i<2;++i){auto& b=*buttons[i];b.ID=101+i;b.ToggleType=1;b.UseFlash=true;
            b.SetPosition(sidebar.RepairPosition.X+i*sidebar.RepairPitch,sidebar.RepairPosition.Y);
            b.SetShape(type->Buildup,32,24);GScreenClass::Instance.AddButton(&b);}
        sidebar.UpdateCommandButtons();ASSERT_FALSE(buttons[0]->Disabled);ASSERT_TRUE(building->CanBeSold());
        t.id=game::object_id(*t.world.view->world,building);
    },&state));
    ASSERT_TRUE(game::center_map_view(*world.view,0,480));
    ASSERT_TRUE(game::with_map_view(*world.view,[](void* raw) {
        auto& t=*static_cast<TestState*>(raw);
        auto at=t.building->GetCoords();t.world.view->tactical.CoordsToClient(&at,&t.point);
        // Find an actual pickable point through the production hit test.
        bool found=false;
        for(int y=-40;y<=40&&!found;++y)for(int x=-40;x<=40&&!found;++x) {
            Point2D p{t.point.X+x,t.point.Y+y};
            if(game::pick_world_object(*t.world.view->world,p)==t.building){t.point=p;found=true;}
        }
        ASSERT_TRUE(found);
        CellStruct cell;ASSERT_TRUE(t.world.view->tactical.PickTerrainCell(t.point,DSurface::ViewBounds,cell));
        ASSERT_TRUE(t.building->Owner->IsControlledByCurrentPlayer());ASSERT_TRUE(t.building->CanBeRepaired());
    },&state));
    game::GameInputResult result;
    const auto send=[&](game::GameInputKind kind,Point2D p,unsigned code,bool down){
        ASSERT_TRUE(game::submit_game_input(*world.view,{kind,p.X,p.Y,code,0,down,0},result));
    };
    const auto click=[&](Point2D p){send(game::GameInputKind::pointer_button,p,1,true);send(game::GameInputKind::pointer_button,p,1,false);};
    const auto expect_queue=[&](int count,EventType type=EventType::Empty){
        struct Query {int count;EventType type;} q{count,type};
        ASSERT_TRUE(game::with_map_view(*world.view,[](void* p){auto& q=*static_cast<Query*>(p);
            ASSERT_EQ(EventClass::OutList.Count,q.count);
            if(q.count)EXPECT_EQ(EventClass::OutList.First().Type,q.type);
        },&q));
    };
    auto& sidebar=SidebarClass::Instance;const Point2D repair{sidebar.RepairPosition.X+5,sidebar.RepairPosition.Y+5};
    const Point2D sell{repair.X+sidebar.RepairPitch,repair.Y};
    click(repair);EXPECT_TRUE(sidebar.RepairMode);EXPECT_TRUE(SidebarClass::ToggleRepairButton.IsOn);
    click(state.point);expect_queue(1,EventType::Repair);
    ASSERT_TRUE(game::update_game_view(*world.view,1.0/15.0));EXPECT_TRUE(state.building->IsBeingRepaired);
    click(sell);EXPECT_FALSE(sidebar.RepairMode);EXPECT_TRUE(sidebar.SellMode);
    send(game::GameInputKind::pointer_button,state.point,2,true);send(game::GameInputKind::pointer_button,state.point,2,false);
    EXPECT_FALSE(sidebar.SellMode);EXPECT_FALSE(MouseClass::Instance.unknown_byte_554A);
    click(repair);send(game::GameInputKind::key,state.point,27,true);EXPECT_FALSE(sidebar.RepairMode);
    click(sell);
    send(game::GameInputKind::pointer_button,state.point,1,true);
    send(game::GameInputKind::pointer_move,{state.point.X+20,state.point.Y},0,false);
    send(game::GameInputKind::pointer_button,{state.point.X+20,state.point.Y},1,false);
    expect_queue(0); // dragging is not a sell command
    ASSERT_TRUE(game::with_map_view(*world.view,[](void* raw){auto& t=*static_cast<TestState*>(raw);t.money=t.building->Owner->Balance;},&state));
    click(state.point);expect_queue(1,EventType::Sell);
    ASSERT_TRUE(game::update_game_view(*world.view,1.0/15.0));
    ASSERT_TRUE(game::with_map_view(*world.view,[](void* raw){auto& t=*static_cast<TestState*>(raw);
        EXPECT_EQ(t.building->CurrentMission,Mission::Selling);EXPECT_EQ(t.building->Owner->Balance,t.money);
        EXPECT_FALSE(t.building->IsBeingRepaired);},&state));
    for(int i=0;i<240 && game::resolve_map_object(*world.view->world,state.id);++i)
        ASSERT_TRUE(game::update_game_view(*world.view,1.0/15.0));
    EXPECT_EQ(game::resolve_map_object(*world.view->world,state.id),nullptr);
    ASSERT_TRUE(game::with_map_view(*world.view,[](void* raw){auto& t=*static_cast<TestState*>(raw);
        EXPECT_EQ(HouseClass::CurrentPlayer->Balance,t.money+500);
        EXPECT_EQ(HouseClass::CurrentPlayer->OwnedBuildings,0);
        SidebarClass::Instance.UpdateCommandButtons();EXPECT_TRUE(SidebarClass::ToggleSellButton.Disabled);
        EXPECT_FALSE(SidebarClass::Instance.SellMode);
    },&state));
}

TEST(BuildingCommands, RefundExcludesBundledUnitAndHonorsOwnerAndSoylent) {
    World world("\n[BLDG]\nCost=2000\n",true);
    ASSERT_TRUE(game::with_map_view(*world.view,[](void*) {
        auto* owner=HouseClass::CurrentPlayer;owner->IsHumanPlayer=true;
        owner->CostBuildingsMult=owner->Type->CostBuildingsMult=1.0f;
        owner->CostUnitsMult=owner->Type->CostUnitsMult=1.0f;
        RulesClass::Instance->RefundPercent=0.5;
        auto* type=BuildingTypeClass::Find("BLDG");ASSERT_NE(type,nullptr);
        UnitTypeClass miner("REPAIR_SELL_FREE");miner.Cost=1400;
        auto* previous=type->FreeUnit;type->FreeUnit=&miner;
        const auto restore=ra2::test::scope_exit([&]{type->FreeUnit=previous;});
        EXPECT_EQ(type->GetCost(),600);EXPECT_EQ(type->GetActualCost(owner),2000);
        EXPECT_EQ(type->GetRefund(owner,false),300);
        EXPECT_EQ(type->GetRefund(owner,true),600);
        owner->Type->CostBuildingsMult=0.8f;owner->CostBuildingsMult=0.5f;
        EXPECT_EQ(type->GetRefund(owner,false),120);
        type->Soylent=500;EXPECT_EQ(type->GetRefund(owner,false),400);
        type->Soylent=0;owner->IsHumanPlayer=owner->IsInPlayerControl=false;
        EXPECT_EQ(type->GetRefund(owner,false),240);
    },nullptr));
}

TEST(BuildingCommands, ModesAndEligibilityRejectInvalidCommands) {
    World world("\n[BLDG]\nUnsellable=yes\nRepairable=no\n",true);
    ASSERT_TRUE(game::with_map_view(*world.view,[](void*) {
        auto& sidebar=SidebarClass::Instance;auto* owner=HouseClass::CurrentPlayer;
        owner->IsHumanPlayer=true;owner->Balance=2000;
        auto* type=BuildingTypeClass::Find("BLDG");ASSERT_NE(type,nullptr);
        EXPECT_TRUE(type->Unsellable);EXPECT_FALSE(type->Repairable);
        auto building=std::make_unique<BuildingClass>(type,owner);building->Health=50;
        EXPECT_FALSE(building->CanBeSold());EXPECT_FALSE(building->CanBeRepaired());
        type->Unsellable=false;type->Repairable=true;
        EXPECT_TRUE(building->CanBeSold());EXPECT_TRUE(building->CanBeRepaired());
        building->ForceShielded=1;EXPECT_FALSE(building->CanBeSold());building->ForceShielded=0;
        building->ForceMission(Mission::Construction);EXPECT_FALSE(building->CanBeSold());building->ForceMission(Mission::Guard);
        sidebar.CurrentBuildingType=type;sidebar.SetRepairMode(1);EXPECT_FALSE(sidebar.RepairMode);
        sidebar.CurrentBuildingType=nullptr;sidebar.SetRepairMode(1);EXPECT_TRUE(sidebar.RepairMode);
        sidebar.SetSellMode(1);EXPECT_TRUE(sidebar.SellMode);EXPECT_FALSE(sidebar.RepairMode);
        sidebar.SetSellMode(-1);EXPECT_FALSE(sidebar.SellMode);
        const TargetClass target(building.get());
        EventClass foreign(owner->ArrayIndex+1,EventType::Sell,target.m_ID,target.m_RTTI);
        foreign.Execute();EXPECT_EQ(building->CurrentMission,Mission::Guard);EXPECT_EQ(owner->Balance,2000);
        EventClass sell(owner->ArrayIndex,EventType::Sell,target.m_ID,target.m_RTTI);
        sell.Execute();EXPECT_EQ(building->CurrentMission,Mission::Selling);
        const int status=building->MissionStatus;
        sell.Execute();EXPECT_EQ(building->MissionStatus,status)<<"repeated sell never restarts the mission";
        building.reset();sidebar.UpdateCommandButtons();
        EXPECT_TRUE(SidebarClass::ToggleRepairButton.Disabled);EXPECT_TRUE(SidebarClass::ToggleSellButton.Disabled);
        sidebar.SetRepairMode(1);EXPECT_FALSE(sidebar.RepairMode);
    },nullptr));
}

TEST(BuildingCommands, OriginalAssetsRepairAndSellWithCrew) {
    const char* data=std::getenv("RA2_GAME_DATA");
    if(!data || !*data)GTEST_SKIP()<<"RA2_GAME_DATA enables real building/crew lifecycle regression";
    game::ResourceHandle* raw=nullptr;std::string error;
    ASSERT_TRUE(game::create_resources(data,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> resources(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*resources,{},error),game::ResourceLoadResult::complete)<<error;
    game::MapViewHandle* handle=nullptr;ASSERT_TRUE(game::create_map_view(*resources,handle));
    std::unique_ptr<game::MapViewHandle,decltype(&game::destroy_map_view)> view(handle,game::destroy_map_view);
    ASSERT_TRUE(game::load_map_view(*view,"ALL01UMD.MAP",12))<<game::map_view_error(*view);
    ASSERT_TRUE(game::with_map_view(*view,[](void* raw_view) {
        auto& view=*static_cast<game::MapViewHandle*>(raw_view);
        auto* house=HouseClass::CurrentPlayer;ASSERT_NE(house,nullptr);
        BuildingClass* building=nullptr;
        for(auto* candidate:BuildingClass::Array)
            if(candidate->Owner==house && candidate->CanBeSold() && candidate->Type->Crewed
                && candidate->GetCrew() && candidate->GetCrewCount()>0 && !candidate->Type->UndeploysInto) {
                building=candidate;break;
            }
        ASSERT_NE(building,nullptr)<<"real map must provide an owned sellable building with crew";
        house->Balance=10000;building->Health=building->EstimatedHealth=building->Type->Strength-10;
        const auto identity=game::object_id(*view.world,building);const TargetClass target(building);
        EventClass repair(house->ArrayIndex,EventType::Repair,target.m_ID,target.m_RTTI);
        repair.Execute();ASSERT_TRUE(building->IsBeingRepaired);
        const int health=building->Health,money=house->Balance;
        Unsorted::CurrentFrame=0;building->UpdateRepair();
        EXPECT_GT(building->Health,health);EXPECT_LT(house->Balance,money);
        const int before=house->Balance,refund=building->GetRefund(),crew_before=InfantryClass::Array.Count;
        EventClass sell(house->ArrayIndex,EventType::Sell,target.m_ID,target.m_RTTI);
        sell.Execute();ASSERT_EQ(building->CurrentMission,Mission::Selling);
        for(int i=0;i<240 && game::resolve_map_object(*view.world,identity);++i) {
            ++Unsorted::CurrentFrame;building->Update();
        }
        EXPECT_EQ(game::resolve_map_object(*view.world,identity),nullptr);
        EXPECT_EQ(house->Balance,before+refund);EXPECT_GT(InfantryClass::Array.Count,crew_before);
    },view.get()));
}
