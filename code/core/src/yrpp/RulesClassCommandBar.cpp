// YR-specific advanced-command bar reader, fixed entry 674650.
#include "yrpp/RulesClass.h"
#include "RulesClassReaders.hpp"
#include <cstring>

bool YRPP_STDCALL RulesClass::Read_AdvancedCommandBar(CCINIClass* ini, bool multiplayer) {
    const char* section = multiplayer ? "MultiplayerAdvancedCommandBar" : "AdvancedCommandBar";
    if (!ini || !ini->GetSection(section)) return false;
    const auto& runtime = game::rules_runtime();
    if (!runtime.command_position || !runtime.no_command)
        throw std::runtime_error("RulesClass command position dependency is unavailable");
    for (int index = 0; index < 25; ++index)
        if (!runtime.command_position(runtime.context, index, *runtime.no_command))
            throw std::runtime_error("RulesClass command position reset failed");
    char text[512];
    if (ini->ReadString(section, "ButtonList", "", text, sizeof(text))) {
        // Actual 6CFCC0 table has 11 names; the position array has 25 slots.
        // Comparison is case-sensitive. Unknown names still consume a slot;
        // duplicates overwrite their earlier position.
        constexpr const char* names[] = {"Team01", "Team02", "Team03", "TypeSelect", "Deploy",
            "AttackMove", "Guard", "Beacon", "Stop", "PlanningMode", "Cheer"};
        int count = 0;
        char* cursor = text;
        while (char* name = next_rule_token(cursor)) {
            int command = *runtime.no_command;
            for (int index = 0; index < 11; ++index)
                if (!std::strcmp(name, names[index])) { command = index; break; }
            if (command != *runtime.no_command &&
                !runtime.command_position(runtime.context, command, count))
                throw std::runtime_error("RulesClass command position update failed");
            ++count;
        }
        if (!runtime.command_count || !runtime.command_count(runtime.context, count))
            throw std::runtime_error("RulesClass command geometry dependency failed");
    }
    return true;
}
