// Display/state subset of 0x0045FE50; resource naming/loading from 0x0045F230.
// Rules.Image selects the ART section. ART.Image selects only the body asset.
// Construction fields adapt OpenTS 44fac744 builtype.cpp Read_INI.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/RulesClass.h"
#include "RulesClassReaders.hpp"
#include <cmath>
#include "type_resources.hpp"
#include "building_voxel.hpp"
#include "map_world.hpp"
#include <algorithm>
#include <bit>
#include <cstdio>

namespace {
void register_slot(const BuildingAnimStruct& slot) {
    for (const char* name : {slot.Anim, slot.Damaged, slot.Garrisoned})
        if (*name && !INIClass::IsBlankValue(name)) AnimTypeClass::FindOrAllocate(name);
}
void read_animation(CCINIClass& ini, const char* section, const char* key,
        BuildingAnimStruct& slot) {
    char field[80]{};
    ini.ReadString(section, key, slot.Anim, slot.Anim, sizeof(slot.Anim));
    std::snprintf(field, sizeof(field), "%sDamaged", key);
    ini.ReadString(section, field, slot.Damaged, slot.Damaged, sizeof(slot.Damaged));
    if (!*slot.Damaged) std::snprintf(slot.Damaged, sizeof(slot.Damaged), "%s", slot.Anim);
    std::snprintf(field, sizeof(field), "%sGarrisoned", key);
    ini.ReadString(section, field, slot.Garrisoned, slot.Garrisoned, sizeof(slot.Garrisoned));
    if (!*slot.Garrisoned) std::snprintf(slot.Garrisoned, sizeof(slot.Garrisoned), "%s", slot.Anim);
#define SLOT_INT(suffix, member) \
    std::snprintf(field, sizeof(field), "%s%s", key, suffix); \
    slot.member = ini.ReadInteger(section, field, slot.member)
    SLOT_INT("X", Position.X); SLOT_INT("Y", Position.Y);
    SLOT_INT("ZAdjust", ZAdjust); SLOT_INT("YSort", YSort);
#undef SLOT_INT
#define SLOT_BOOL(suffix, member) \
    std::snprintf(field, sizeof(field), "%s%s", key, suffix); \
    slot.member = ini.ReadBool(section, field, slot.member)
    SLOT_BOOL("Powered", Powered); SLOT_BOOL("PoweredLight", PoweredLight);
    SLOT_BOOL("PoweredEffect", PoweredEffect); SLOT_BOOL("PoweredSpecial", PoweredSpecial);
#undef SLOT_BOOL
    register_slot(slot);
}
bool read_shapes(BuildingTypeClass& type, CCINIClass& art) {
    char name[64]{};
    art.ReadString(type.ImageFile, "Image", "", name, sizeof(name));
    if (type.ImageAllocated) YRMemory::Deallocate(type.Image);
    type.Image = nullptr; type.ImageAllocated = false;
    if (!game::load_building_cached_shape(*name ? name : type.ImageFile, type.Theater,
            type.Image, type.TheaterSpecificID, sizeof(type.TheaterSpecificID))) return false;
    if (type.Image) type.MaxDimension = std::max({8, int(type.Image->Width), int(type.Image->Height)});
    art.ReadString(type.ImageFile, "Buildup", type.BuildupFile,
        type.BuildupFile, sizeof(type.BuildupFile));
    if (!game::load_building_owned_shape(type.BuildupFile, type.Buildup, type.BuildupLoaded)) return false;
    if(type.Buildup){
        // 0x0045F230: body/shadow halves; gates use GateStages + 1.
        auto* image=type.Buildup;
        if(auto* reference=image->AsReference()){reference->Load();image=reference->Data;}
        if(!image)return false;
        auto& sequence=type.BuildingAnimFrame[0];sequence.dwUnknown=0;
        sequence.FrameCount=type.Gate?type.GateStages+1:int(image->Frames)/2;
        sequence.FrameDuration=1;
        if(sequence.FrameCount>0){
            const double minutes=RulesClass::Instance->BuildupTime;
            const double product=minutes*900.0;
            const auto truncate=[](double nearest,double residual){return (nearest>0&&residual<0)||(nearest<0&&residual>0)?std::nextafter(nearest,0.0):nearest;};
            const double ticks=truncate(product,std::fma(minutes,900.0,-product));
            const double ratio=ticks/sequence.FrameCount;
            const double exact=truncate(ratio,std::fma(-ratio,double(sequence.FrameCount),ticks));
            if(!std::isfinite(exact)||exact<0||exact>=2147483648.0)return false;
            sequence.FrameDuration=static_cast<int>(exact);
        }
    }
    struct OwnedPart { const char* key; SHPStruct*& image; bool& owned; };
    const OwnedPart parts[] = {
        {"DeployingAnim", type.DeployingAnim, type.DeployingAnimLoaded},
        {"RoofDeployingAnim", type.RoofDeployingAnim, type.RoofDeployingAnimLoaded},
        {"UnderDoorAnim", type.UnderDoorAnim, type.UnderDoorAnimLoaded},
        {"UnderRoofDoorAnim", type.UnderRoofDoorAnim, type.UnderRoofDoorAnimLoaded},
        {"Rubble", type.Rubble, type.RubbleLoaded},
        {"BibShape", type.BibShape, type.BibShapeLoaded}
    };
    for (const auto& part : parts) {
        art.ReadString(type.ImageFile, part.key, "", name, sizeof(name));
        if (!game::load_building_owned_shape(name, part.image, part.owned)) return false;
    }
    // These original members have no ownership flag; retain FileSystem's cache.
    art.ReadString(type.ImageFile, "DoorAnim", "", name, sizeof(name));
    if (!game::load_building_cached_shape(name, false, type.DoorAnim)) return false;
    art.ReadString(type.ImageFile, "SpecialZOverlay", "", name, sizeof(name));
    return game::load_building_cached_shape(name, false, type.SpecialZOverlay);
}
}

BuildingTypeClass* YRPP_FASTCALL BuildingTypeClass::FindOrAllocate(const char* id) {
    return FindOrAllocate(id, ConstructionDefaults{});
}
bool BuildingTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini || !TechnoTypeClass::LoadFromINI(ini)) return false;
    // OpenTS 44fac744 builtype.cpp Read_INI; YR 0x0045FE50 reads these
    // from rules[ID], retaining the existing defaults on absent keys.
    BuildCat = static_cast<::BuildCat>(ini->ReadBuildCat(ID, "BuildCat", static_cast<int>(BuildCat)));
    Adjacent = ini->ReadInteger(ID, "Adjacent", Adjacent);
    BaseNormal = ini->ReadBool(ID, "BaseNormal", BaseNormal);
    EligibileForAllyBuilding = ini->ReadBool(ID, "EligibileForAllyBuilding", EligibileForAllyBuilding);
    ConstructionYard = ini->ReadBool(ID, "ConstructionYard", ConstructionYard);
    read_rule_type(*ini,ID,"FreeUnit",AbstractType::UnitType,FreeUnit);
    DockUnload=ini->ReadBool(ID,"DockUnload",DockUnload);
    Refinery=ini->ReadBool(ID,"Refinery",Refinery);
    NumberImpassableRows=ini->ReadInteger(ID,"NumberImpassableRows",NumberImpassableRows);
    ResourceDestination=ini->ReadBool(ID,"ResourceDestination",ResourceDestination);
    auto& art = game::type_art_ini();
    Foundation = static_cast<::Foundation>(art.ReadFoundation(ImageFile, "Foundation", static_cast<int>(Foundation)));
    FoundationData = game::native_building_foundation(static_cast<int>(Foundation));
    if (!FoundationData) return false;
    // OpenTS ExitLists, YR selects the table by Foundation at 0x460Dxx.
    // Kept in the original BuildingType field, shared by all instances.
    static CellStruct exits[22][30]={
        {{0,1},{-1,1},{1,1},{-1,0},{1,0},{0,-1},{-1,-1},{1,-1},{0x7FFF,0x7FFF}},
        {{0,1},{1,1},{-1,1},{2,1},{-1,0},{2,0},{0,-1},{1,-1},{-1,-1},{2,-1},{0x7FFF,0x7FFF}},
        {{0,2},{1,2},{-1,2},{-1,1},{1,1},{-1,0},{1,0},{0,-1},{-1,-1},{1,-1},{0x7FFF,0x7FFF}},
        {{0,2},{1,2},{-1,2},{2,2},{-1,1},{2,1},{-1,0},{2,0},{0,-1},{1,-1},{-1,-1},{2,-1},{0x7FFF,0x7FFF}},
        {{0,3},{1,3},{-1,3},{2,3},{-1,2},{2,2},{-1,1},{2,1},{-1,0},{2,0},{0,-1},{1,-1},{-1,-1},{2,-1},{0x7FFF,0x7FFF}},
        {{0,2},{1,2},{2,2},{-1,2},{3,2},{-1,1},{3,1},{-1,0},{3,0},{0,-1},{1,-1},{2,-1},{-1,-1},{3,-1},{0x7FFF,0x7FFF}},
        {{0,3},{1,3},{2,3},{-1,3},{3,3},{-1,2},{3,2},{-1,1},{3,1},{-1,0},{3,0},{0,-1},{1,-1},{2,-1},{-1,-1},{3,-1},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},{-1,0},{3,0},{-1,1},{3,1},{-1,2},{3,2},{-1,3},{3,3},{-1,4},{3,4},{-1,5},{0,5},{1,5},{2,5},{3,5},{0x7FFF,0x7FFF}},
        {{0,2},{1,2},{2,2},{3,2},{-1,2},{4,2},{-1,1},{4,1},{-1,0},{4,0},{0,-1},{1,-1},{2,-1},{3,-1},{-1,-1},{4,-1},{0x7FFF,0x7FFF}},
        {{0,0},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{1,1},{-1,2},{1,2},{-1,3},{0,3},{1,3},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},{-1,0},{3,0},{-1,1},{0,1},{1,1},{2,1},{3,1},{0x7FFF,0x7FFF}},
        {{0,3},{1,3},{2,3},{-1,3},{3,3},{-1,2},{3,2},{-1,1},{3,1},{-1,0},{3,0},{0,-1},{1,-1},{2,-1},{-1,-1},{3,-1},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{1,1},{-1,2},{1,2},{-1,3},{1,3},{-1,4},{0,4},{1,4},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{1,1},{-1,2},{1,2},{-1,3},{1,3},{-1,4},{1,4},{-1,5},{0,5},{1,5},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{2,-1},{-1,0},{2,0},{-1,1},{2,1},{-1,2},{2,2},{-1,3},{2,3},{-1,5},{2,5},{-1,6},{0,6},{1,6},{2,6},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{2,-1},{-1,0},{2,0},{-1,1},{2,1},{-1,2},{2,2},{-1,3},{2,3},{-1,5},{0,5},{1,5},{2,5},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},{4,-1},{5,-1},{-1,0},{5,0},{-1,1},{5,1},{-1,2},{5,2},{-1,3},{0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{0x7FFF,0x7FFF}},
        {{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},{4,-1},{-1,4},{0,4},{1,4},{2,4},{3,4},{4,4},{-1,0},{-1,1},{-1,2},{-1,3},{-1,4},{4,0},{4,1},{4,2},{4,3},{4,4},{0x7FFF,0x7FFF}},
        {{0,4},{1,4},{2,4},{-1,4},{3,4},{-1,2},{3,2},{-1,1},{3,1},{-1,0},{3,0},{0,-1},{1,-1},{2,-1},{-1,-1},{3,-1},{0x7FFF,0x7FFF}},
        {{2,-1},{0x7FFF,0x7FFF}},
        {{0x7FFF,0x7FFF}}
    };
    const int foundation=static_cast<int>(Foundation);
    FoundationOutside=foundation>=0&&foundation<22?exits[foundation]:nullptr;
    char factory[64]{};if(ini->ReadString(ID,"Factory","",factory,sizeof(factory))){
        Factory=AbstractType::None;
        if(!_strcmpi(factory,"InfantryType"))Factory=AbstractType::InfantryType;
        else if(!_strcmpi(factory,"UnitType"))Factory=AbstractType::UnitType;
        else if(!_strcmpi(factory,"AircraftType"))Factory=AbstractType::AircraftType;
        else if(!_strcmpi(factory,"BuildingType"))Factory=AbstractType::BuildingType;
    }
    ini->ReadPoint3D(ExitCoord,ID,"ExitCoord",ExitCoord);
#define RULE_BOOL(f) f = ini->ReadBool(ID, #f, f)
    RefinerySmokeFrames=ini->ReadInteger(ID,"RefinerySmokeFrames",RefinerySmokeFrames);
    RULE_BOOL(Powered); RULE_BOOL(PoweredSpecial); RULE_BOOL(Overpowerable);
    // 0x0045FE50: radar capability participates in House 0x00508DF0.
    RULE_BOOL(Radar); RULE_BOOL(TogglePower); RULE_BOOL(UnitAbsorb); RULE_BOOL(InfantryAbsorb);
    RULE_BOOL(Unsellable); RULE_BOOL(Capturable); RULE_BOOL(ClickRepairable); RULE_BOOL(CanBeOccupied);
    RULE_BOOL(ShowOccupantPips); RULE_BOOL(CanOccupyFire); RULE_BOOL(NeedsEngineer);
    RULE_BOOL(Wall); RULE_BOOL(InvisibleInGame); RULE_BOOL(PlaceAnywhere);
    RULE_BOOL(ExtraDamageStage); RULE_BOOL(TurretAnimIsVoxel); RULE_BOOL(BarrelAnimIsVoxel);
    RULE_BOOL(GDIBarracks); RULE_BOOL(NODBarracks); RULE_BOOL(YuriBarracks);
    RULE_BOOL(LeaveRubble); RULE_BOOL(HasSpotlight); RULE_BOOL(Gate); RULE_BOOL(WeaponsFactory);
    RULE_BOOL(LaserFencePost); RULE_BOOL(LaserFence); RULE_BOOL(FirestormWall);
#undef RULE_BOOL
#define ART_BOOL(f) f = art.ReadBool(ImageFile, #f, f)
    ART_BOOL(Bib); ART_BOOL(TerrainPalette); ART_BOOL(Flat); ART_BOOL(DamagedDoor);
    ART_BOOL(ChargeAnim); ART_BOOL(IsAnimDelayedFire);
#undef ART_BOOL
    LightVisibility=ini->ReadInteger(ID,"LightVisibility",LightVisibility);
    // 0x0045EEC0: floating INI levels are stored in thousandths, with +0.1
    // before truncation (including negative light sources).
    LightIntensity=int(ini->ReadDouble(ID,"LightIntensity",LightIntensity/1000.0)*1000.0+0.1);
    LightRedTint=int(ini->ReadDouble(ID,"LightRedTint",LightRedTint/1000.0)*1000.0+0.1);
    LightGreenTint=int(ini->ReadDouble(ID,"LightGreenTint",LightGreenTint/1000.0)*1000.0+0.1);
    LightBlueTint=int(ini->ReadDouble(ID,"LightBlueTint",LightBlueTint/1000.0)*1000.0+0.1);
    BarrelStartPitch=32*ini->ReadInteger(ID,"BarrelStartPitch",BarrelStartPitch/32);
    Height = art.ReadInteger(ImageFile, "Height", Height);
    NormalZAdjust = art.ReadInteger(ImageFile, "NormalZAdjust", NormalZAdjust);
    SpecialZOverlayZAdjust = art.ReadInteger(ImageFile, "SpecialZOverlayZAdjust", SpecialZOverlayZAdjust);
    OccupyHeight = ini->ReadInteger(ID, "OccupyHeight", OccupyHeight);
    // 0x460F70..0x460F87 uses rules[ID], not the ART/image section.
    ini->ReadPoint3D(TargetCoordOffset,ID,"TargetCoordOffset",TargetCoordOffset);
    MaxNumberOccupants = ini->ReadInteger(ID, "MaxNumberOccupants", MaxNumberOccupants);
    GateStages = ini->ReadInteger(ID, "GateStages", GateStages);
    DoorStages = art.ReadInteger(ImageFile, "DoorStages", DoorStages);
    art.ReadPoint2D(ZShapePointMove, ImageFile, "ZShapePointMove", ZShapePointMove);
    ExtraLight = WORD(art.ReadInteger(ImageFile, "ExtraLight", static_cast<short>(ExtraLight)));
    // 0x0045FE50 reads contiguous DamageFireOffset0..7. Missing entries
    // use the parser sentinel, then become the original zero terminator.
    for (int i=0;i<8;++i) {
        char key[32]{};std::snprintf(key,sizeof(key),"DamageFireOffset%d",i);
        Point2D missing{0xFFFF,0xFFFF};
        art.ReadPoint2D(DamageFireOffset[i],ImageFile,key,missing);
        if (DamageFireOffset[i]==missing) {DamageFireOffset[i]={0,0};break;}
    }
    // 0x00461060..0x004610D6. NEG wraps as a 32-bit integer; ExtraPower's
    // nonnegative branch deliberately leaves an existing ExtraPowerDrain.
    const auto negate=[](int value){return std::bit_cast<int>(0u-static_cast<unsigned>(value));};
    const int power=ini->ReadInteger(ID,"Power",PowerBonus>0?PowerBonus:negate(PowerDrain));
    PowerBonus=power<0?0:power;
    PowerDrain=power<0?negate(power):0;
    ExtraPowerBonus=ini->ReadInteger(ID,"ExtraPower",ExtraPowerBonus>0?ExtraPowerBonus:negate(ExtraPowerDrain));
    if(ExtraPowerBonus<0){ExtraPowerDrain=negate(ExtraPowerBonus);ExtraPowerBonus=0;}
    Upgrades = std::clamp(ini->ReadInteger(ID, "Upgrades", Upgrades), 0, 3);
    if (!read_shapes(*this, art)) return false;

    constexpr const char* keys[] = {"PowerUp1Anim", "PowerUp2Anim", "PowerUp3Anim",
        "ActiveAnim", "ActiveAnimTwo", "ActiveAnimThree", "ActiveAnimFour",
        "PreProductionAnim", "ProductionAnim", "TurretAnim", "SpecialAnim",
        "SpecialAnimTwo", "SpecialAnimThree", "SpecialAnimFour", "SuperAnim",
        "SuperAnimTwo", "SuperAnimThree", "SuperAnimFour", "IdleAnim", "LowPower", "SuperLowPower"};
    for (int i = 3; i < 21; ++i)
        if (i != 9) read_animation(art, ImageFile, keys[i], BuildingAnim[i]);

    // 0x0045FE50 uses rules[ID] for the turret, but rules[ImageFile] for
    // TurretAnimGarrisoned. Do not flatten this into a generic ART slot reader.
    auto& turret = BuildingAnim[9];
    ini->ReadString(ID, "TurretAnim", turret.Anim, turret.Anim, sizeof(turret.Anim));
    ini->ReadString(ID, "TurretAnimDamaged", turret.Damaged, turret.Damaged, sizeof(turret.Damaged));
    if (!*turret.Damaged) std::snprintf(turret.Damaged, sizeof(turret.Damaged), "%s", turret.Anim);
    ini->ReadString(ImageFile, "TurretAnimGarrisoned", turret.Garrisoned, turret.Garrisoned, sizeof(turret.Garrisoned));
    if (!*turret.Garrisoned) std::snprintf(turret.Garrisoned, sizeof(turret.Garrisoned), "%s", turret.Anim);
    turret.Position.X = ini->ReadInteger(ID, "TurretAnimX", turret.Position.X);
    turret.Position.Y = ini->ReadInteger(ID, "TurretAnimY", turret.Position.Y);
    turret.ZAdjust = ini->ReadInteger(ID, "TurretAnimZAdjust", turret.ZAdjust);
    turret.YSort = ini->ReadInteger(ID, "TurretAnimYSort", turret.YSort);
    if (!TurretAnimIsVoxel) register_slot(turret); // VXL names are not AnimType IDs.
    ini->ReadString(ID, "VoxelBarrelFile", VoxelBarrelFile, VoxelBarrelFile, sizeof(VoxelBarrelFile));
#define RULE_POINT(f) ini->ReadPoint3D(f, ID, #f, f)
    RULE_POINT(VoxelBarrelOffsetToPitchPivotPoint); RULE_POINT(VoxelBarrelOffsetToRotatePivotPoint);
    RULE_POINT(VoxelBarrelOffsetToBuildingPivotPoint); RULE_POINT(VoxelBarrelOffsetToBarrelEnd);
#undef RULE_POINT
    // Only declared upgrade slots are read in 0x0045FE50.
    for (int i = 0; i < Upgrades; ++i) {
        auto& slot = BuildingAnim[i]; char key[64]{};
        art.ReadString(ImageFile, keys[i], slot.Anim, slot.Anim, sizeof(slot.Anim));
        std::snprintf(key, sizeof(key), "PowerUp%dDamagedAnim", i + 1);
        art.ReadString(ImageFile, key, slot.Damaged, slot.Damaged, sizeof(slot.Damaged));
#define POWERUP_INT(suffix, member) \
        std::snprintf(key, sizeof(key), "PowerUp%d%s", i + 1, suffix); \
        slot.member = art.ReadInteger(ImageFile, key, slot.member)
        POWERUP_INT("LocXX", Position.X); POWERUP_INT("LocYY", Position.Y);
        POWERUP_INT("LocZZ", ZAdjust); POWERUP_INT("YSort", YSort);
#undef POWERUP_INT
        register_slot(slot);
    }
    constexpr const char* groups[] = {"AnimConstruction", "AnimIdle", "AnimActive", "AnimFull", "AnimAux1", "AnimAux2"};
    constexpr const char* aliases[] = {"Anim.Construction", "Anim.Idle", "Anim.Active", "Anim.Full", "Anim.Aux1", "Anim.Aux2"};
    for (int i = 0; i < 6; ++i) {
        auto& f = BuildingAnimFrame[i]; char value[96]{};
        if (art.ReadString(ImageFile, groups[i], "", value, sizeof(value)) ||
                art.ReadString(ImageFile, aliases[i], "", value, sizeof(value))) {
            int start = 0, count = 1, duration = 1;
            if (std::sscanf(value, "%d,%d,%d", &start, &count, &duration) >= 2) {
                f.dwUnknown = std::max(start, 0); f.FrameCount = std::max(count, 1);
                f.FrameDuration = std::max(duration, 0);
            }
        }
    }
    return game::load_building_voxels(*this);
}
