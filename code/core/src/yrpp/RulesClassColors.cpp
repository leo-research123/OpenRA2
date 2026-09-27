// YR 66D3A0: register each named RGB value, then create one- and 53-shade
// schemes with the game's existing palettes. ColorScheme owns those objects.
#include "yrpp/RulesClass.h"
#include "RulesClassReaders.hpp"

bool YRPP_STDCALL RulesClass::Read_Colors(CCINIClass* ini) {
    if (!ini || !ini->GetSection("Colors")) return false;
    const int count = ini->GetKeyCount("Colors");
    for (int index = 0; index < count; ++index) {
        const char* name = ini->GetKeyName("Colors", index);
        const ColorStruct color = ini->ReadColor("Colors", name, ColorStruct{0, 0, 0});
        const auto& runtime = game::rules_runtime();
        if (!runtime.register_color || !runtime.register_color(runtime.context, name, color))
            throw std::runtime_error("RulesClass color registration dependency failed");
        if (!runtime.create_color_scheme ||
            !runtime.create_color_scheme(runtime.context, name, color, 1) ||
            !runtime.create_color_scheme(runtime.context, name, color, 53))
            throw std::runtime_error("RulesClass color scheme dependency failed");
    }
    return true;
}
