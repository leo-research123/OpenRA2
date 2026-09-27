#include "support/test_support.hpp"
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Powerups.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/Unsorted.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(std::is_same_v<decltype(RulesClass::AISuperDefenseDistance), int>);
static_assert(std::is_same_v<decltype(RulesClass::TalkBubbleTime), DWORD>);
static_assert(std::is_same_v<decltype(&RulesClass::Read_Difficulty),
    bool (YRPP_FASTCALL *)(CCINIClass*, DifficultyStruct*, const char*)>);
static_assert(!std::is_copy_constructible_v<RulesClass>);

namespace {


void lifecycle() {
    EXPECT_TRUE((RulesClass::Instance == nullptr)) << "host singleton starts empty";
    RulesClass owner;
    RulesClass::Instance = &owner;
    for (int i = 0; i < 32; ++i) {
        RulesClass rules;
        EXPECT_TRUE((RulesClass::Instance == &owner)) << "constructor does not publish its object";
        EXPECT_TRUE((rules.DetailMinFrameRateNormal == 15 && rules.DetailMinFrameRateMovie == 20)) << "original detail defaults";
        EXPECT_TRUE((rules.TunnelSpeed == 1.0 && rules.TiberiumHeal == 1.0 / 60.0)) << "original double defaults";
        EXPECT_TRUE((rules.AISuperDefenseDistance == 10 && rules.TalkBubbleTime == 300)) << "distance and talk duration use integer storage";
        EXPECT_TRUE((rules.V3Rocket.TiltFrames == 60 && rules.V3Rocket.BodyLength == 256 &&
            rules.V3Rocket.LazyCurve && !rules.DMisl.LazyCurve && rules.CMisl.Damage == 500)) << "three independent rocket default sets";
        EXPECT_TRUE((rules.BarrelDebris.Items == nullptr && rules.BarrelDebris.Count == 0 &&
            rules.BarrelDebris.Capacity == 0 && rules.BarrelDebris.CapacityIncrement == 10 &&
            rules.BarrelDebris.IsInitialized && !rules.BarrelDebris.IsAllocated)) << "empty original container state";
        EXPECT_TRUE((rules.BaseUnit.AddItem(nullptr) && rules.DeadBodies.AddItem(nullptr) &&
            rules.FillEarliestTeamProbability.AddItem(7))) << "owned member storage is usable";
        int borrowed[] = {11, 13};
        rules.CreditTicks = TypeList<int>(2, borrowed);
        rules.CreditTicks.Count = 2;
        EXPECT_TRUE((!rules.CreditTicks.IsAllocated && rules.CreditTicks[1] == 13)) << "borrowed list retains external ownership";
        for (const auto& color : rules.ColorAdd)
            EXPECT_TRUE((color.R == 0 && color.G == 0 && color.B == 0)) << "all color-add entries initialized";
    }
    EXPECT_TRUE((RulesClass::Instance == &owner)) << "destruction does not clear somebody else's singleton";
    RulesClass::Instance = nullptr;
}

void overlays() {
    RulesClass rules;
    CCINIClass base, patch, absent;
    EXPECT_TRUE((!rules.Read_IQ(&absent) && rules.Read_Maximums(&absent) &&
        rules.Read_Difficulties(&absent))) << "individual readers retain different absent-section returns";
    base.WriteInteger("IQ", "MaxIQLevels", 11);
    base.WriteInteger("IQ", "SuperWeapons", 7);
    base.WriteInteger("IQ", "Production", 9);
    base.WriteDouble("WallModel", "WallPenetratorThreshold", 0.75);
    base.WriteBool("WallModel", "AlliedWallTransparency", true);
    base.WriteInteger("ElevationModel", "ElevationIncrement", 5);
    base.WriteDouble("ElevationModel", "ElevationIncrementBonus", 1.25);
    base.WriteDouble("ElevationModel", "ElevationBonusCap", 0.8);
    base.WriteInteger("JumpjetControls", "Speed", 19);
    base.WriteInteger("JumpjetControls", "TurnRate", 6);
    base.WriteDouble("JumpjetControls", "Climb", 2.5);
    base.WriteInteger("JumpjetControls", "CruiseHeight", 900);
    EXPECT_TRUE((rules.Read_IQ(&base) && rules.Read_WallModel(&base) &&
        rules.Read_ElevationModel(&base) && rules.Read_JumpjetControls(&base))) << "read base sections";
    patch.WriteInteger("IQ", "SuperWeapons", 3);
    patch.WriteString("IQ", "Production", "invalid");
    patch.WriteString("WallModel", "WallPenetratorThreshold", "50%");
    patch.WriteInteger("JumpjetControls", "Speed", 27);
    EXPECT_TRUE((rules.Read_IQ(&patch) && rules.Read_WallModel(&patch) &&
        rules.Read_JumpjetControls(&patch))) << "read partial overlays";
    EXPECT_TRUE((rules.MaxIQLevels == 11 && rules.SuperWeapons == 3 && rules.Production == 0)) << "missing keys inherit; invalid decimal integers follow original atoi zero";
    EXPECT_TRUE((rules.WallPenetratorThreshold == 0.5 && rules.AlliedWallTransparency)) << "INI percent conversion and missing bool inheritance";
    EXPECT_TRUE((rules.ElevationIncrement == 5 && rules.ElevationIncrementBonus == 1.25 &&
        rules.ElevationBonusCap == static_cast<double>(0.8f))) << "elevation retains INI float-to-double parsing";
    EXPECT_TRUE((rules.Speed == 27 && rules.TurnRate == 6 && rules.Climb == 2.5 && rules.CruiseHeight == 900)) << "jumpjet overlay retains unspecified fields";
    EXPECT_TRUE((!rules.Read_ElevationModel(&absent) && rules.ElevationIncrement == 5)) << "absent section makes no changes";

    base.WriteInteger("MultiplayerDialogSettings", "Money", 12000);
    base.WriteInteger("MultiplayerDialogSettings", "AIDifficulty", 2);
    base.WriteBool("MultiplayerDialogSettings", "AllyChangeAllowed", false);
    base.WriteBool("MultiplayerDialogSettings", "SuperWeaponsAllowed", false);
    base.WriteBool("MultiplayerDialogSettings", "FogOfWar", true);
    base.WriteInteger("Maximums", "Players", 12);
    EXPECT_TRUE((rules.Read_MultiplayerDialogSettings(&base) && rules.Read_Maximums(&base))) << "multiplayer read";
    EXPECT_TRUE((rules.Money == 12000 && rules.AIDifficultyStruct == 2 && rules.Players == 12 &&
        !rules.AllyChangeAllowed && !rules.SuperWeaponsAllowed && rules.FogOfWar)) << "multiplayer keys map to original fields";
}

void difficulty() {
    RulesClass rules;
    CCINIClass base, patch, absent;
    rules.Easy.Firepower = 7;
    EXPECT_TRUE((!RulesClass::Read_Difficulty(&absent, &rules.Easy, "Easy") && rules.Easy.Firepower == 7)) << "absent difficulty leaves output unchanged";
    base.WriteDouble("Easy", "Firepower", 9.0);
    base.WriteDouble("Easy", "FirePower", 1.5);
    base.WriteDouble("Easy", "GroundSpeed", 9.0);
    base.WriteDouble("Easy", "Groundspeed", 1.25);
    base.WriteDouble("Easy", "Airspeed", 1.75);
    base.WriteDouble("Easy", "Armor", 1.4);
    base.WriteBool("Easy", "ContentScan", true);
    EXPECT_TRUE((RulesClass::Read_Difficulty(&base, &rules.Easy, "Easy"))) << "difficulty returns ContentScan true";
    EXPECT_TRUE((rules.Easy.Firepower == 1.5 && rules.Easy.Armor == static_cast<double>(1.4f) &&
        rules.Easy.GroundSpeed == 1.25 && rules.Easy.AirSpeed == 1.75)) << "difficulty preserves case-sensitive original key spelling";
    patch.WriteDouble("Easy", "Armor", 0.9);
    EXPECT_TRUE((!RulesClass::Read_Difficulty(&patch, &rules.Easy, "Easy"))) << "present difficulty can return false after a successful read";
    EXPECT_TRUE((rules.Easy.Firepower == 1.0 && rules.Easy.Armor == static_cast<double>(0.9f) &&
        rules.Easy.BuildTime == 1.0 && rules.Easy.RepairDelay == 0.02 &&
        rules.Easy.BuildDelay == 0.03 && !rules.Easy.BuildSlowdown && rules.Easy.DestroyWalls &&
        !rules.Easy.ContentScan)) << "difficulty overlay resets missing keys to fixed defaults";
    rules.Normal.Firepower = 8;
    rules.Difficult.Firepower = 9;
    patch.WriteDouble("Difficult", "FirePower", 0.8);
    EXPECT_TRUE((rules.Read_Difficulties(&patch) && rules.Normal.Firepower == 8 &&
        rules.Difficult.Firepower == static_cast<double>(0.8f))) << "three difficulty sections are independent";
}

void invalidation() {
    RulesClass rules;
    int first = 1, second = 2;
    auto* removed = reinterpret_cast<AbstractClass*>(&first);
    auto* unit = reinterpret_cast<UnitTypeClass*>(&first);
    auto* other_unit = reinterpret_cast<UnitTypeClass*>(&second);
    rules.LargeVisceroid = unit;
    rules.SmallVisceroid = other_unit;
    rules.BaseUnit.AddItem(unit);
    rules.BaseUnit.AddItem(other_unit);
    rules.BaseUnit.AddItem(unit);
    rules.AmerParaDropNum.AddItem(3);
    rules.CrushWarhead = reinterpret_cast<WarheadTypeClass*>(&first);
    rules.C4Warhead = reinterpret_cast<WarheadTypeClass*>(&second);
    rules.PrerequisiteProcAlternate = unit;
    rules.ThirdPowerPlant = reinterpret_cast<BuildingTypeClass*>(&second);
    rules.PointerGotInvalid(removed, false);
    EXPECT_TRUE((rules.LargeVisceroid == nullptr && rules.SmallVisceroid == other_unit)) << "scalar invalidation matches pointer identity";
    EXPECT_TRUE((rules.BaseUnit.Count == 2 && rules.BaseUnit[0] == other_unit && rules.BaseUnit[1] == unit)) << "invalidation removes only the first occurrence and preserves order";
    EXPECT_TRUE((rules.AmerParaDropNum.Count == 1 && rules.AmerParaDropNum[0] == 3)) << "numeric lists are unaffected";
    EXPECT_TRUE((rules.CrushWarhead == reinterpret_cast<WarheadTypeClass*>(&first) && !rules.C4Warhead &&
        rules.PrerequisiteProcAlternate == unit && !rules.ThirdPowerPlant)) << "preserve the two original cross-member clears";
    rules.PointerGotInvalid(removed, true);
    EXPECT_TRUE((rules.BaseUnit.Count == 1 && rules.BaseUnit[0] == other_unit)) << "removed flag does not change invalidation behavior";
}

void table_readers() {
    RulesClass rules;
    CCINIClass base, patch, absent;
    EXPECT_TRUE((!rules.Read_ColorAdd(&absent) && RulesClass::Read_LandCharacteristics(&absent) &&
        !RulesClass::Read_Powerups(&absent) && !RulesClass::Read_Movies(&absent))) << "table reader absent returns";
    base.WriteString("ColorAdd", "9", "255,128,0");
    base.WriteString("ColorAdd", "2", "1,2,3");
    rules.ColorAdd[2] = ColorStruct(4, 5, 6);
    EXPECT_TRUE((rules.Read_ColorAdd(&base) && rules.ColorAdd[0].R == 255 &&
        rules.ColorAdd[0].G == 128 && rules.ColorAdd[1].B == 3 && rules.ColorAdd[2].B == 6)) << "ColorAdd follows entry order and preserves trailing slots";
    CCINIClass oversized;
    for (int i = 0; i < 17; ++i) {
        const auto key = std::to_string(i);
        oversized.WriteString("ColorAdd", key.c_str(), "7,8,9");
    }
    EXPECT_TRUE((!rules.Read_ColorAdd(&oversized) && rules.ColorAdd[0].R == 255 && rules.ColorAdd[1].B == 3)) << "oversized ColorAdd rejects before mutation";

    for (auto& ground : GroundType::Array) {
        for (auto& cost : ground.Cost) cost = 0.25f;
        ground.Buildable = true;
    }
    base.WriteString("Clear", "Hover", "2.5");
    base.WriteString("Clear", "Foot", "-0.5");
    base.WriteString("Clear", "Track", "75%");
    base.WriteString("Clear", "Winged", "0.1");
    EXPECT_TRUE((RulesClass::Read_LandCharacteristics(&base))) << "ground sections read";
    const auto& clear = GroundType::Array[0];
    EXPECT_TRUE((clear.Cost[3] == 1.0f && clear.Cost[0] == -0.5f && clear.Cost[1] == 0.75f &&
        clear.Cost[4] == 1.0f && clear.Cost[2] == 1.0f && !clear.Buildable)) << "ground caps only the upper limit, forces Winged and uses fixed missing defaults";
    EXPECT_TRUE((GroundType::Array[1].Cost[0] == 0.25f && GroundType::Array[1].Buildable)) << "absent land sections retain existing state";
    patch.WriteBool("Clear", "Buildable", true);
    EXPECT_TRUE((RulesClass::Read_LandCharacteristics(&patch) && clear.Cost[0] == 1.0f && clear.Buildable)) << "present land overlay resets unspecified speeds to one";

    EXPECT_TRUE((Powerups::Weights[0] == 50 && Powerups::Weights[1] == 20 && Powerups::Anims[0] == -1)) << "original powerup table defaults";
    EXPECT_TRUE((AnimTypeClass::FindIndex(nullptr) == -1 && AnimTypeClass::FindIndex("<none>") == -1)) << "animation name lookup guards null and original sentinel";
    Powerups::Naval[1] = true;
    Powerups::Arguments[1] = 42;
    base.WriteString("Powerups", "Money", "8,NONE,YES,25%");
    base.WriteString("Powerups", "Unit", "3");
    EXPECT_TRUE((RulesClass::Read_Powerups(&base) && Powerups::Weights[0] == 8 &&
        Powerups::Anims[0] == -1 && Powerups::Naval[0] && Powerups::Arguments[0] == 0.25)) << "YR powerups parse four tokens with percentage data";
    EXPECT_TRUE((Powerups::Weights[1] == 3 && Powerups::Naval[1] && Powerups::Arguments[1] == 42 &&
        Powerups::Weights[2] == 0)) << "partial tokens preserve later fields; absent keys use 0,NONE";
    patch.WriteString("Powerups", "Money", ",, 5,, NONE,,no, 2.5");
    EXPECT_TRUE((RulesClass::Read_Powerups(&patch) && Powerups::Weights[0] == 5 && !Powerups::Naval[0] &&
        Powerups::Arguments[0] == 2.5 && Powerups::Weights[1] == 0 && Powerups::Arguments[1] == 42)) << "comma-only tokenization skips empty fields and preserves fourth-token inheritance";
    patch.WriteString("Powerups", "Money", "6,NONE,true");
    EXPECT_TRUE((RulesClass::Read_Powerups(&patch) && !Powerups::Naval[0] && Powerups::Arguments[0] == 2.5)) << "powerup water flag accepts yes/no only";
}

void movies() {
    MovieInfo::ClearArray();
    CCINIClass base, patch;
    for (int i = 0; i < 24; ++i) {
        const auto key = std::to_string(i);
        const auto value = "Movie" + key;
        base.WriteString("Movies", key.c_str(), value.c_str());
    }
    EXPECT_TRUE((RulesClass::Read_Movies(&base) && MovieInfo::Array.Count == 24)) << "movie list survives multiple pointer-buffer growths";
    EXPECT_TRUE((MovieInfo::FindIndex("mOVIE23") == 23 && !std::strcmp(MovieInfo::Array[0], "Movie0"))) << "movie names retain ownership and have case-insensitive lookup";
    patch.WriteString("Movies", "0", "MOVIE0");
    patch.WriteString("Movies", "1", "<none>");
    patch.WriteString("Movies", "2", "<none>");
    patch.WriteString("Movies", "3", "123456789012345678901234567890123456789");
    EXPECT_TRUE((RulesClass::Read_Movies(&patch) && MovieInfo::Array.Count == 27)) << "ordinary names deduplicate; <none> retains original append behavior";
    EXPECT_TRUE((MovieInfo::FindIndex("<none>") == -1 && std::strlen(MovieInfo::Array[26]) == 31)) << "movie sentinel and original 32-byte name buffer";
    MovieInfo::ClearArray();
    EXPECT_TRUE((MovieInfo::Array.Count == 0 && MovieInfo::Array.Items == nullptr)) << "explicit movie shutdown frees strings and pointer storage";
    MovieInfo::Array.CapacityIncrement = 0;
    EXPECT_TRUE((!RulesClass::Read_Movies(&base) && MovieInfo::Array.Count == 0)) << "failed movie insertion returns false without leaking its copied name";
    MovieInfo::Array.CapacityIncrement = 10;
    EXPECT_TRUE((RulesClass::Read_Movies(&base))) << "movie list can rebuild after failed insertion";
    MovieInfo::ClearArray();
}
}


TEST(Rules, Contracts) {
    lifecycle(); overlays(); difficulty(); invalidation(); table_readers(); movies();
}
