// Native registry/device adapter. CommandClass owns hotkey lookup/dispatch.
#include "player_commands.hpp"
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
#include "yrpp/CommandClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/Unsorted.h"
#include "map_world_internal.hpp"
#include <cstring>
#include <utility>
#include "yrpp/Commands/SetStructureTabCommandClass.h"
#include "yrpp/Commands/SetDefenseTabCommandClass.h"
#include "yrpp/Commands/SetInfantryTabCommandClass.h"
#include "yrpp/Commands/SetUnitTabCommandClass.h"
#include "yrpp/Commands/TypeSelectCommandClass.h"
#include "yrpp/Commands/StopCommandClass.h"
#include "yrpp/Commands/DeployCommandClass.h"
#include "yrpp/Commands/GuardCommandClass.h"
#include "yrpp/Commands/ScatterCommandClass.h"
#include "yrpp/Commands/ToggleRepairCommandClass.h"
#include "yrpp/Commands/ToggleSellCommandClass.h"
#include "yrpp/Commands/OptionsCommandClass.h"

namespace game {
namespace {
constexpr unsigned command_count = 14;
int type_selection_key = 0;
int planning_key = 0;
bool bindings_loaded = false;
bool user_override = false;
unsigned defaults[command_count]{}, bindings[command_count]{};
unsigned preferences[command_count]{};
struct CommandOwner {
    CommandClass* items[command_count]{};
    CommandOwner() {
        try {
            items[0] = new ::OptionsCommandClass;
            items[1] = new ::ToggleSellCommandClass;
            items[2] = new ::ToggleRepairCommandClass;
            items[3] = new ::ScatterCommandClass;
            items[4] = new ::GuardCommandClass;
            items[5] = new ::StopCommandClass;
            items[6] = new ::DeployCommandClass;
            items[7] = new ::TypeSelectCommandClass;
            items[8] = new ::SetStructureTabCommandClass;
            items[9] = new ::SetDefenseTabCommandClass;
            items[10] = new ::SetUnitTabCommandClass;
            items[11] = new ::SetInfantryTabCommandClass;
            items[12] = new ::PlanningModeCommandClass;
            items[13] = new ::DeleteCommandClass;
        } catch (...) {
            for (auto* item : items)
                delete item;
            throw;
        }
    }
    ~CommandOwner() {
        // Retire registry references before destroying the owned original classes.
        CommandClass::Hotkeys.Clear();
        for (auto* item : items) {
            const int index = CommandClass::Array.FindItemIndex(item);
            if (index >= 0) CommandClass::Array.RemoveItem(index);
            delete item;
        }
    }
};
CommandClass** commands() {
    static CommandOwner owner;
    return owner.items;
}

void exchange(IndexClass<int, CommandClass*>& a, IndexClass<int, CommandClass*>& b) noexcept {
    std::swap(a.IndexTable, b.IndexTable);
    std::swap(a.IndexCount, b.IndexCount);
    std::swap(a.IndexSize, b.IndexSize);
    std::swap(a.IsSorted, b.IsSorted);
    std::swap(a.Archive, b.Archive);
}
bool replace_bindings(const unsigned* keys) {
    IndexClass<int, CommandClass*> next;
    auto** items = commands();
    for (unsigned i = 0; i < command_count; ++i)
        if (keys[i] && !next.AddIndex(int(keys[i]), items[i])) return false;
    exchange(CommandClass::Hotkeys, next);
    return true;
}
bool interface_command(const CommandClass* c) noexcept {
    auto** items = commands();
    for (unsigned i = 0; i < command_count; ++i)
        if (c == items[i]) return i <= 2 || (i >= 8 && i <= 11);
    return false;
}
}
void cancel_player_hotkeys() noexcept {
    if (planning_key) {
        planning_key = 0;
        Game::EndPlanning(false);
    }
    type_selection_key = 0;
    Game::TypeSelectionActive = false;
}
bool load_player_hotkeys(CCINIClass& ini) noexcept {
    try {
        IndexClass<int, CommandClass*> next;
        unsigned loaded[command_count]{};
        auto** items = commands();
        if (auto* section = ini.GetSection("Hotkey"))
            for (auto* entry : section->Entries) {
                const int key = ini.ReadInteger("Hotkey", entry->Key, 0);
                for (unsigned i = 0; i < command_count; ++i)
                    if (!std::strcmp(items[i]->GetName(), entry->Key)) {
                        if (key && !next.AddIndex(key, items[i])) return false;
                        loaded[i] = unsigned(key);
                        break;
                    }
            }
        exchange(CommandClass::Hotkeys, next);
        for (unsigned i = 0; i < command_count; ++i)
            bindings[i] = loaded[i];
        bindings_loaded = true;
        return true;
    } catch (...) {
        return false;
    }
}
bool initialize_player_commands() noexcept {
    try {
        auto** items = commands();
        const int previous = CommandClass::Array.Count;
        for (unsigned i = 0; i < command_count; ++i)
            if (CommandClass::Array.FindItemIndex(items[i]) < 0 && !CommandClass::Array.AddItem(items[i])) {
                CommandClass::Array.Count = previous;
                return false;
            }
        CCFileClass file("KEYBOARDMD.INI");
        CCINIClass ini;
        if (file.Exists() && ini.ReadCCFile(&file, false, false) > 0) {
            if (!load_player_hotkeys(ini)) return false;
        } else {
            const unsigned fallback[]{27, 'L', 'K', 'X', 'G', 'S', 'D', 'T', 'Q', 'W', 'R', 'E', 'Z', 46};
            if (!replace_bindings(fallback)) return false;
            for (unsigned i = 0; i < command_count; ++i)
                bindings[i] = fallback[i];
        }
        for (unsigned i = 0; i < command_count; ++i)
            defaults[i] = bindings[i];
        if (user_override) {
            if (!replace_bindings(preferences)) return false;
            for (unsigned i = 0; i < command_count; ++i)
                bindings[i] = preferences[i];
        }
        bindings_loaded = true;
        return true;
    } catch (...) {
        return false;
    }
}
unsigned player_hotkey_count() noexcept { return command_count; }
bool read_player_hotkey(unsigned index, PlayerHotkeyBinding& output) noexcept {
    try {
        if (index >= command_count || (!bindings_loaded && !initialize_player_commands())) return false;
        auto* c = commands()[index];
        output = {c->GetName(), c->GetUIName(), bindings[index], defaults[index]};
        return true;
    } catch (...) {
        return false;
    }
}
bool set_player_hotkeys(const unsigned* keys, unsigned count) noexcept {
    try {
        if (!keys || count != command_count || (!bindings_loaded && !initialize_player_commands())) return false;
        for (unsigned i = 0; i < count; ++i) {
            if (keys[i] && (keys[i] & ~0x17FFu || !(keys[i] & 0xFFu))) return false;
            for (unsigned j = 0; j < i; ++j)
                if (keys[i] && keys[i] == keys[j]) return false;
        }
        if (!replace_bindings(keys)) return false;
        for (unsigned i = 0; i < count; ++i)
            preferences[i] = bindings[i] = keys[i];
        user_override = true;
        cancel_player_hotkeys();
        return true;
    } catch (...) {
        return false;
    }
}
bool dispatch_player_hotkey(WWKey input) noexcept {
    try {
        if (!int(input)) return false;
        auto** items = commands();
        if ((int(input) & 0x800) && type_selection_key && (int(input) & 0xFF) == type_selection_key) {
            if (auto* world = current_map_world()) rebuild_world_selectables(*world);
            type_selection_key = 0;
            items[7]->Execute(input);
            return true;
        }
        if ((int(input) & 0x800) && planning_key && (int(input) & 0xFF) == planning_key) {
            planning_key = 0;
            items[12]->Execute(input);
            return true;
        }
        const int key = int(input) & ~int(WWKey::Release);
        CommandClass* command = nullptr;
        if (!CommandClass::FindHotkey(input, command) || (Unsorted::UserInputLocked && !interface_command(command)))
            return false;
        if (command == items[12] && !(int(input) & 0x800)) planning_key = key & 0xFF;
        if (command == items[7]) {
            if (int(input) & 0x800) {
                if (auto* world = current_map_world()) rebuild_world_selectables(*world);
                type_selection_key = 0;
            } else if (!Game::TypeSelectionActive)
                type_selection_key = key & 0xFF;
        }
        // Host events arrive individually, with no duplicate device queue to drain.
        bool drain = false;
        return command->ProcessInput(input, drain);
    } catch (...) {
        return false;
    }
}
void deploy_selected_objects() noexcept { Game::DeploySelected(); }
void stop_selected_objects() noexcept { Game::StopSelected(); }
}
