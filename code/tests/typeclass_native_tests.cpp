#include "support/test_support.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {

int expirations = 0;
void expired(AbstractTypeClass*, bool removed) noexcept { if (removed) ++expirations; }
TEST(TypeclassNative, Contracts) {
    const int count = AbstractTypeClass::Array.Count;
    AbstractTypeClass::PointerExpirationObserver = expired;
    {
        ScriptTypeClass script("script"); SideClass side("allied"); TaskForceClass task("force");
        OverlayTypeClass overlay("wall"); SmudgeTypeClass smudge("crater"); TerrainTypeClass terrain("tree");
        EXPECT_TRUE((AbstractTypeClass::Array.Count == count + 6)) << "actual base registration";
        EXPECT_TRUE((script.ActionsCount == 0 && !script.IsGlobal && script.ArrayIndex >= 0)) << "script defaults";
        EXPECT_TRUE((ScriptTypeClass::Find("ScRiPt") == &script && ScriptTypeClass::Find(nullptr) == nullptr)) << "script lookup";
        EXPECT_TRUE((ScriptTypeClass::FindOrAllocate("SCRIPT") == &script)) << "no duplicate factory allocation";
        EXPECT_TRUE((!ScriptTypeClass::FindOrAllocate("none") && !ScriptTypeClass::FindOrAllocate("<none>"))) << "sentinels";
        EXPECT_TRUE((side.HouseTypes.Count == 0 && side.UniqueID == 0)) << "side registry and absent Scenario ID";
        EXPECT_TRUE((task.Group == -1 && task.CountEntries == 0 && !task.IsGlobal)) << "task-force defaults";
        for (const auto& entry : task.Entries) EXPECT_TRUE((!entry.Type && entry.Amount == 0)) << "six clear task entries";
        EXPECT_TRUE((overlay.Strength == 1 && overlay.NoUseTileLandType && overlay.DrawFlat && !overlay.ImageLoaded)) << "overlay target defaults";
        EXPECT_TRUE((smudge.Width == 1 && smudge.Height == 1 && smudge.Immune && !smudge.LegalTarget)) << "smudge defaults";
        EXPECT_TRUE((terrain.Strength == -1 && static_cast<int>(terrain.Armor) == 6 && terrain.IsLogic &&
            terrain.SnowOccupationBits == 7)) << "terrain inherited overrides";
        EXPECT_TRUE((script.AddRef() == 1 && script.Release() == 1 && script.RefCount == 0 && script.IsDead())) << "original base identity semantics, not COM reference counting";
        CoordStruct coordinates; EXPECT_TRUE((script.GetCoords(&coordinates) == &coordinates &&
            coordinates == CoordStruct::Empty)) << "base coordinates";
        CLSID clsid{}; EXPECT_TRUE((script.GetClassID(nullptr) < 0 && script.GetClassID(&clsid) == 0)) << "local COM identity status";
        EXPECT_TRUE((script.WhatAmI() == AbstractType::ScriptType && script.Size() == sizeof(script))) << "local virtual dispatch";
        CCINIClass ini;
        ini.WriteString("script", "Name", "Native script");
        ini.WriteString("script", "0", "1,2"); ini.WriteString("script", "7", "18,3");
        EXPECT_TRUE((script.LoadFromINI(&ini) && script.ActionsCount == 2 && script.ScriptActions[1].Action == 18)) << "sparse script keys compact in numeric order";
        EXPECT_TRUE((script.ScriptActions[1].Argument == 3 && !std::strcmp(script.Name, "Native script"))) << "script contents";
        EXPECT_TRUE((script.SaveToINI(&ini))) << "script save";
        char text[64]; ini.ReadString("script", "1", "", text, sizeof(text));
        EXPECT_TRUE((!std::strcmp(text, "18,3"))) << "saved action encoding";
        ini.ReadString("script", "7", "absent", text, sizeof(text));
        EXPECT_TRUE((!std::strcmp(text, "absent"))) << "save removes stale slots";
        CRCEngine crc; script.ComputeCRC(crc); side.ComputeCRC(crc); task.ComputeCRC(crc);
        overlay.ComputeCRC(crc); smudge.ComputeCRC(crc); terrain.ComputeCRC(crc);
        const auto& runtime = game::native_rules_runtime();
        int n = -1; AbstractTypeClass* resolved = nullptr;
        EXPECT_TRUE((runtime.type_count(nullptr, AbstractType::ScriptType, n) && n == ScriptTypeClass::Array.Count)) << "native runtime sees the real registry";
        EXPECT_TRUE((runtime.resolve(nullptr, AbstractType::ScriptType, "script", resolved) && resolved == &script)) << "runtime uses native class lookup";
        resolved = &script;
        EXPECT_TRUE((!runtime.resolve(nullptr, AbstractType::BuildingType, "NO_NATIVE_DEFAULTS", resolved) && resolved == &script)) << "unsupported factory neither invents a type nor enters EXE";
        EXPECT_TRUE((!runtime.type_at(nullptr, AbstractType::ScriptType, -1, resolved))) << "invalid registry index";
    }
    AbstractTypeClass::PointerExpirationObserver = nullptr;
    EXPECT_TRUE((expirations == 6 && AbstractTypeClass::Array.Count == count)) << "destruction and notifications";
    EXPECT_TRUE((ScriptTypeClass::Array.Count == 0 && SideClass::Array.Count == 0 && TaskForceClass::Array.Count == 0 &&
        OverlayTypeClass::Array.Count == 0 && SmudgeTypeClass::Array.Count == 0 && TerrainTypeClass::Array.Count == 0)) << "no stale derived registrations";
    auto* allocated = ScriptTypeClass::FindOrAllocate("allocated");
    EXPECT_TRUE((allocated != nullptr)) << "real factory allocation"; GameDelete(allocated);
    EXPECT_TRUE((AbstractTypeClass::Array.Count == count)) << "factory deletion";
}
}

