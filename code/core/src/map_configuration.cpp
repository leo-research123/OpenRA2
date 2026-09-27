#include "map_configuration.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "rules_runtime.hpp"
#include "scenario_runtime.hpp"
#include "yrpp/FileSystem.h"
#include <cstdio>
#include <exception>
#include <stdexcept>

namespace game {
bool parse_map_rule_fields(MapWorld& world, CCINIClass& ini, char* error, std::size_t capacity) noexcept {
    try {
        if (!RulesClass::Instance) throw std::runtime_error("Rules object is unavailable");
        auto services = native_rules_runtime();
        services.context = &world;
        services.defer_sound_references = true;
        services.retain_sound_reference = [](void* p, const CCINIClass*, const char* section, const char* key) noexcept {
            try {
                static_cast<MapWorld*>(p)->impl->deferred_sound_fields.emplace_back(section,key);
                return true;
            } catch (...) { return false; }
        };
        services.shape = [](void*, const char* name, SHPStruct*& result) {
            result = static_cast<SHPStruct*>(FileSystem::LoadFile(name,false));
            return true; // Missing optional art is the original null result.
        };
        struct Context { MapWorld& world; CCINIClass& ini; } context{world,ini};
        return with_rules_runtime(services, [](void* p) {
            auto& c=*static_cast<Context*>(p);
            auto& rules=*RulesClass::Instance;
            const auto read=[&](const char* section, bool (RulesClass::*reader)(CCINIClass*)) {
                if (!c.ini.GetSection(section)) return;
                if (!(rules.*reader)(&c.ini))
                    throw std::runtime_error(std::string("Could not parse rules section ")+section);
                c.world.impl->parsed_rule_sections.emplace_back(section);
            };
            // Scalar/reference readers from 0x00668BF0, retaining their own
            // conversions. Registry creation/type-art loading remain with the
            // native world; unsupported object controllers are not started.
            read("ColorAdd", &RulesClass::Read_ColorAdd);
            read("JumpjetControls", &RulesClass::Read_JumpjetControls);
            read("MultiplayerDialogSettings", &RulesClass::Read_MultiplayerDialogSettings);
            read("AI", &RulesClass::Read_AI);
            read("IQ", &RulesClass::Read_IQ);
            read("General", &RulesClass::Read_General);
            rules.Read_Difficulties(&c.ini);
            for (const char* s : {"Easy","Normal","Difficult"})
                if (c.ini.GetSection(s)) c.world.impl->parsed_rule_sections.emplace_back(s);
            read("CrateRules", &RulesClass::Read_CrateRules);
            read("CombatDamage", &RulesClass::Read_CombatDamage);
            read("Radiation", &RulesClass::Read_Radiation);
            read("ElevationModel", &RulesClass::Read_ElevationModel);
            read("WallModel", &RulesClass::Read_WallModel);
            read("AudioVisual", &RulesClass::Read_AudioVisual);
            read("SpecialWeapons", &RulesClass::Read_SpecialWeapons);
            read("Maximums", &RulesClass::Read_Maximums);
            if(c.ini.GetSection("Powerups")) {
                if(!RulesClass::Read_Powerups(&c.ini))throw std::runtime_error("Powerup configuration parsing failed");
                c.world.impl->parsed_rule_sections.emplace_back("Powerups");
            }
            for(int i=0;i<32;++i) {
                auto& mission=MissionControlClass::Array[i];mission.ArrayIndex=i;
                if(mission.LoadFromINI(&c.ini))c.world.impl->parsed_rule_sections.emplace_back(mission.GetName());
            }
            RulesClass::Read_LandCharacteristics(&c.ini);
        }, &context);
    } catch (const std::exception& e) { if (error&&capacity) std::snprintf(error,capacity,"%s",e.what()); }
      catch (...) { if (error&&capacity) std::snprintf(error,capacity,"Rule configuration parsing failed"); }
    return false;
}

bool parse_map_scenario_fields(MapWorld& world, char* error, std::size_t capacity) noexcept {
    try {
        if(world.art_ini.GetSection("Movies")&&!RulesClass::Read_Movies(&world.art_ini))
            throw std::runtime_error("Movie name configuration parsing failed");
        const bool armageddon=false;
        ScenarioIniServices ini_services{};
        ini_services.armageddon_mode=&armageddon;
        ini_services.fields_only=true;
        auto services=scenario_runtime();
        services.ini=&ini_services;
        struct Context { MapWorld& world; bool parsed=false; } context{world};
        const bool entered=with_scenario_runtime(services,[](void* p) {
            auto& c=*static_cast<Context*>(p);
            c.parsed=c.world.impl->view.scenario.ReadINI(c.world.map_ini);
        },&context);
        if (!entered||!context.parsed) throw std::runtime_error("Scenario configuration parsing failed");
        world.impl->scenario_fields_parsed=true;
        return true;
    } catch (const std::exception& e) { if(error&&capacity) std::snprintf(error,capacity,"%s",e.what()); }
      catch (...) { if(error&&capacity) std::snprintf(error,capacity,"Scenario configuration parsing failed"); }
    return false;
}
}
