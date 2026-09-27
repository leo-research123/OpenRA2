/*
 * Rules processing order adapts EA REDALERT/RULES.CPP::Process, revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts
 * Inc. GPL-3.0-or-later with terms in third_party/ea/LICENSE.TXT.
 * YR Init/Read_File at 6686C0/668BF0 add dynamic registry cleanup, language
 * rules and game-mode overlays. Owning classes provide external dependencies.
 */
#include "yrpp/RulesClass.h"
#include "yrpp/ColorScheme.h"
#include "RulesClassReaders.hpp"

namespace {
void check_color_capacity(CCINIClass& ini) {
    if (ini.GetKeyCount("ColorAdd") > 16)
        throw std::length_error("RulesClass ColorAdd exceeds its 16 slots");
}
void clear_types(AbstractType kind) {
    const auto& runtime = game::rules_runtime();
    int count = 0;
    if (!game::rules_type_count(kind, count))
        throw std::runtime_error("RulesClass reset requires the original type registry");
    while (count) {
        if (!runtime.destroy_first_type || !runtime.destroy_first_type(runtime.context, kind))
            throw std::runtime_error("RulesClass type destruction dependency failed");
        int remaining = 0;
        if (!game::rules_type_count(kind, remaining) || remaining >= count)
            throw std::runtime_error("RulesClass type destructor did not remove its registry entry");
        count = remaining;
    }
}
}
void RulesClass::Read_File(CCINIClass* ini) {
    if (!ini) throw std::invalid_argument("RulesClass::Read_File requires an INI");
    check_color_capacity(*ini);
    Read_Colors(ini);
    Read_ColorAdd(ini);
    Read_Countries(ini);
    Read_Sides(ini);
    Read_OverlayTypes(ini);
    Read_SuperWeaponTypes(ini);
    Read_Warheads(ini);
    Read_SmudgeTypes(ini);
    Read_TerrainTypes(ini);
    Read_BuildingTypes(ini);
    Read_VehicleTypes(ini);
    Read_AircraftTypes(ini);
    Read_InfantryTypes(ini);
    Read_Animations(ini);
    Read_VoxelAnims(ini);
    Read_Particles(ini);
    Read_ParticleSystems(ini);
    Read_JumpjetControls(ini);
    Read_MultiplayerDialogSettings(ini);
    Read_AI(ini);
    Read_Powerups(ini);
    Read_LandCharacteristics(ini);
    Read_IQ(ini);
    Read_General(ini);
    LoadTypesFromINI(ini);
    Read_Difficulties(ini);
    Read_CrateRules(ini);
    Read_CombatDamage(ini);
    Read_Radiation(ini);
    Read_ElevationModel(ini);
    Read_WallModel(ini);
    Read_AudioVisual(ini);
    Read_SpecialWeapons(ini);
    const auto& runtime = game::rules_runtime();
    if (!runtime.read_tiberiums || !runtime.read_tiberiums(runtime.context, ini))
        throw std::runtime_error("RulesClass Tiberium content dependency failed");
    if (!runtime.session_mode)
        throw std::runtime_error("RulesClass session mode is unavailable");
    const int mode = *runtime.session_mode;
    Read_AdvancedCommandBar(ini, mode != 0 && mode != 5);
}

void RulesClass::Init(CCINIClass* ini) {
    if (!ini) throw std::invalid_argument("RulesClass::Init requires an INI");
    check_color_capacity(*ini);
    const auto& runtime = game::rules_runtime();
    if (!runtime.color_schemes || !runtime.clear_color_tables)
        throw std::runtime_error("RulesClass reset requires the existing color registries");
    auto& colors = *runtime.color_schemes;
    while (colors.Count) {
        const int count = colors.Count;
        auto* color = colors.Items[0];
        if (!color || !runtime.destroy_color_scheme || !runtime.destroy_color_scheme(runtime.context, color) ||
            colors.Count >= count)
            throw std::runtime_error("RulesClass color destructor did not remove its registry entry");
    }
    colors.Count = 0;
    colors.Clear();
    if (!runtime.clear_color_tables(runtime.context))
        throw std::runtime_error("RulesClass color table cleanup failed");
    for (AbstractType kind : {AbstractType::TeamType, AbstractType::TaskForce, AbstractType::ScriptType,
            AbstractType::OverlayType, AbstractType::SmudgeType, AbstractType::TerrainType,
            AbstractType::InfantryType, AbstractType::UnitType, AbstractType::AircraftType,
            AbstractType::BulletType, AbstractType::BuildingType, AbstractType::SuperWeaponType,
            AbstractType::AnimType, AbstractType::WeaponType, AbstractType::WarheadType,
            AbstractType::VoxelAnimType, AbstractType::ParticleType, AbstractType::ParticleSystemType,
            AbstractType::House, AbstractType::HouseType})
        clear_types(kind);
    Read_Maximums(ini);
    Read_File(ini);
    for (auto& color : ColorAdd) color = ColorStruct{0, 0, 0};
    CCFileClass language_file("LANGRULE.INI");
    if (language_file.Exists(false)) {
        CCINIClass language;
        if (language.ReadCCFile(&language_file, true, false) > 1) return;
        Read_File(&language);
    }
    Read_ColorAdd(ini);
    if (!runtime.session_mode)
        throw std::runtime_error("RulesClass session mode is unavailable");
    if (*runtime.session_mode != 0) {
        CCINIClass* mode_ini = nullptr;
        if (!runtime.mode_ini || !runtime.mode_ini(runtime.context, mode_ini))
            throw std::runtime_error("RulesClass game-mode INI dependency failed");
        if (mode_ini) Read_File(mode_ini);
    }
}
