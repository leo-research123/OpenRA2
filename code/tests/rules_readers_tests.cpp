#include "support/test_support.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/MissionClass.h"
#include <cstring>
#include <cfenv>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Dependencies {
    struct Lookup { AbstractType kind; std::string name; };
    std::vector<Lookup> names;
    bool fail = false;
    int sound = -1;
    game::RulesRuntimeServices services() {
        return {this,
            [](void* context, AbstractType kind, const char* name, AbstractTypeClass*& result) {
                auto& self = *static_cast<Dependencies*>(context);
                self.names.push_back({kind, name});
                if (self.fail) return false;
                // Exercise the factory's valid null result (NONE or allocation
                // failure). Actual type factories are tested by the x86 probe.
                result = nullptr;
                return true;
            },
            [](void* context, const char*, int& result) {
                result = static_cast<Dependencies*>(context)->sound;
                return true;
            },
            [](void*, AbstractType, int& count) { count = 0; return true; },
            nullptr};
    }
};
struct Operation { RulesClass* rules; CCINIClass* ini; bool (RulesClass::*read)(CCINIClass*); bool result; };
void run_reader(void* data) {
    auto& op = *static_cast<Operation*>(data);
    op.result = (op.rules->*op.read)(op.ini);
}
void types() {
    using Reader = bool (YRPP_STDCALL*)(CCINIClass*);
    struct Row { const char* section; AbstractType kind; Reader read; };
    const Row rows[] = {
        {"InfantryTypes", AbstractType::InfantryType, RulesClass::Read_InfantryTypes},
        {"Countries", AbstractType::HouseType, RulesClass::Read_Countries},
        {"VehicleTypes", AbstractType::UnitType, RulesClass::Read_VehicleTypes},
        {"AircraftTypes", AbstractType::AircraftType, RulesClass::Read_AircraftTypes},
        {"SuperWeaponTypes", AbstractType::SuperWeaponType, RulesClass::Read_SuperWeaponTypes},
        {"BuildingTypes", AbstractType::BuildingType, RulesClass::Read_BuildingTypes},
        {"TerrainTypes", AbstractType::TerrainType, RulesClass::Read_TerrainTypes},
        {"SmudgeTypes", AbstractType::SmudgeType, RulesClass::Read_SmudgeTypes},
        {"OverlayTypes", AbstractType::OverlayType, RulesClass::Read_OverlayTypes},
        {"Animations", AbstractType::AnimType, RulesClass::Read_Animations},
        {"VoxelAnims", AbstractType::VoxelAnimType, RulesClass::Read_VoxelAnims},
        {"Warheads", AbstractType::WarheadType, RulesClass::Read_Warheads},
        {"Particles", AbstractType::ParticleType, RulesClass::Read_Particles},
        {"ParticleSystems", AbstractType::ParticleSystemType, RulesClass::Read_ParticleSystems},
    };
    for (const auto& row : rows) {
        CCINIClass ini;
        EXPECT_TRUE((!row.read(nullptr) && !row.read(&ini))) << "missing type section";
        ini.WriteString(row.section, "9", "   ");
        EXPECT_TRUE((row.read(&ini))) << "blank key still counts as a present type list";
        ini.WriteString(row.section, "2", "Repeated");
        ini.WriteString(row.section, "1", "Repeated");
        ini.WriteString(row.section, "Long", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
        Dependencies deps;
        struct Call { Reader read; CCINIClass* ini; } call{row.read, &ini};
        game::with_rules_runtime(deps.services(), [](void* ptr) {
            auto& call = *static_cast<Call*>(ptr);
            EXPECT_TRUE((call.read(call.ini))) << "null factory result does not change key-count return";
        }, &call);
        EXPECT_TRUE((deps.names.size() == 3 && deps.names[0].name == "Repeated" &&
            deps.names[1].name == "Repeated" && deps.names[2].name == "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234" &&
            deps.names[0].kind == row.kind)) << "INI enumeration order, duplicates and 31-byte truncation";
    }
}
void sections() {
    RulesClass rules;
    CCINIClass ini;
    EXPECT_TRUE((!rules.Read_Radiation(&ini) && !rules.Read_CrateRules(&ini) &&
        !rules.Read_SpecialWeapons(&ini) && !rules.Read_AI(&ini))) << "absent sections require no type services";
    ini.WriteString("Radiation", "RadLevelFactor", "125%");
    ini.WriteString("Radiation", "RadColor", "-1,256,511");
    EXPECT_TRUE((rules.Read_Radiation(&ini) && rules.RadLevelFactor == 1.25 &&
        rules.RadColor.R == 255 && rules.RadColor.G == 0 && rules.RadColor.B == 255)) << "radiation percent and RGB byte conversion";
    ini.WriteString("CrateRules", "CrateRadius", "-1");
    rules.CrateRadius = 731;
    EXPECT_TRUE((rules.Read_CrateRules(&ini) && rules.CrateRadius == 731)) << "distance -1 preserves previous value";
    ini.WriteString("CrateRules", "CrateRadius", "-.125");
    EXPECT_TRUE((rules.Read_CrateRules(&ini) && rules.CrateRadius == -32)) << "negative distance is not clamped";
    ini.WriteString("CrateRules", "HealCrateSound", "Unknown");
    rules.HealCrateSound = 37;
    Dependencies deps;
    Operation op{&rules, &ini, &RulesClass::Read_CrateRules, false};
    game::with_rules_runtime(deps.services(), run_reader, &op);
    EXPECT_TRUE((op.result && rules.HealCrateSound == 37)) << "unknown sound retains previous index";
    deps.sound = 0;
    game::with_rules_runtime(deps.services(), run_reader, &op);
    EXPECT_TRUE((rules.HealCrateSound == 0)) << "sound index zero is valid";
    ini.WriteString("SpecialWeapons", "NukeWarhead", "NONE");
    ini.WriteString("SpecialWeapons", "EMPulseProjectile", "Projectile");
    op.read = &RulesClass::Read_SpecialWeapons;
    game::with_rules_runtime(deps.services(), run_reader, &op);
    EXPECT_TRUE((op.result && deps.names.size() == 2 && deps.names[0].kind == AbstractType::WarheadType &&
        deps.names[1].kind == AbstractType::BulletType)) << "special-weapon type dependency routing";

    BuildingTypeClass* borrowed[] = {nullptr, nullptr};
    rules.BuildConst = TypeList<BuildingTypeClass*>(2, borrowed);
    rules.BuildConst.Count = 2;
    ini.WriteString("AI", "AIForcePredictionFudge", "1,,bad,4294967295,-2");
    ini.WriteString("AI", "AttackInterval", "75%");
    EXPECT_TRUE((rules.Read_AI(&ini) && rules.BuildConst.Count == 2 && rules.BuildConst.Items != borrowed &&
        rules.BuildConst.IsAllocated)) << "AI missing type key copies borrowed fallback into owned storage";
    EXPECT_TRUE((rules.AIForcePredictionFudge.Count == 4 && rules.AIForcePredictionFudge[1] == 0 &&
        rules.AIForcePredictionFudge[2] == -1 && rules.AIForcePredictionFudge[3] == -2 &&
        rules.AttackInterval == .75)) << "AI integer list tokenization/overflow and scalar overlay";
    ini.WriteString("AI", "BuildConst", ",,NONE,,<none>,");
    op.read = &RulesClass::Read_AI;
    game::with_rules_runtime(deps.services(), run_reader, &op);
    EXPECT_TRUE((rules.BuildConst.Count == 0)) << "present sentinel-only list clears old content";
    deps.fail = true;
    bool failed = false;
    try { game::with_rules_runtime(deps.services(), run_reader, &op); }
    catch (const std::runtime_error&) { failed = true; }
    EXPECT_TRUE((failed)) << "unavailable factory must not report successful read";
    failed = false;
    try { rules.Read_AI(&ini); }
    catch (const std::runtime_error&) { failed = true; }
    // Native defaults exist now, but BuildingType resolution is unavailable.
    EXPECT_TRUE((failed)) << "throwing operation restores the native partial runtime scope";
}
void numeric_overlays() {
    RulesClass rules;
    // These original fields are intentionally not initialized by the ctor.
    rules.AICaptureLowMoneyMark = 0;
    rules.AICaptureWoundedMark = 0;
    rules.TiberiumTransmogrify = false;
    rules.GuardModeStray = 0;
    rules.TalkBubbleTime = 0xfffffffeu;
    rules.DMisl.Acceleration = .375;
    rules.CMisl.Acceleration = .75;
    rules.PrismSupportModifier = 3;
    CCINIClass ini;
    ini.WriteString("General", "Unrelated", "1");
    EXPECT_TRUE((rules.Read_General(&ini))) << "General partial overlay";
    // Values independently observed at 671E81 in the fixed original EXE.
    EXPECT_TRUE((rules.TalkBubbleTime == 221)) << ("unsigned time and x87 intermediate precision: " +
        std::to_string(rules.TalkBubbleTime)).c_str();
    EXPECT_TRUE((std::fegetround() == FE_TOWARDZERO)) << "integer conversion leaves target rounding mode active";
    EXPECT_TRUE((rules.CMisl.Acceleration == .375 && rules.PrismSupportModifier == 300)) << "original cross-field and percentage missing-key defaults";
    ini.WriteString("General", "TalkBubbleTime", "-.125");
    EXPECT_TRUE((rules.Read_General(&ini) && rules.TalkBubbleTime == 0xfffffff9u)) << "negative fractional time truncates before unsigned storage";
    EXPECT_TRUE((rules.PrismSupportModifier == 30000)) << "repeated missing percentage key scales again";
}
void controls() {
    MissionControlClass mission;
    EXPECT_TRUE((mission.ArrayIndex == -1 && mission.Rate == .016 && mission.Recruitable)) << "mission initial state";
    mission.ArrayIndex = static_cast<int>(MissionControlClass::FindIndex("attack"));
    EXPECT_TRUE((mission.ArrayIndex >= 0 && !std::strcmp(mission.GetName(), "Attack"))) << "mission names";
    CCINIClass ini;
    EXPECT_TRUE((!mission.LoadFromINI(&ini))) << "absent mission preserves state";
    ini.WriteString("Attack", "Rate", "125%");
    ini.WriteString("Attack", "NoThreat", "yes");
    EXPECT_TRUE((mission.LoadFromINI(&ini) && mission.Rate == 1.25 && mission.AARate == 1.25 &&
        mission.NoThreat)) << "mission AARate zero default inherits Rate";
    ini.WriteString("Attack", "AARate", ".5");
    EXPECT_TRUE((mission.LoadFromINI(&ini) && mission.AARate == .5)) << "explicit mission AARate";
    ini.WriteString("Attack", "AARate", "");
    EXPECT_TRUE((mission.LoadFromINI(&ini) && mission.AARate == 1.25)) << "missing AARate ignores previous value";
    EXPECT_TRUE((MissionControlClass::Find("unknown") == nullptr)) << "unknown mission lookup";

    struct State { std::vector<int> shades; int colors = 0; int positions[25]{}; int count = 17; } state;
    const int sentinel = -1;
    game::RulesRuntimeServices runtime;
    runtime.context = &state;
    runtime.register_color = [](void* ptr, const char* name, const ColorStruct& color) {
        EXPECT_TRUE((!std::strcmp(name, "Blue") && color.R == 1 && color.G == 2 && color.B == 3)) << "registered RGB color";
        ++static_cast<State*>(ptr)->colors;
        return true;
    };
    runtime.create_color_scheme = [](void* ptr, const char*, const ColorStruct& color, int shades) {
        EXPECT_TRUE((color.R == 1 && color.G == 2 && color.B == 3)) << "scheme borrows the same RGB color";
        static_cast<State*>(ptr)->shades.push_back(shades);
        return true;
    };
    runtime.command_position = [](void* ptr, int command, int position) {
        EXPECT_TRUE((command >= 0 && command < 25)) << "command index";
        static_cast<State*>(ptr)->positions[command] = position;
        return true;
    };
    runtime.command_count = [](void* ptr, int count) { static_cast<State*>(ptr)->count = count; return true; };
    runtime.no_command = &sentinel;
    ini.WriteString("Colors", "Blue", "1,2,3");
    ini.WriteString("AdvancedCommandBar", "Unrelated", "1");
    game::with_rules_runtime(runtime, [](void* ptr) {
        auto* ini = static_cast<CCINIClass*>(ptr);
        EXPECT_TRUE((RulesClass::Read_Colors(ini))) << "color section";
        RulesClass::Read_AdvancedCommandBar(ini, false);
    }, &ini);
    EXPECT_TRUE((state.colors == 1 && state.shades == std::vector<int>({1, 53}))) << "color scheme order";
    EXPECT_TRUE((state.count == 17 && state.positions[0] == -1 && state.positions[24] == -1)) << "absent command list resets positions and retains geometry count";
}
}

TEST(RulesReaders, Contracts) {
    types(); sections(); numeric_overlays(); controls();
}
