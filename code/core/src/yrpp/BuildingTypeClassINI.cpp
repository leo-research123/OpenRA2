// Building rules/ART reader at 0x0045FE50; resource loading remains separate.
// Rules.Image selects the ART section. ART.Image selects only the body asset.
// Construction fields adapt OpenTS 44fac744 builtype.cpp Read_INI.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "RulesClassReaders.hpp"
#include "type_resources.hpp"
#include "map_world.hpp"
#include <bit>
#include <cstdio>
#include <cstring>
#include <iterator>

namespace {
template <std::size_t Size>
void read_nonempty_name(CCINIClass& ini, const char* section, const char* key, char (&name)[Size]) {
    char value[Size]{};
    if (ini.ReadString(section, key, "", value, Size)) std::memcpy(name, value, std::strlen(value) + 1);
}
void read_animation(CCINIClass& ini, const char* section, const char* key, BuildingAnimStruct& slot, bool powered) {
    char field[80]{};
    read_nonempty_name(ini, section, key, slot.Anim);
    std::snprintf(field, sizeof(field), "%sDamaged", key);
    read_nonempty_name(ini, section, field, slot.Damaged);
    if (!*slot.Damaged) std::memcpy(slot.Damaged, slot.Anim, std::strlen(slot.Anim) + 1);
    std::snprintf(field, sizeof(field), "%sGarrisoned", key);
    read_nonempty_name(ini, section, field, slot.Garrisoned);
    if (!*slot.Garrisoned) std::memcpy(slot.Garrisoned, slot.Anim, std::strlen(slot.Anim) + 1);
    if (!*slot.Anim && !*slot.Damaged && !*slot.Garrisoned) return;
#define SLOT_INT(suffix, member)                                                                                       \
    std::snprintf(field, sizeof(field), "%s%s", key, suffix);                                                          \
    slot.member = ini.ReadInteger(section, field, slot.member)
    SLOT_INT("X", Position.X);
    SLOT_INT("Y", Position.Y);
    SLOT_INT("ZAdjust", ZAdjust);
    SLOT_INT("YSort", YSort);
#undef SLOT_INT
    // Production and PreProduction do not read the powered attributes.
    if (!powered) return;
#define SLOT_BOOL(suffix, member)                                                                                      \
    std::snprintf(field, sizeof(field), "%s%s", key, suffix);                                                          \
    slot.member = ini.ReadBool(section, field, slot.member)
    SLOT_BOOL("Powered", Powered);
    SLOT_BOOL("PoweredLight", PoweredLight);
    SLOT_BOOL("PoweredEffect", PoweredEffect);
    SLOT_BOOL("PoweredSpecial", PoweredSpecial);
#undef SLOT_BOOL
}
void read_animation_frame(CCINIClass& art, const char* section, const char* key, BuildingAnimFrameStruct& frame) {
    char fallback[64], value[64]{};
    int start = std::bit_cast<int>(frame.dwUnknown);
    int count = frame.FrameCount, duration = frame.FrameDuration;
    std::snprintf(fallback, sizeof(fallback), "%d,%d,%d", start, count, duration);
    if (art.ReadString(section, key, fallback, value, sizeof(value)) > 0) {
        // Partial conversions retain the components not assigned by sscanf.
        std::sscanf(value, "%d,%d,%d", &start, &count, &duration);
        frame.dwUnknown = std::bit_cast<DWORD>(start);
        frame.FrameCount = count;
        frame.FrameDuration = duration;
    }
}
int read_building_light(CCINIClass& ini, const char* section, const char* key, int value) {
    // 0x00460CA5: the default uses integer division before promotion.
    const double level = ini.ReadDouble(section, key, double(value / 1000));
    return rule_integer(level * 1000.0 + 0.1);
}

}

BuildingTypeClass* YRPP_FASTCALL BuildingTypeClass::FindOrAllocate(const char* id) {
    return FindOrAllocate(id, ConstructionDefaults{});
}
bool BuildingTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    try {
        ini->Reset();
        if (!TechnoTypeClass::LoadFromINI(ini)) return false;
        auto& art = game::type_art_ini();
#define RULE_INT(f) f = ini->ReadInteger(ID, #f, f)
#define RULE_BOOL(f) f = ini->ReadBool(ID, #f, f)
#define RULE_POINT(f) ini->ReadPoint3D(f, ID, #f, f)
#define RULE_FLOAT(f) f = static_cast<float>(ini->ReadDouble(ID, #f, f))
        BuildCat = static_cast<::BuildCat>(ini->ReadBuildCat(ID, "BuildCat", static_cast<int>(BuildCat)));
        RULE_INT(AntiInfantryValue);
        RULE_INT(AntiArmorValue);
        RULE_INT(AntiAirValue);
        RULE_BOOL(HasSpotlight);
        RULE_POINT(HalfDamageSmokeLocation1);
        RULE_POINT(HalfDamageSmokeLocation2);
        RULE_BOOL(Radar);
        RULE_BOOL(SpySat);
        SpeedType =
            ini->ReadBool(ID, "WaterBound", SpeedType == ::SpeedType::Float) ? ::SpeedType::Float : ::SpeedType::None;
        RULE_INT(Adjacent);
        RULE_BOOL(Capturable);
        RULE_BOOL(Powered);
        RULE_BOOL(PoweredSpecial);
        RULE_BOOL(Overpowerable);
        RULE_BOOL(Spyable);
        RULE_BOOL(CanC4);
        RULE_BOOL(WantsExtraSpace);
        RULE_BOOL(Bib);
        RULE_BOOL(Unsellable);
        RULE_BOOL(ClickRepairable);
        RULE_BOOL(CanBeOccupied);
        RULE_BOOL(CanOccupyFire);
        RULE_BOOL(ShowOccupantPips);
        RULE_INT(MaxNumberOccupants);
        RULE_INT(NumberImpassableRows);
        RULE_INT(ProduceCashStartup);
        RULE_INT(ProduceCashAmount);
        RULE_INT(ProduceCashDelay);
        RULE_INT(InfantryGainSelfHeal);
        RULE_INT(UnitsGainSelfHeal);
        RULE_INT(RefinerySmokeFrames);
        RULE_BOOL(BaseNormal);
        RULE_BOOL(EligibileForAllyBuilding);
        RULE_BOOL(EligibleForDelayKill);
        RULE_BOOL(NeedsEngineer);
        CaptureEvaEvent = ini->ReadVoxName(ID, "CaptureEvaEvent", CaptureEvaEvent);
        RULE_BOOL(ProtectWithWall);

        // The original overruns MuzzleFlash for a larger positive occupant count.
        if (MaxNumberOccupants > int(std::size(MuzzleFlash))) return false;
        Point2D missing{0xFFFF, 0xFFFF};
        for (int i = 0; i < MaxNumberOccupants; ++i) {
            char key[32];
            std::snprintf(key, sizeof(key), "MuzzleFlash%d", i);
            art.ReadPoint2D(MuzzleFlash[i], ImageFile, key, missing);
            if (MuzzleFlash[i] == missing) MuzzleFlash[i] = {0, 0};
        }
        for (int i = 0; i < int(std::size(DamageFireOffset)); ++i) {
            char key[32];
            std::snprintf(key, sizeof(key), "DamageFireOffset%d", i);
            art.ReadPoint2D(DamageFireOffset[i], ImageFile, key, missing);
            if (DamageFireOffset[i] == missing) {
                DamageFireOffset[i] = {0, 0};
                break;
            }
        }

        RULE_BOOL(Wall);
        RULE_BOOL(Weeder);
        RULE_BOOL(Helipad);
        RULE_BOOL(OrePurifier);
        RULE_BOOL(FactoryPlant);
        Factory = static_cast<AbstractType>(ini->ReadFactory(ID, "Factory", static_cast<int>(Factory)));
        read_rule_type(*ini, ID, "FreeUnit", AbstractType::UnitType, FreeUnit);
        RULE_BOOL(HoverPad);
        RULE_BOOL(IsTemple);
        RULE_BOOL(IsPlug);
        read_rule_type(*ini, ID, "SecretInfantry", AbstractType::InfantryType, SecretInfantry);
        read_rule_type(*ini, ID, "SecretUnit", AbstractType::UnitType, SecretUnit);
        read_rule_type(*ini, ID, "SecretBuilding", AbstractType::BuildingType, SecretBuilding);
        RULE_FLOAT(InfantryCostBonus);
        RULE_FLOAT(UnitsCostBonus);
        RULE_FLOAT(AircraftCostBonus);
        RULE_FLOAT(BuildingsCostBonus);
        RULE_FLOAT(DefensesCostBonus);
        RULE_BOOL(TogglePower);
        read_rule_sound(*ini, ID, "BuildupSound", BuildupSound);
        read_rule_sound(*ini, ID, "PackupSound", PackupSound);
        read_rule_sound(*ini, ID, "CreateUnitSound", CreateUnitSound);
        read_rule_sound(*ini, ID, "UnitExitSound", UnitExitSound);
        read_rule_sound(*ini, ID, "UnitEnterSound", UnitEnterSound);
        read_rule_sound(*ini, ID, "WorkingSound", WorkingSound);
        read_rule_sound(*ini, ID, "NotWorkingSound", NotWorkingSound);
        RULE_BOOL(UnitRepair);
        RULE_BOOL(UnitReload);
        RULE_BOOL(Bunker);
        RULE_BOOL(Cloning);
        RULE_BOOL(Grinding);
        RULE_BOOL(UnitAbsorb);
        RULE_BOOL(InfantryAbsorb);
        RULE_BOOL(SecretLab);
        RULE_BOOL(DockUnload);
        RULE_BOOL(Gate);
        RULE_BOOL(SAM);
        RULE_BOOL(ConstructionYard);
        RULE_BOOL(NukeSilo);
        RULE_BOOL(Refinery);
        RULE_BOOL(WeaponsFactory);
        RULE_BOOL(LaserFencePost);
        RULE_BOOL(LaserFence);
        RULE_BOOL(FirestormWall);
        RULE_BOOL(Hospital);
        RULE_BOOL(Armory);
        RULE_BOOL(GDIBarracks);
        RULE_BOOL(NODBarracks);
        RULE_BOOL(YuriBarracks);
        RULE_BOOL(EMPulseCannon);
        RULE_BOOL(TickTank);
        RULE_FLOAT(ChargedAnimTime);
        SuperWeapon = ini->ReadSWType(ID, "SuperWeapon", SuperWeapon);
        SuperWeapon2 = ini->ReadSWType(ID, "SuperWeapon2", SuperWeapon2);
        RULE_BOOL(CloakGenerator);
        RULE_BOOL(SensorArray);
        // 0x00460C17 sign-extends the stored byte for the default, then stores AL.
        CloakRadiusInCells =
            BYTE(ini->ReadInteger(ID, "CloakRadiusInCells", int(std::bit_cast<signed char>(CloakRadiusInCells))));
        RULE_INT(PsychicDetectionRadius);
        BarrelStartPitch =
            std::bit_cast<int>(DWORD(ini->ReadInteger(ID, "BarrelStartPitch", BarrelStartPitch >> 5)) << 5);
        DeployFacing = std::bit_cast<int>(DWORD(ini->ReadInteger(ID, "DeployFacing", DeployFacing >> 5)) << 5);
        RULE_INT(LightVisibility);
        LightIntensity = read_building_light(*ini, ID, "LightIntensity", LightIntensity);
        LightRedTint = read_building_light(*ini, ID, "LightRedTint", LightRedTint);
        LightGreenTint = read_building_light(*ini, ID, "LightGreenTint", LightGreenTint);
        LightBlueTint = read_building_light(*ini, ID, "LightBlueTint", LightBlueTint);
        GateCloseDelay = ini->ReadDouble(ID, "GateCloseDelay", GateCloseDelay);
        RULE_BOOL(InvisibleInGame);
        if (InvisibleInGame) {
            Invisible = true;
            RadarVisible = false;
        }
        read_nonempty_name(*ini, ID, "PowersUpBuilding", PowersUpBuilding);
        RULE_INT(PowersUpToLevel);
        RULE_BOOL(BridgeRepairHut);
        RULE_BOOL(HasStupidGuardMode);
        RULE_BOOL(CrateBeneath);
        RULE_BOOL(CrateBeneathIsMoney);
        RULE_BOOL(LeaveRubble);
        RULE_BOOL(PlaceAnywhere);
        RULE_BOOL(ICBMLauncher);
        RULE_BOOL(Artillary);
        RULE_POINT(TargetCoordOffset);
        RULE_POINT(ExitCoord);
        RULE_BOOL(AIBuildThis);
        RULE_BOOL(IsBaseDefense);
        RULE_BOOL(IsThreatRatingNode);
        RULE_BOOL(ConcentricRadialIndicator);
        // 0x0046104C derives the inherited rotation count after the building flags.
        RotCount = Turret ? 32 : 1;
        const auto negate = [](int value) { return std::bit_cast<int>(0u - DWORD(value)); };
        const int power = ini->ReadInteger(ID, "Power", PowerBonus > 0 ? PowerBonus : negate(PowerDrain));
        PowerBonus = power < 0 ? 0 : power;
        PowerDrain = power < 0 ? negate(power) : 0;
        ExtraPowerBonus =
            ini->ReadInteger(ID, "ExtraPower", ExtraPowerBonus > 0 ? ExtraPowerBonus : negate(ExtraPowerDrain));
        if (ExtraPowerBonus < 0) {
            ExtraPowerDrain = negate(ExtraPowerBonus);
            ExtraPowerBonus = 0;
        }

#define ART_INT(f) f = art.ReadInteger(ImageFile, #f, f)
#define ART_BOOL(f) f = art.ReadBool(ImageFile, #f, f)
#define ART_POINT(f) art.ReadPoint2D(f, ImageFile, #f, f)
        ART_INT(Height);
        OccupyHeight = art.ReadInteger(ImageFile, "OccupyHeight", Height);
        ART_BOOL(Recoilless);
        ART_BOOL(DoubleThick);
        ART_BOOL(Flat);
        ART_BOOL(SiloDamage);
        ART_BOOL(ChargeAnim);
        ART_BOOL(IsAnimDelayedFire);
        ART_INT(DelayedFireDelay);
        read_rule_type(art, ImageFile, "ToOverlay", AbstractType::OverlayType, ToOverlay);
        Foundation =
            static_cast<::Foundation>(art.ReadFoundation(ImageFile, "Foundation", static_cast<int>(Foundation)));
        const int idFoundation = art.ReadFoundation(ID, "Foundation", static_cast<int>(Foundation));
        if (idFoundation) Foundation = static_cast<::Foundation>(idFoundation);
        ART_INT(MidPoint);
        ART_INT(DoorStages);
        ART_BOOL(DamagedDoor);
        ART_BOOL(TerrainPalette);
        ART_INT(GateStages);
        ART_POINT(PrimaryFirePixelOffset);
        ART_POINT(SecondaryFirePixelOffset);
        ART_BOOL(PrimaryFireDualOffset);
        ART_BOOL(ExtraDamageStage);
        ART_INT(SpecialZOverlayZAdjust);
        ART_INT(NormalZAdjust);
        ART_POINT(ZShapePointMove);
        ExtraLight = WORD(art.ReadInteger(ImageFile, "ExtraLight", std::bit_cast<short>(ExtraLight)));
        CanHideThings = art.ReadBool(ID, "CanHideThings", CanHideThings);
        for (int i = 0; i < 8; ++i) {
            char key[32];
            std::snprintf(key, sizeof(key), "AddOccupy%d", i + 1);
            // Original +0x1624 receives AddOccupy; the retained YRpp member is RemoveOccupy.
            art.ReadPoint2D(RemoveOccupy[i], ImageFile, key, missing);
            std::snprintf(key, sizeof(key), "RemoveOccupy%d", i + 1);
            art.ReadPoint2D(AddOccupy[i], ImageFile, key, missing);
        }
        Point2D zero{0, 0};
        art.ReadPoint2D(QueueingCell, ImageFile, "QueueingCell", zero);
        FoundationData = game::native_building_foundation(static_cast<int>(Foundation));
        if (!FoundationData) return false;
        static CellStruct exits[22][30] = {
            {{0, 1}, {-1, 1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}, {0x7FFF, 0x7FFF}},
            {{0, 1}, {1, 1}, {-1, 1}, {2, 1}, {-1, 0}, {2, 0}, {0, -1}, {1, -1}, {-1, -1}, {2, -1}, {0x7FFF, 0x7FFF}},
            {{0, 2}, {1, 2}, {-1, 2}, {-1, 1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}, {0x7FFF, 0x7FFF}},
            {{0, 2},
             {1, 2},
             {-1, 2},
             {2, 2},
             {-1, 1},
             {2, 1},
             {-1, 0},
             {2, 0},
             {0, -1},
             {1, -1},
             {-1, -1},
             {2, -1},
             {0x7FFF, 0x7FFF}},
            {{0, 3},
             {1, 3},
             {-1, 3},
             {2, 3},
             {-1, 2},
             {2, 2},
             {-1, 1},
             {2, 1},
             {-1, 0},
             {2, 0},
             {0, -1},
             {1, -1},
             {-1, -1},
             {2, -1},
             {0x7FFF, 0x7FFF}},
            {{0, 2},
             {1, 2},
             {2, 2},
             {-1, 2},
             {3, 2},
             {-1, 1},
             {3, 1},
             {-1, 0},
             {3, 0},
             {0, -1},
             {1, -1},
             {2, -1},
             {-1, -1},
             {3, -1},
             {0x7FFF, 0x7FFF}},
            {{0, 3},
             {1, 3},
             {2, 3},
             {-1, 3},
             {3, 3},
             {-1, 2},
             {3, 2},
             {-1, 1},
             {3, 1},
             {-1, 0},
             {3, 0},
             {0, -1},
             {1, -1},
             {2, -1},
             {-1, -1},
             {3, -1},
             {0x7FFF, 0x7FFF}},
            {{-1, -1}, {0, -1}, {1, -1}, {2, -1}, {3, -1}, {-1, 0}, {3, 0}, {-1, 1}, {3, 1}, {-1, 2},         {3, 2},
             {-1, 3},  {3, 3},  {-1, 4}, {3, 4},  {-1, 5}, {0, 5},  {1, 5}, {2, 5},  {3, 5}, {0x7FFF, 0x7FFF}},
            {{0, 2},
             {1, 2},
             {2, 2},
             {3, 2},
             {-1, 2},
             {4, 2},
             {-1, 1},
             {4, 1},
             {-1, 0},
             {4, 0},
             {0, -1},
             {1, -1},
             {2, -1},
             {3, -1},
             {-1, -1},
             {4, -1},
             {0x7FFF, 0x7FFF}},
            {{0, 0}, {0x7FFF, 0x7FFF}},
            {{-1, -1},
             {0, -1},
             {1, -1},
             {-1, 0},
             {1, 0},
             {-1, 1},
             {1, 1},
             {-1, 2},
             {1, 2},
             {-1, 3},
             {0, 3},
             {1, 3},
             {0x7FFF, 0x7FFF}},
            {{-1, -1},
             {0, -1},
             {1, -1},
             {2, -1},
             {3, -1},
             {-1, 0},
             {3, 0},
             {-1, 1},
             {0, 1},
             {1, 1},
             {2, 1},
             {3, 1},
             {0x7FFF, 0x7FFF}},
            {{0, 3},
             {1, 3},
             {2, 3},
             {-1, 3},
             {3, 3},
             {-1, 2},
             {3, 2},
             {-1, 1},
             {3, 1},
             {-1, 0},
             {3, 0},
             {0, -1},
             {1, -1},
             {2, -1},
             {-1, -1},
             {3, -1},
             {0x7FFF, 0x7FFF}},
            {{-1, -1},
             {0, -1},
             {1, -1},
             {-1, 0},
             {1, 0},
             {-1, 1},
             {1, 1},
             {-1, 2},
             {1, 2},
             {-1, 3},
             {1, 3},
             {-1, 4},
             {0, 4},
             {1, 4},
             {0x7FFF, 0x7FFF}},
            {{-1, -1},
             {0, -1},
             {1, -1},
             {-1, 0},
             {1, 0},
             {-1, 1},
             {1, 1},
             {-1, 2},
             {1, 2},
             {-1, 3},
             {1, 3},
             {-1, 4},
             {1, 4},
             {-1, 5},
             {0, 5},
             {1, 5},
             {0x7FFF, 0x7FFF}},
            {{-1, -1},
             {0, -1},
             {1, -1},
             {2, -1},
             {-1, 0},
             {2, 0},
             {-1, 1},
             {2, 1},
             {-1, 2},
             {2, 2},
             {-1, 3},
             {2, 3},
             {-1, 5},
             {2, 5},
             {-1, 6},
             {0, 6},
             {1, 6},
             {2, 6},
             {0x7FFF, 0x7FFF}},
            {{-1, -1},
             {0, -1},
             {1, -1},
             {2, -1},
             {-1, 0},
             {2, 0},
             {-1, 1},
             {2, 1},
             {-1, 2},
             {2, 2},
             {-1, 3},
             {2, 3},
             {-1, 5},
             {0, 5},
             {1, 5},
             {2, 5},
             {0x7FFF, 0x7FFF}},
            {{-1, -1}, {0, -1}, {1, -1}, {2, -1}, {3, -1}, {4, -1}, {5, -1}, {-1, 0}, {5, 0}, {-1, 1},         {5, 1},
             {-1, 2},  {5, 2},  {-1, 3}, {0, 3},  {1, 3},  {2, 3},  {3, 3},  {4, 3},  {5, 3}, {0x7FFF, 0x7FFF}},
            {{-1, -1}, {0, -1}, {1, -1}, {2, -1}, {3, -1}, {4, -1}, {-1, 4},         {0, 4},
             {1, 4},   {2, 4},  {3, 4},  {4, 4},  {-1, 0}, {-1, 1}, {-1, 2},         {-1, 3},
             {-1, 4},  {4, 0},  {4, 1},  {4, 2},  {4, 3},  {4, 4},  {0x7FFF, 0x7FFF}},
            {{0, 4},
             {1, 4},
             {2, 4},
             {-1, 4},
             {3, 4},
             {-1, 2},
             {3, 2},
             {-1, 1},
             {3, 1},
             {-1, 0},
             {3, 0},
             {0, -1},
             {1, -1},
             {2, -1},
             {-1, -1},
             {3, -1},
             {0x7FFF, 0x7FFF}},
            {{2, -1}, {0x7FFF, 0x7FFF}},
            {{0x7FFF, 0x7FFF}}};
        const int foundation = static_cast<int>(Foundation);
        FoundationOutside = foundation >= 0 && foundation < 22 ? exits[foundation] : nullptr;
        read_nonempty_name(art, ImageFile, "Buildup", BuildupFile);
        LoadArt(game::type_theater());
        if (game::last_type_resource_status() != game::TypeResourceStatus::complete) return false;
        read_animation_frame(art, ImageFile, "AnimIdle", BuildingAnimFrame[1]);
        read_animation_frame(art, ImageFile, "AnimActive", BuildingAnimFrame[2]);
        read_animation_frame(art, ImageFile, "AnimAux1", BuildingAnimFrame[4]);
        read_animation_frame(art, ImageFile, "AnimAux2", BuildingAnimFrame[5]);

        // 0x004617B8..0x0046445F: slot order affects INI and registry observations.
        struct SlotReader {
            const char* key;
            int index;
            bool powered;
        };
        constexpr SlotReader slots[] = {
            {"ActiveAnim", 3, true},      {"ActiveAnimTwo", 4, true},     {"ActiveAnimThree", 5, true},
            {"ActiveAnimFour", 6, true},  {"SuperAnim", 14, true},        {"SuperAnimTwo", 15, true},
            {"SuperAnimThree", 16, true}, {"SuperAnimFour", 17, true},    {"SpecialAnim", 10, true},
            {"SpecialAnimTwo", 11, true}, {"SpecialAnimThree", 12, true}, {"SpecialAnimFour", 13, true},
            {"LowPower", 19, true},       {"SuperLowPower", 20, true},    {"ProductionAnim", 8, false},
            {"IdleAnim", 18, true},       {"PreProductionAnim", 7, false}};
        for (const auto& slot : slots)
            read_animation(art, ImageFile, slot.key, BuildingAnim[slot.index], slot.powered);

        // The garrisoned turret name comes from rules[ImageFile], unlike its other fields.
        auto& turret = BuildingAnim[9];
        read_nonempty_name(*ini, ID, "TurretAnim", turret.Anim);
        read_nonempty_name(*ini, ID, "TurretAnimDamaged", turret.Damaged);
        if (!*turret.Damaged) std::memcpy(turret.Damaged, turret.Anim, std::strlen(turret.Anim) + 1);
        read_nonempty_name(*ini, ImageFile, "TurretAnimGarrisoned", turret.Garrisoned);
        if (!*turret.Garrisoned) std::memcpy(turret.Garrisoned, turret.Anim, std::strlen(turret.Anim) + 1);
        turret.Position.X = ini->ReadInteger(ID, "TurretAnimX", turret.Position.X);
        turret.Position.Y = ini->ReadInteger(ID, "TurretAnimY", turret.Position.Y);
        turret.ZAdjust = ini->ReadInteger(ID, "TurretAnimZAdjust", turret.ZAdjust);
        turret.YSort = ini->ReadInteger(ID, "TurretAnimYSort", turret.YSort);
        RULE_BOOL(TurretAnimIsVoxel);
        RULE_BOOL(BarrelAnimIsVoxel);
        // The original reader only writes 0x10 bytes of this 0x14-byte member.
        ini->ReadString(ID, "VoxelBarrelFile", VoxelBarrelFile, VoxelBarrelFile, 0x10);
        RULE_POINT(VoxelBarrelOffsetToPitchPivotPoint);
        RULE_POINT(VoxelBarrelOffsetToRotatePivotPoint);
        RULE_POINT(VoxelBarrelOffsetToBuildingPivotPoint);
        RULE_POINT(VoxelBarrelOffsetToBarrelEnd);
        RULE_INT(Upgrades);
        if (Upgrades > 3) return false;  // Reject the original fixed-slot overrun, retaining negative values.
        for (int i = 0; i < Upgrades; ++i) {
            auto& slot = BuildingAnim[i];
            char key[64];
            std::snprintf(key, sizeof(key), "PowerUp%01dAnim", i + 1);
            read_nonempty_name(art, ImageFile, key, slot.Anim);
            std::snprintf(key, sizeof(key), "PowerUp%01dDamagedAnim", i + 1);
            read_nonempty_name(art, ImageFile, key, slot.Damaged);
#define POWERUP_INT(suffix, member)                                                                                    \
    std::snprintf(key, sizeof(key), "PowerUp%01d%s", i + 1, suffix);                                                   \
    slot.member = art.ReadInteger(ImageFile, key, slot.member)
            POWERUP_INT("LocXX", Position.X);
            POWERUP_INT("LocYY", Position.Y);
            POWERUP_INT("LocZZ", ZAdjust);
            POWERUP_INT("YSort", YSort);
#undef POWERUP_INT
        }
        const int oldDocks = NumberOfDocks;
        RULE_INT(NumberOfDocks);
        if (NumberOfDocks > DockingOffsets.Capacity) {
            if (oldDocks < 0 || !DockingOffsets.SetCapacity(NumberOfDocks)) return false;
            for (int i = oldDocks; i < NumberOfDocks; ++i)
                DockingOffsets[i] = {0, 0, 0};
        }
        for (int i = 0; i < NumberOfDocks; ++i) {
            char key[40];
            std::snprintf(key, sizeof(key), "DockingOffset%d", i);
            art.ReadPoint3D(DockingOffsets[i], ImageFile, key, DockingOffsets[i]);
        }
#undef RULE_INT
#undef RULE_BOOL
#undef RULE_POINT
#undef RULE_FLOAT
#undef ART_INT
#undef ART_BOOL
#undef ART_POINT
        return true;
    } catch (...) {
        game::type_resource_failure(ID);
        return false;
    }
}
