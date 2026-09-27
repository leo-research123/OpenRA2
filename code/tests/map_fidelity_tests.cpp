#include "support/test_support.hpp"
// Original-derived contracts and bounded native-session invariants.
// Provenance and limits: tests/fixtures/map_fidelity_reference.json.
// All synthetic fixtures exercise the real core; no Godot-side object model.
#include "support/map_test_support.hpp"
#include "map_world_internal.hpp"
#include "type_resources.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Pipes.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <functional>
#include <sstream>
#include <exception>
#include <algorithm>
using namespace map_fixture;

namespace {
void ticks(game::MapViewHandle& view, int count) {
    // This helper counts logic iterations, not host refreshes. Timing itself
    // is compared to the EXE by original_time.
    const int speed=GameOptionsClass::Instance.GameSpeed;
    GameOptionsClass::Instance.GameSpeed=0;view.loop.frame_timer.Stop();
    for(int i=0;i<count;++i)EXPECT_TRUE((game::update_game_view(view,0.016))) << "advance simulation tick";
    GameOptionsClass::Instance.GameSpeed=speed;
}
struct Draw {
    std::vector<game::ShapeDrawingRequest> shapes;
    std::vector<game::RasterDrawingRequest> rasters;
};
Draw draw(game::MapViewHandle& view) {
    EXPECT_TRUE((game::set_map_viewport(view, 640, 480))) << "set viewport";
    Draw out; game::MapDrawingContext context; context.types.backend_context = &out;
    context.types.target = reinterpret_cast<game::DrawingTargetHandle*>(&out);
    context.terrain_palette = [](void*, const BytePalette& p, int,int,int,int,
            const game::DrawingPaletteHandle*& result) noexcept {
        result = reinterpret_cast<const game::DrawingPaletteHandle*>(&p); return game::DrawingStatus::drawn;
    };
    context.shape_palette = [](void*, const BytePalette& p, int, const game::DrawingPaletteHandle*& result) noexcept {
        result = reinterpret_cast<const game::DrawingPaletteHandle*>(&p); return game::DrawingStatus::drawn;
    };
    context.color_scheme_palette=context.shape_palette;
    context.types.backend.tile = [](void*, const game::TileDrawingRequest&) { return game::DrawingStatus::drawn; };
    context.types.backend.shape = [](void* p, const game::ShapeDrawingRequest& r) {
        static_cast<Draw*>(p)->shapes.push_back(r); return game::DrawingStatus::drawn;
    };
    context.types.backend.raster = [](void* p, const game::RasterDrawingRequest& r) {
        // Only metadata is inspected after the callback. Pixel storage is borrowed.
        auto copy = r; copy.pixels = nullptr;
        static_cast<Draw*>(p)->rasters.push_back(copy); return game::DrawingStatus::drawn;
    };
    game::MapDrawStatistics statistics;
    EXPECT_TRUE((game::draw_map_view(view, context, statistics) == game::DrawingStatus::drawn)) << "draw native map";
    return out;
}
void resource_fixture(const std::filesystem::path& root, int growth, int spread=0) {
    edit(root/"world.map", "Size=0,0,8,12\nLocalSize=0,0,8,12", "Size=0,0,32,32\nLocalSize=0,0,32,32");
    edit(root/"RULESMD.INI", "Growth=1\n", "Growth=" + std::to_string(growth) + "\n");
    edit(root/"RULESMD.INI", "Spread=0\n", "Spread=" + std::to_string(spread) + "\nSpreadPercentage=1\n");
    edit(root/"RULESMD.INI", "SpawnsTiberium=yes", "SpawnsTiberium=no");
}
std::vector<CellClass*> seed(game::MapViewHandle& view, int count) {
    std::vector<CellClass*> cells;
    native(view, [&] {
        for (int n=0; n<TerrainClass::Array.Count; ++n) TerrainClass::Array[n]->Type->IsAnimated=false;
        auto* type=TiberiumClass::Array[0];
        for (int n=0; n<MapClass::Instance.Cells.Capacity && int(cells.size())<count; ++n) {
            auto* cell=MapClass::Instance.Cells[n];
            if (cell && cell->CanTiberiumGerminate(type) && cell->IncreaseTiberium(0,1)) cells.push_back(cell);
        }
        EXPECT_TRUE((int(cells.size())==count)) << "seed stable resource cohort";
    });
    return cells;
}
void no_starvation(int count, int period) {
    session([=](const auto& root) { resource_fixture(root,period); }, [=](auto& view) {
        auto cells=seed(view,count); ticks(view,100);
        for (auto* cell:cells) EXPECT_TRUE((cell->OverlayData>1)) << "all resource cohorts eventually receive growth";
    });
}
void wall_fixture(const std::filesystem::path& root,bool generic=false) {
 edit(root/"RULESMD.INI","[ORE102]","114=GAWALL\n[GAWALL]\nWall=yes\nImage=GAWALL\n[ORE102]");
 std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[GAWALL]\nNewTheater=yes\n";
 // Sixteen connection frames plus their sixteen shadow frames.
 shp(root/(generic?"GGWALL.SHP":"GTWALL.SHP"),32);
 // Original Theater table: Snow.Letter is 'A' (0x007E1C3E), not 'S'.
 // 0x005F96B0 therefore resolves the snow wall to GAWALL.SHP.
 shp(root/"GAWALL.SHP",64);
}

 
TEST(MapFidelity, N27_wall_theater_image_and_draw) {
    SCOPED_TRACE("0x005FE620 / 0x0047F6A0");
    session([](const auto&r){wall_fixture(r);},[](auto&v){
      // Original wall drawing takes the current player's converter. The
      // minimal synthetic map does not select a player during loading.
      struct PlayerScope {HouseClass* previous;~PlayerScope(){HouseClass::CurrentPlayer=previous;}} player_scope{HouseClass::CurrentPlayer};
      native(v,[&]{HouseClass::CurrentPlayer=placed().Owner;ASSERT_NE(HouseClass::CurrentPlayer,nullptr);});
      SHPStruct*image=nullptr;native(v,[&]{auto*t=OverlayTypeClass::Find("GAWALL");EXPECT_TRUE((t&&t->GetImage())) << "scene-specific wall is loaded";image=t->GetImage();EXPECT_TRUE((image->Frames==32&&!t->ImageAllocated&&!t->ImageLoaded)) << "eager image stays cache-owned";
       auto*c=MapClass::Instance.TryGetCellAt(CellStruct{8,4});EXPECT_TRUE((c!=nullptr)) << "wall cell";c->OverlayTypeIndex=t->ArrayIndex;c->OverlayData=12;game::changed_world_cell(*v.world,*c);game::map_object_changed();});
      const auto rendered=draw(v);int found=0;for(const auto&r:rendered.shapes)if(r.image==image){const bool shadow=found++==1;EXPECT_TRUE((r.frame==(shadow?28:12)&&r.flags==(shadow?0x4601u:0x4E00u)&&r.gradient==(shadow?0:2)&&r.depth_mode==game::ShapeDepthMode::legacy)) << "original wall body and matching shadow";}
      EXPECT_EQ(found,2) << "wall body and shadow reach native drawing callback";
      native(v,[&]{auto*ref=image->AsReference();EXPECT_TRUE((ref&&ref->Loaded&&ref->Data)) << "world drawing retains original SHP reference data";
       ref->Unload();game::map_object_changed();});
      const auto reloaded=draw(v);EXPECT_TRUE((!reloaded.shapes.empty())) << "world redraw after explicit SHP unload";
      native(v,[&]{EXPECT_TRUE((image->AsReference()->Loaded)) << "world drawing reloads explicitly unloaded SHP";});});
}

 
TEST(MapFidelity, N28_wall_generic_and_theater_reload) {
    SCOPED_TRACE("0x005FE620 generic fallback and scene switch");
    session([](const auto&r){wall_fixture(r,true);},[](auto&v){native(v,[]{auto*t=OverlayTypeClass::Find("GAWALL");EXPECT_TRUE((t&&t->GetImage()&&t->GetImage()->Frames==32)) << "generic fallback loaded";auto*generic=t->Image;
      OverlayTypeClass::LoadFromIniList(1);EXPECT_TRUE((t->Image&&t->Image->Frames==64&&t->Image!=generic)) << "snow image replaces cached scene image";
      OverlayTypeClass::LoadFromIniList(0);EXPECT_TRUE((t->Image==generic)) << "switch back reuses owned cache";});});
}

 
TEST(MapFidelity, N29_overlay_owned_demand_invalidation) {
    SCOPED_TRACE("0x005FE620 / 0x005FEDE0 ownership");
    session([](const auto&r){wall_fixture(r);},[](auto&v){native(v,[]{auto*t=OverlayTypeClass::Find("GAWALL");auto*cached=t->Image;t->ImageLoaded=true;
      OverlayTypeClass::LoadFromIniList(0);EXPECT_TRUE((t->Image==cached)) << "unowned cache must not be freed";t->Image=nullptr;EXPECT_TRUE((t->GetImage()!=nullptr&&t->ImageAllocated)) << "demand load owns raw storage";
      OverlayTypeClass::LoadFromIniList(0);EXPECT_TRUE((!t->Image&&!t->ImageAllocated&&t->ImageLoaded)) << "scene init frees owned demand image and retains mode";EXPECT_TRUE((t->GetImage()!=nullptr)) << "demand image reloads after invalidation";});});
}


 
TEST(MapFidelity, F01_art_image_alias) {
    SCOPED_TRACE("0x0045F230");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nImage=BODYALIAS\n");
            std::filesystem::remove(root/"BLDG.SHP"); shp(root/"BODYALIAS.SHP",2);
        }, [](auto& view) { native(view, [] {
            EXPECT_TRUE((placed().Type->Image && placed().Type->Image->GetData())) << "ART.Image resolves existing body alias";
        }); });
}

 
TEST(MapFidelity, F02_art_remapable) {
    SCOPED_TRACE("0x00712170");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nRemapable=yes\n");
            edit(root/"RULESMD.INI", "[BLDG]\n", "[BLDG]\nRemapable=no\n");
        }, [](auto& view) { native(view, [] { EXPECT_TRUE((placed().Type->Remapable)) << "ART wins, rules.Remapable is not its source"; }); });
}

 
TEST(MapFidelity, F03_rules_turret) {
    SCOPED_TRACE("0x0045FE50");

        session([](const auto& root) {
            turret_fixture(root);
            edit(root/"RULESMD.INI", "[BLDG]\n", "[BLDG]\nTurretAnimX=17\nTurretAnimY=-9\nTurretAnimZAdjust=-42\nTurretAnimYSort=19\n");
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nTurretAnim=WRONG\nTurretAnimX=999\n");
        }, [](auto& view) { native(view, [] {
            const auto& slot=placed().Type->BuildingAnim[9];
            EXPECT_TRUE((std::string(slot.Anim)=="TURRET" && slot.Position==Point2D{17,-9} && slot.ZAdjust==-42 && slot.YSort==19)) << "turret names and offsets come from rules, not ART";
        }); });
}

 
TEST(MapFidelity, F04_factory_shapes) {
    SCOPED_TRACE("0x0045F230");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nDeployingAnim=SPIN\nRoofDeployingAnim=SPIN\nDoorAnim=SPIN\nUnderDoorAnim=SPIN\nUnderRoofDoorAnim=SPIN\nSpecialZOverlay=SPIN\nRubble=SPIN\n");
        }, [](auto& view) { native(view, [] {
            auto& t=*placed().Type;
            for (auto* image:{t.DeployingAnim,t.RoofDeployingAnim,t.DoorAnim,t.UnderDoorAnim,t.UnderRoofDoorAnim,t.SpecialZOverlay,t.Rubble})
                EXPECT_TRUE((image && image->GetData())) << "all seven factory/auxiliary assets resolve";
            EXPECT_TRUE((t.DeployingAnimLoaded&&t.RoofDeployingAnimLoaded&&t.UnderDoorAnimLoaded&&t.UnderRoofDoorAnimLoaded&&t.RubbleLoaded)) << "owned auxiliary allocations carry destructor ownership flags";
        }); });
}

 
TEST(MapFidelity, F05_powerup_location_keys) {
    SCOPED_TRACE("0x0045FE50");

        session([](const auto& root) {
            // Corrected audit precondition: original reads slots only for Upgrades>0.
            edit(root/"RULESMD.INI", "[BLDG]\n", "[BLDG]\nUpgrades=1\n");
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nPowerUp1Anim=SPIN\nPowerUp1LocXX=13\nPowerUp1LocYY=-7\nPowerUp1LocZZ=-42\nPowerUp1YSort=19\nPowerUp1AnimX=999\n");
        }, [](auto& view) { native(view, [] {
            const auto& slot=placed().Type->BuildingAnim[0];
            EXPECT_TRUE((slot.Position==Point2D{13,-7}&&slot.ZAdjust==-42&&slot.YSort==19)) << "original PowerUp LocXX/LocYY/LocZZ/YSort keys";
        }); });
}

 
TEST(MapFidelity, F06_damage_before_garrison) {
    SCOPED_TRACE("0x00451750");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "ActiveAnimDamaged=SPINDAM\n", "ActiveAnimDamaged=SPINDAM\nActiveAnimGarrisoned=SPIN\n");
        }, [](auto& view) { native(view, [] {
            auto& b=placed(); b.PlayNthAnim(static_cast<BuildingAnimSlot>(3),true,true,0);
            EXPECT_TRUE((b.Anims[3] && std::string(b.Anims[3]->Type->ID)=="SPINDAM")) << "damage takes priority over garrison";
        }); });
}

 
TEST(MapFidelity, F07_resource_cohort_128_2) {
    SCOPED_TRACE("native liveness invariant (not EXE timing parity)");
     no_starvation(128,2);
}

 

 
TEST(MapFidelity, N01_alias_preserves_art_section) {
    SCOPED_TRACE("0x0045F230");

        session([](const auto& root) {
            edit(root/"RULESMD.INI", "[BLDG]\n", "[BLDG]\nImage=ARTBASE\n");
            edit(root/"ARTMD.INI", "[BLDG]\n", "[ARTBASE]\nImage=BODYALIAS\n");
            std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[BODYALIAS]\nFoundation=1x1\nActiveAnim=WRONG\n";
            shp(root/"BODYALIAS.SHP",2);
        }, [](auto& view) { native(view, [] {
            auto& t=*placed().Type;
            EXPECT_TRUE((std::string(t.ImageFile)=="ARTBASE"&&t.GetFoundationWidth()==2&&std::string(t.BuildingAnim[3].Anim)=="SPIN")) << "body alias must not become the ART section name";
            EXPECT_TRUE((t.Image&&t.Image->GetData())) << "body alias loads when rules.Image differs from type ID";
        }); });
}

 
TEST(MapFidelity, N02_building_theater_without_newtheater) {
    SCOPED_TRACE("0x005F96B0 / 0x0045F230");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nImage=GABODY\n");
            shp(root/"GTBODY.SHP",2); std::filesystem::remove(root/"BLDG.SHP");
        }, [](auto& view) { native(view, [] {
            auto& t=*placed().Type;
            EXPECT_TRUE((t.Image&&t.Image->GetData())) << "building-specific theater applies even with NewTheater=no";
            EXPECT_TRUE((std::string(t.TheaterSpecificID)=="GTBODY.shp")) << "resolved body filename is recorded";
        }); });
}

 
TEST(MapFidelity, N03_generic_theater_fallback) {
    SCOPED_TRACE("0x005F9710");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nImage=GABODY\n");
            shp(root/"GGBODY.SHP",2); std::filesystem::remove(root/"BLDG.SHP");
        }, [](auto& view) { native(view, [] {
            EXPECT_TRUE((placed().Type->Image&&placed().Type->Image->GetData())) << "generic G candidate resolves when theater-specific file is absent";
            EXPECT_TRUE((std::string(placed().Type->TheaterSpecificID)=="GGBODY.shp")) << "fallback candidate is recorded";
        }); });
}

 
TEST(MapFidelity, N04_auxiliary_theater_and_ownership) {
    SCOPED_TRACE("0x0045F230");

        session([](const auto& root) {
            edit(root/"ARTMD.INI", "[BLDG]\n", "[BLDG]\nBibShape=GABIB\nBuildup=GABUILD\nDeployingAnim=GAROOF\n");
            shp(root/"GTBIB.SHP",2);shp(root/"GGBUILD.SHP",4);shp(root/"GGROOF.SHP",2);
        }, [](auto& view) {
            for(int i=0;i<3;++i) {
                native(view, [] { auto& t=*placed().Type;
                    EXPECT_TRUE((t.BibShape&&t.BibShapeLoaded&&t.Buildup&&t.BuildupLoaded&&t.DeployingAnim&&t.DeployingAnimLoaded)) << "auxiliary theater/fallback loading preserves ownership";
                });
                EXPECT_TRUE((game::load_map_view(view,"world.map",9))) << "reload cleans owned and cached resources";
            }
        });
}

 
TEST(MapFidelity, N05_rules_remapable_ignored) {
    SCOPED_TRACE("0x00712170");

        session([](const auto& root) { edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nRemapable=yes\n"); },
            [](auto& view) { native(view, [] { EXPECT_TRUE((!placed().Type->Remapable)) << "rules-only Remapable must not enable remapping"; }); });
}

 
TEST(MapFidelity, N06_undeclared_powerup_ignored) {
    SCOPED_TRACE("0x0045FE50");

        session([](const auto& root) { edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nPowerUp1Anim=SPIN\nPowerUp1LocXX=13\n"); },
            [](auto& view) { native(view, [] {
                EXPECT_TRUE((!*placed().Type->BuildingAnim[0].Anim&&placed().Type->BuildingAnim[0].Position.X==0)) << "Upgrades=0 leaves upgrade slot untouched";
            }); });
}

 
TEST(MapFidelity, N07_all_three_powerup_slots) {
    SCOPED_TRACE("0x0045FE50");

        session([](const auto& root) {
            edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nUpgrades=3\n");
            std::string values="[BLDG]\n";
            for(int i=1;i<=3;++i) values+="PowerUp"+std::to_string(i)+"Anim=SPIN\nPowerUp"+std::to_string(i)+"LocXX="+std::to_string(i*7)+"\n";
            edit(root/"ARTMD.INI","[BLDG]\n",values);
        }, [](auto& view) { native(view, [] {
            for(int i=0;i<3;++i) EXPECT_TRUE((placed().Type->BuildingAnim[i].Position.X==(i+1)*7)) << "all upgrade keys use independent indices";
        }); });
}

 
TEST(MapFidelity, N08_turret_garrison_rules_image_section) {
    SCOPED_TRACE("0x0045FE50");

        session([](const auto& root) {
            turret_fixture(root); edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nImage=ARTBASE\nTurretAnimGarrisoned=WRONG\n");
            std::ofstream(root/"RULESMD.INI",std::ios::app)<<"\n[ARTBASE]\nTurretAnimGarrisoned=SPIN\n";
            edit(root/"ARTMD.INI","[BLDG]\n","[ARTBASE]\nImage=BLDG\nTurretAnimGarrisoned=ALSO_WRONG\n");
        }, [](auto& view) { native(view, [] {
            EXPECT_TRUE((std::string(placed().Type->BuildingAnim[9].Garrisoned)=="SPIN")) << "exceptional source is rules[ImageFile]";
        }); });
}

 
TEST(MapFidelity, N09_turret_facings_original_table) {
    SCOPED_TRACE("0x007F4890 / 0x0045125A");

        session(turret_fixture, [](auto& view) { native(view, [] {
            std::ifstream input(RA2_TURRET_FIXTURE); EXPECT_TRUE((bool(input))) << "open binary-derived turret fixture";
            unsigned raw; int expected, count=0;
            while(input>>raw>>expected) {
                placed().PrimaryFacing.SetCurrent(DirStruct(int(raw))); placed().UpdateAnimations();
                EXPECT_TRUE((placed().Anims[9]&&placed().Anims[9]->Animation.Value==expected)) << "original rounded BAM direction lookup";
                EXPECT_TRUE((placed().Anims[9]->Animation.Rate==0)) << "facing-driven timer stays disabled"; ++count;
            }
            EXPECT_TRUE((input.eof()&&count==160)) << "complete direction and rounding-boundary fixture";
        }); });
}

 
TEST(MapFidelity, N10_turret_does_not_free_run) {
    SCOPED_TRACE("0x00451286 / 0x0045128C");

        session(turret_fixture, [](auto& view) {
            native(view, [] { placed().PrimaryFacing.SetCurrent(DirStruct(0));placed().UpdateAnimations(); });
            ticks(view,20);
            EXPECT_TRUE((placed().Anims[9]&&placed().Anims[9]->Animation.Value==28&&placed().Anims[9]->Animation.Rate==0)) << "unchanged direction remains frame 28 after many simulation ticks";
        });
}

 
TEST(MapFidelity, N11_turret_updates_with_facing) {
    SCOPED_TRACE("0x004509D0");

        session(turret_fixture, [](auto& view) {
            native(view, [] { placed().PrimaryFacing.SetCurrent(DirStruct(0x4000)); });ticks(view,1);
            EXPECT_TRUE((placed().Anims[9]&&placed().Anims[9]->Animation.Value==20)) << "normal tick updates turret after facing changes";
        });
}

 
TEST(MapFidelity, N12_turret_damage_transition) {
    SCOPED_TRACE("0x00451750 / 0x004509D0");

        session([](const auto& root) {
            turret_fixture(root);edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nTurretAnimDamaged=TDAM\n");
            std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[TDAM]\nStart=0\nEnd=32\nShadow=no\nRate=1\n";shp(root/"TDAM.SHP",32);
        }, [](auto& view) { native(view, [] {
            auto& b=placed();b.PrimaryFacing.SetCurrent(DirStruct(0x8000)); b.Health=25; b.UpdateAnimations();
            EXPECT_TRUE((b.Anims[9]&&std::string(b.Anims[9]->Type->ID)=="TDAM"&&b.Anims[9]->Animation.Value==12)) << "damaged turret preserves direction";
            b.Health=100;b.UpdateAnimations();
            EXPECT_TRUE((std::string(b.Anims[9]->Type->ID)=="TURRET"&&b.Anims[9]->Animation.Value==12)) << "repair restores healthy turret without losing facing";
        }); });
}

 
TEST(MapFidelity, N13_voxel_not_registered_as_shp) {
    SCOPED_TRACE("separate VXL/SHP resource domains");

        session([](const auto& root) {
            edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nTurret=yes\nTurretAnim=VOXTUR\nTurretAnimIsVoxel=yes\nVoxelBarrelFile=VOXBAR\nVoxelBarrelOffsetToBarrelEnd=1,2,3\n");
        }, [](auto& view) { native(view, [] {
            auto& b=placed();EXPECT_TRUE((b.Type->TurretAnimIsVoxel&&std::string(b.Type->BuildingAnim[9].Anim)=="VOXTUR")) << "VXL configuration is retained";
            EXPECT_TRUE((!b.Anims[9]&&!AnimTypeClass::Find("VOXTUR"))) << "unsupported VXL must not become a fake SHP animation";
            EXPECT_TRUE((std::string(b.Type->VoxelBarrelFile)=="VOXBAR"&&b.Type->VoxelBarrelOffsetToBarrelEnd==CoordStruct{1,2,3})) << "barrel filename and original vector key are retained";
        }); });
}

 
TEST(MapFidelity, N14_inactive_slots_not_auto_started) {
    SCOPED_TRACE("state-driven animation activation");

        session([](const auto& root) {
            edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nProductionAnim=SPIN\nPreProductionAnim=SPIN\nSpecialAnim=SPIN\nSuperAnim=SPIN\n");
        }, [](auto& view) { native(view, [] {
            for(int slot:{0,1,2,7,8,10,14}) EXPECT_TRUE((!placed().Anims[slot])) << "inactive conditional slots must not be started en masse";
        }); });
}

 
TEST(MapFidelity, N15_shp_turret_draw_submission) {
    SCOPED_TRACE("SHP facing state to original shape request");

        session(turret_fixture, [](auto& view) {
            native(view, [] {
                EXPECT_TRUE((placed().Anims[9] && placed().Anims[9]->Animation.Rate==0))
                    << "configured SHP turret exists and its timer is stopped before drawing";
            });
            auto out=draw(view);bool found=false;
            for(const auto& r:out.shapes) if(r.image==placed().Anims[9]->Type->Image) {
                found=true;EXPECT_TRUE((r.frame==28&&r.depth_mode==game::ShapeDepthMode::legacy)) << "turret frame reaches renderer-neutral request";
            }
            EXPECT_TRUE((found)) << "turret is not only a state field; a shape is submitted";
        });
}

 
TEST(MapFidelity, N16_body_uses_original_inclusive_half_limit) {
    SCOPED_TRACE("SHP body/shadow domains");

        session([](const auto& root) { shp(root/"BLDG.SHP",4); }, [](auto& view) {
            native(view, [] { placed().Animation.Value=100;game::map_object_changed(); });
            auto out=draw(view);EXPECT_TRUE((body_request(out).frame==2)) << "0x0043D290 clamps to Frames / 2 inclusively";
        });
}

 
TEST(MapFidelity, N17_no_shadow_keeps_original_half_limit) {
    SCOPED_TRACE("NoShadow frame domain");

        session([](const auto& root) { shp(root/"BLDG.SHP",4);edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nNoShadow=yes\n"); }, [](auto& view) {
            native(view, [] { placed().Health=100;placed().Animation.Value=3;game::map_object_changed(); });
            auto out=draw(view);EXPECT_TRUE((body_request(out).frame==2)) << "0x0043D290 retains its frame limit when NoShadow is set";
            for(const auto& r:out.shapes) if(r.image==placed().Type->Image)EXPECT_TRUE((!(r.flags&1))) << "NoShadow emits no body shadow request";
        });
}

 
TEST(MapFidelity, N18_construction_not_damaged_idle) {
    SCOPED_TRACE("0x0043EF90 construction branch");

        session([](const auto&){}, [](auto& view) { native(view, [] {
            auto& b=placed();b.BState=static_cast<int>(BStateType::Construction);b.Animation.Value=4;b.Health=1;
            EXPECT_TRUE((b.GetCurrentFrame()==4)) << "construction frame is not offset by ordinary damage bank";
        }); });
}

 
TEST(MapFidelity, N19_gate_damaged_bank) {
    SCOPED_TRACE("0x0043EF90 gate branch");

        session([](const auto& root) { edit(root/"RULESMD.INI","[BLDG]\n","[BLDG]\nGate=yes\nGateStages=9\n"); }, [](auto& view) { native(view, [] {
            auto& b=placed();b.GateStage=4;b.Health=100;EXPECT_TRUE((b.GetCurrentFrame()==0)) << "healthy non-construction gate body";
            b.Health=20;EXPECT_TRUE((b.GetCurrentFrame()==10)) << "damaged gate bank starts at GateStages+1";
        }); });
}

 
TEST(MapFidelity, N20_resource_cohort_192_3) {
    SCOPED_TRACE("native liveness invariant");
     no_starvation(192,3);
}

TEST(MapFidelity, N31_resource_germination_gates) {
    SCOPED_TRACE("YR 0x4838E0: local radar, Ground.Buildable and tile.AllowTiberium");
    session([](const auto&){},[](auto& view){native(view,[&]{
        auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{8,11});ASSERT_NE(cell,nullptr);
        auto* tile=IsometricTileTypeClass::Array.GetItemOrDefault(cell->IsoTileTypeIndex);ASSERT_NE(tile,nullptr);
        auto& ground=GroundType::Array[static_cast<int>(cell->LandType)];
        EXPECT_TRUE(cell->CanTiberiumGerminate(nullptr)) << "YR accepts a null resource argument";
        tile->AllowTiberium=false;EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));tile->AllowTiberium=true;
        ground.Buildable=false;EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));ground.Buildable=true;
        const auto rect=MapClass::Instance.VisibleRect;
        MapClass::Instance.VisibleRect={0,0,1,1};EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));
        MapClass::Instance.VisibleRect=rect;
        cell->SlopeIndex=1;EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));cell->SlopeIndex=0;
        const auto flags=cell->Flags;
        cell->Flags=static_cast<CellFlags>(static_cast<unsigned>(flags)|0x100u);
        EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));cell->Flags=flags;
        cell->OverlayTypeIndex=TiberiumClass::Array[0]->Image->ArrayIndex;
        EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));cell->OverlayTypeIndex=-1;
        auto& building=placed();cell->FirstObject=&building;
        EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));
        building.Type->Invisible=true;EXPECT_TRUE(cell->CanTiberiumGerminate(nullptr));building.Type->Invisible=false;
        building.Type->InvisibleInGame=true;EXPECT_TRUE(cell->CanTiberiumGerminate(nullptr));building.Type->InvisibleInGame=false;
        const int health=building.Health;building.Health=0;
        EXPECT_TRUE(cell->CanTiberiumGerminate(nullptr));building.Health=health;
        auto* terrain=TerrainClass::Array[0];cell->FirstObject=terrain;terrain->Type->SpawnsTiberium=false;
        EXPECT_TRUE(cell->CanTiberiumGerminate(nullptr)) << "ordinary terrain is allowed";
        terrain->Type->SpawnsTiberium=true;EXPECT_FALSE(cell->CanTiberiumGerminate(nullptr));
        cell->FirstObject=nullptr;
        EXPECT_TRUE(cell->CanTiberiumGerminate(nullptr));
    });});
}

TEST(MapFidelity, N32_resource_growth_registers_spread) {
    SCOPED_TRACE("YR 0x487190 registers spread before the next growth queue score");
    session([](const auto& root){resource_fixture(root,2200,2200);},[](auto& view){native(view,[&]{
        auto* cell=MapClass::Instance.TryGetCellAt(CellStruct{32,20});ASSERT_NE(cell,nullptr);
        auto* type=TiberiumClass::Array[0];ASSERT_TRUE(cell->IncreaseTiberium(0,1));
        type->RebuildSpread();type->SpreadLogic.Queue->Clear();type->SpreadLogic.Count=0;
        std::fill_n(type->SpreadLogic.CellIndexesWithTiberium,PriorityQueueClassNode::SurfaceDataCount(),false);
        view.scenario.SpecialFlags.TiberiumSpreads=true;
        view.scenario.Random=Randomizer(12345);Randomizer expected(12345);
        const int random=expected.Random();const unsigned magnitude=random<0?0u-static_cast<unsigned>(random):static_cast<unsigned>(random);
        ASSERT_TRUE(cell->IncreaseTiberium(0,1));
        ASSERT_EQ(type->SpreadLogic.Queue->Count,1);
        EXPECT_EQ(type->SpreadLogic.Queue->Top()->MapCoord,cell->MapCoords);
        EXPECT_EQ(type->SpreadLogic.Queue->Top()->Score,float(Unsorted::CurrentFrame+int(magnitude%50)));
        EXPECT_EQ(view.scenario.Random.Next1,expected.Next1);
        EXPECT_EQ(view.scenario.Random.Next2,expected.Next2);
    });});
}

 
TEST(MapFidelity, N21_resource_cohort_65_2) {
    SCOPED_TRACE("native liveness invariant");
     no_starvation(65,2);
}

 
TEST(MapFidelity, N22_resource_disabled) {
    SCOPED_TRACE("native configuration invariant");

        session([](const auto& root){resource_fixture(root,2);}, [](auto& view) {
            auto cells=seed(view,128);native(view,[&]{view.scenario.TiberiumGrowthEnabled=false;});ticks(view,30);
            for(auto* cell:cells)EXPECT_TRUE((cell->OverlayData==1)) << "disabled resource growth does not mutate cells";
            native(view,[&]{view.scenario.TiberiumGrowthEnabled=true;});ticks(view,100);
            for(auto* cell:cells)EXPECT_TRUE((cell->OverlayData>1)) << "reenabling does not leave a cohort stuck";
        });
}

 
TEST(MapFidelity, N23_resource_zero_probability) {
    SCOPED_TRACE("native configuration invariant");

        session([](const auto& root){resource_fixture(root,1);edit(root/"RULESMD.INI","GrowthPercentage=1","GrowthPercentage=0");}, [](auto& view) {
            auto cells=seed(view,128);ticks(view,30);for(auto* cell:cells)EXPECT_TRUE((cell->OverlayData==1)) << "zero growth probability remains zero";
        });
}

 
TEST(MapFidelity, N24_resource_reload_determinism) {
    SCOPED_TRACE("native determinism invariant, not original RNG trace");

        session([](const auto& root){resource_fixture(root,3);}, [](auto& view) {
            auto first=seed(view,192);ticks(view,20);std::vector<int> expected;
            for(auto* cell:first)expected.push_back(cell->OverlayData);
            EXPECT_TRUE((game::load_map_view(view,"world.map",9))) << "reload native world with same seed";
            auto second=seed(view,192);ticks(view,20);
            for(std::size_t i=0;i<second.size();++i)EXPECT_TRUE((second[i]->OverlayData==expected[i])) << "reload resets original frame, queues and scenario RNG deterministically";
        });
}

 
TEST(MapFidelity, N25_resource_reseed_registers_growth) {
    SCOPED_TRACE("native lifetime invariant");

        session([](const auto& root){resource_fixture(root,10);}, [](auto& view) {
            auto cells=seed(view,1);ticks(view,9);
            native(view,[&]{cells[0]->ReduceTiberium(100);EXPECT_TRUE((cells[0]->IncreaseTiberium(0,1))) << "reseed harvested cell";});
            auto& logic=TiberiumClass::Array[0]->GrowthLogic;
            EXPECT_TRUE((logic.Queue&&logic.Queue->Count>0&&logic.Count>0)) << "reseed registers in original growth queue";
            EXPECT_TRUE((logic.Nodes[logic.Count-1].MapCoord==cells[0]->MapCoords)) << "original registration owns cell coordinate";
            ticks(view,30);EXPECT_TRUE((cells[0]->OverlayData>1)) << "reseeded resource eventually grows";
        });
}

 
TEST(MapFidelity, N26_resource_spread_is_live) {
    SCOPED_TRACE("native spread liveness invariant");

        session([](const auto& root){resource_fixture(root,0,2);}, [](auto& view) {
            seed(view,128);ticks(view,8);game::MapWorldSnapshot snapshot{};
            EXPECT_TRUE((game::get_map_world_snapshot(view,snapshot)&&snapshot.resource_cells>128)) << "original spread queue produces real new resource cells";
        });
}

 // Explicit negative probes: run with --known-gaps. These are NOT included in
 // the passing acceptance count, and are never marked WILL_FAIL to look green.
 
TEST(MapFidelity, N30_original_pips_health_bar) {
    SCOPED_TRACE("0x006F64A0");

        session([](const auto& root){shp(root/"PIPS.SHP",8,8,8);}, [](auto& view) {
            native(view, []{placed().Select();game::map_object_changed();});
            auto out=draw(view);bool found=false;
            native(view,[&]{
                auto* pips=static_cast<SHPStruct*>(FileSystem::LoadFile("PIPS.SHP",false));
                for(const auto& request:out.shapes)if(request.image==pips)found=true;
            });
            EXPECT_TRUE((found)) << "original building HP must submit PIPS.SHP rather than a horizontal RGB rectangle";
        });
}

 
TEST(MapFidelity, G03_construction_uses_buildup) {
    SCOPED_TRACE("original image selection: 0x004513D0");

        session([](const auto& root){
            edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nBuildup=CONSTR\n");shp(root/"CONSTR.SHP",8);
        }, [](auto& view) {
            native(view, []{placed().BState=static_cast<int>(BStateType::Construction);
                placed().Animation.Value=0;placed().Health=100;game::map_object_changed();});
            auto out=draw(view);bool found=false;
            for(const auto& request:out.shapes)if(request.image==placed().Type->Buildup)found=true;
            EXPECT_TRUE((found)) << "loaded Buildup must be selected by the construction drawing path";
        });
}


}

TEST(MapFidelity, BridgeOriginalPlacementAndDrawing) {
    session([](const auto& root) {
        edit(root/"world.map", "Size=0,0,8,12\nLocalSize=0,0,8,12", "Size=0,0,32,32\nLocalSize=0,0,32,32");
        std::ostringstream extra;
        for(int i=114;i<=238;++i)extra<<i<<"=ORE"<<i<<'\n';
        edit(root/"RULESMD.INI","[ORE102]\n",extra.str()+"[ORE102]\n");
        std::ofstream rules(root/"RULESMD.INI",std::ios::app);
        for(int i:{24,25,237,238})rules<<"[ORE"<<i<<"]\nImage=BRIDGESYN\nTheater=no\n";
        std::ofstream(root/"ARTMD.INI",std::ios::app)<<"\n[BRIDGESYN]\nTheater=no\nNewTheater=no\n";
        shp(root/"BRIDGESYN.SHP",36);
        // Exercise the real map-loading order, not just InitializeBridge in
        // isolation: neighbour data must survive the second packed-data pass.
        std::vector<unsigned char>overlay(MapClass::MaxCells,255),frames(MapClass::MaxCells,0);
        const int indices[]={24,25,237,238};
        for(int n=0;n<4;++n){
            const int x=32+(n&1)*8,y=32+(n/2)*8;
            overlay[y*512+x]=indices[n];frames[y*512+x]=(n&1)?9:0;
            frames[(y-((n&1)?0:1))*512+x-((n&1)?1:0)]=77+n;
        }
        CCINIClass ini;
        const auto pack=[&](const char*section,const auto&bytes){
            std::vector<unsigned char>compressed(1024*1024);
            BufferPipe target(compressed.data(),int(compressed.size()));
            LCWPipe encoder(0,8192);encoder.Put_To(target);
            encoder.Put(bytes.data(),int(bytes.size()));encoder.Flush();
            ASSERT_TRUE(ini.WriteUUBlock(section,compressed.data(),target.Index));
        };
        pack("OverlayPack",overlay);pack("OverlayDataPack",frames);
        std::vector<unsigned char>text(1024*1024);BufferPipe target(text.data(),int(text.size()));
        ASSERT_GT(ini.WritePipe(target),0);
        std::ofstream map(root/"world.map",std::ios::app|std::ios::binary);
        map.write(reinterpret_cast<const char*>(text.data()),target.Index);
    },[](auto& view) {
        native(view,[]{
            const int indices[]={24,25,237,238};
            for(int n=0;n<4;++n){
                auto*c=MapClass::Instance.TryGetCellAt(CellStruct{short(32+(n&1)*8),short(32+(n/2)*8)});
                ASSERT_NE(c,nullptr);EXPECT_EQ(c->OverlayTypeIndex,indices[n]);
                EXPECT_EQ(static_cast<unsigned>(c->Flags),unsigned((n&1)?0x11380:0x11B80));
                EXPECT_EQ(int(c->OverlayData),(n&1)?9:0);
                auto*neighbour=c->GetNeighbourCell((n&1)?FacingType::West:FacingType::North);
                EXPECT_EQ(neighbour->BridgeOwnerCell,c);EXPECT_EQ(int(neighbour->OverlayData),77+n);
            }
        });
        EXPECT_TRUE(game::set_map_viewport(view,640,480));
        EXPECT_TRUE(game::center_map_view(view,0,1000));
        std::ifstream input(RA2_BRIDGE_FIXTURE);std::string tag;
        input>>tag;ASSERT_EQ(tag,"BRIDGE_REFERENCE_V1");
        unsigned init_count=0,draw_count=0;
        while(input>>tag){
            if(tag=="INIT"){
                int direction,count;std::string seed_text;input>>direction>>seed_text>>count;
                const auto seed=static_cast<unsigned>(std::stoul(seed_text,nullptr,0));
                native(view,[&]{
                    for(int y=37;y<44;++y)for(int x=37;x<44;++x){
                        auto*c=MapClass::Instance.TryGetCellAt(CellStruct{short(x),short(y)});
                        ASSERT_NE(c,nullptr);c->Flags=static_cast<CellFlags>(seed);c->OverlayData=0;c->BridgeOwnerCell=nullptr;
                    }
                    auto*owner=MapClass::Instance.TryGetCellAt(CellStruct{40,40});ASSERT_NE(owner,nullptr);
                    owner->InitializeBridge(static_cast<FacingType>(direction));
                    for(int n=0;n<count;++n){
                        int x,y,has_owner,frame;std::string flags;input>>x>>y>>flags>>has_owner>>frame;
                        auto*c=MapClass::Instance.TryGetCellAt(CellStruct{short(x),short(y)});ASSERT_NE(c,nullptr);
                        EXPECT_EQ(static_cast<unsigned>(c->Flags),std::stoul(flags,nullptr,0))<<x<<','<<y<<" direction="<<direction;
                        EXPECT_EQ(c->BridgeOwnerCell,has_owner?owner:nullptr);
                        EXPECT_EQ(int(c->OverlayData),frame);
                    }
                });++init_count;
            }else{
                ASSERT_EQ(tag,"DRAW");
                int index,x,y,level,frame;input>>index>>x>>y>>level>>frame;
                struct Expected {int frame,x,y,z,gradient,intensity;unsigned flags;}expected[2];
                for(auto&e:expected){std::string flags;input>>e.frame>>e.x>>e.y>>flags>>e.z>>e.gradient>>e.intensity;e.flags=std::stoul(flags,nullptr,0);}
                native(view,[&]{
                    auto*c=MapClass::Instance.TryGetCellAt(CellStruct{short(x),short(y)});ASSERT_NE(c,nullptr);
                    c->OverlayTypeIndex=index;c->OverlayData=frame;c->Level=level;c->Flags=CellFlags::BridgeOwner;c->Color1_Blue=700;
                    auto&w=*view.world->impl;ASSERT_NE(OverlayTypeClass::Array[index]->GetImage(),nullptr)<<OverlayTypeClass::Array[index]->ID<<" image="<<OverlayTypeClass::Array[index]->ImageFile<<" theater="<<OverlayTypeClass::Array[index]->Theater;w.decorated_cells={c};game::map_object_changed();game::rebuild_world_sprites(*view.world);
                    int found=0;
                    for(const auto&s:w.sprites)if(s.cell==c&&s.image==OverlayTypeClass::Array[index]->GetImage()){
                        ASSERT_LT(found,2);const auto&e=expected[found++];
                        EXPECT_EQ(s.frame,e.frame);EXPECT_EQ(s.position,(Point2D{e.x-view.tactical.TacticalPos.X,e.y-view.tactical.TacticalPos.Y}));
                        EXPECT_TRUE(s.original_depth);EXPECT_EQ(s.flags,e.flags);EXPECT_EQ(s.depth_adjustment,e.z);
                        EXPECT_EQ(s.gradient,e.gradient);EXPECT_EQ(s.intensity,e.intensity);
                    }
                    EXPECT_EQ(found,2)<<index<<' '<<x<<','<<y<<" level="<<level<<" frame="<<frame<<" camera="<<view.tactical.TacticalPos.X<<","<<view.tactical.TacticalPos.Y<<" sprites="<<w.sprites.size();
                });++draw_count;
            }
            ASSERT_TRUE(bool(input));
        }
        EXPECT_EQ(init_count,12u);EXPECT_EQ(draw_count,3456u);
    });
}

TEST(MapFidelity, BuildingBodyBibOriginalRequests) {
    session([](const auto&root){
        // Original reference doubles GetImage: supply equally shaped but
        // distinct normal/buildup images so image selection stays exercised.
        edit(root/"ARTMD.INI","[BLDG]\n","[BLDG]\nBibShape=BBIB\nBuildup=CONSTR\n");
        shp(root/"BLDG.SHP",6);shp(root/"BBIB.SHP",6);shp(root/"CONSTR.SHP",6);
    },[](auto&view){
        std::ifstream in(RA2_BUILDING_DRAW_FIXTURE);std::string tag;in>>tag;
        ASSERT_EQ(tag,"BUILDING_DRAWING_V1");
        int foundation,height,frame,bstate,no_shadow,count,cases=0;
        while(in>>foundation>>height>>frame>>bstate>>no_shadow>>count){
            native(view,[&]{auto&b=placed();auto&t=*b.Type;
                b.Health=t.Strength;b.Location.Z=height;b.Animation.Value=frame;b.BState=bstate;
                t.Foundation=static_cast<Foundation>(foundation);t.NoShadow=no_shadow;
                t.NormalZAdjust=-20;t.ZShapePointMove={3,-2};game::map_object_changed();
            });
            const auto output=draw(view);const auto&b=placed();
            std::vector<const game::ShapeDrawingRequest*>requests;
            for(const auto&r:output.shapes)if(r.image==b.GetImage()||r.image==b.Type->BibShape)requests.push_back(&r);
            ASSERT_EQ(requests.size(),std::size_t(count))<<"foundation="<<foundation<<" height="<<height<<" state="<<bstate;
            auto origin=TacticalClass::CoordsToScreen(b.GetRenderCoords());origin.X-=view.tactical.TacticalPos.X;origin.Y-=view.tactical.TacticalPos.Y;
            for(int n=0;n<count;++n){
                int bib,expected_frame,x,y,z,gradient,aux,dx,dy;std::string flags;
                in>>bib>>expected_frame>>x>>y>>flags>>z>>gradient>>aux>>dx>>dy;
                const auto&r=*requests[n];
                EXPECT_EQ(r.image,bib?b.Type->BibShape:b.GetImage());EXPECT_EQ(r.frame,expected_frame);
                EXPECT_EQ(r.position,(Point2D{origin.X+x-50,origin.Y+y-50}));
                EXPECT_EQ(r.flags,std::stoul(flags,nullptr,0));EXPECT_EQ(r.depth_mode,game::ShapeDepthMode::legacy);
                EXPECT_EQ(r.depth_adjustment,z);EXPECT_EQ(r.gradient,gradient);EXPECT_EQ(r.depth_image!=nullptr,aux!=0);
                if(aux)EXPECT_EQ(r.depth_offset,(Point2D{dx,dy}));
            }
            ASSERT_TRUE(bool(in));++cases;
        }
        EXPECT_EQ(cases,792);
    });
}

TEST(MapFidelity, BuildingPlacementUsesRampFloor) {
    session([](const auto&){},[](auto&view){native(view,[&]{
        auto&b=placed();auto*c=MapClass::Instance.TryGetCellAt(b.Location);ASSERT_NE(c,nullptr);
        const auto xy=b.Location;
        for(int level:{0,3,7})for(int slope=0;slope<=20;++slope){
            game::detach_map_object(b);c->Level=level;c->SlopeIndex=slope;b.Location=xy;b.Location.Z=-999;
            game::attach_map_object(b);
            EXPECT_EQ(b.Location.X,xy.X);EXPECT_EQ(b.Location.Y,xy.Y);
            EXPECT_EQ(b.Location.Z,c->GetFloorHeight({xy.X,xy.Y}));
            // 0x00464A70 corrects the original virtual as well as map placement.
            CoordStruct location=xy;ObjectTypeClass*type=b.Type;
            ASSERT_EQ(type->vt_entry_6C(&location,&location),&location);
            EXPECT_EQ(location,b.Location);
        }
    });});
}

TEST(MapFidelity, MapBuildingListExtendsBaseRegistry) {
    session([](const auto& root) {
        // As in ALL01UMD, map-local numeric keys collide with base rules keys.
        edit(root/"world.map","[Terrain]\n",
            "1=Neutral,EXTRA,256,9,5,0,None,1,0,1,0,0,None,None,None,0,0\n[Terrain]\n");
        std::ofstream(root/"world.map",std::ios::app)
            << "\n[BuildingTypes]\n0=EXTRA\n1=catime\n2=<none>\n3=\n4=None\n99=bldg\n"
               "[BLDG]\nStrength=240\n[EXTRA]\nStrength=80\n";
        std::ofstream(root/"ARTMD.INI",std::ios::app)
            << "\n[EXTRA]\nFoundation=1x1\nNewTheater=no\n";
        shp(root/"EXTRA.SHP",2);
    }, [](auto& view) {
        const auto verify=[&] {
            game::MapWorldSnapshot snapshot{};
            ASSERT_TRUE(game::get_map_world_snapshot(view,snapshot));
            EXPECT_EQ(snapshot.buildings,2u);
            EXPECT_EQ(snapshot.unknown_records,0u);
            native(view,[&] {
                ASSERT_EQ(BuildingTypeClass::Array.Count,3);
                EXPECT_STREQ(BuildingTypeClass::Array[0]->ID,"BLDG");
                EXPECT_STREQ(BuildingTypeClass::Array[1]->ID,"CATIME");
                EXPECT_STREQ(BuildingTypeClass::Array[2]->ID,"EXTRA");
                EXPECT_EQ(BuildingTypeClass::Find("catime"),BuildingTypeClass::Array[1]);
                auto* base=BuildingTypeClass::Find("BLDG");
                ASSERT_NE(base,nullptr);
                EXPECT_EQ(BuildingTypeClass::Find("bldg"),base);
                EXPECT_EQ(base->Strength,240); // Map attributes still override.
                EXPECT_STREQ(base->Name,"Native Building"); // Unchanged base attributes survive.
                ASSERT_EQ(BuildingClass::Array.Count,2);
                EXPECT_EQ(BuildingClass::Array[0]->Type,base);
                EXPECT_EQ(BuildingClass::Array[0]->Health,120);
                EXPECT_EQ(BuildingClass::Array[1]->Type,BuildingTypeClass::Array[2]);
                EXPECT_EQ(BuildingClass::Array[1]->Health,80);
            });
        };
        verify();
        ASSERT_TRUE(game::load_map_view(view,"world.map",9)) << game::map_view_error(view);
        verify(); // Registration order and counts remain stable across reloads.
    });
}
