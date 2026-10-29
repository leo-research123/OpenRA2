// Cell-tag phase of DisplayClass::LoadFromINI, 0x4AD1B3..0x4AD332.
#include "yrpp/DisplayClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TagClass.h"
#include <cstdlib>

bool DisplayClass::LoadCellTagsFromINI(CCINIClass& ini) noexcept {
    try {
        const int count = ini.GetKeyCount("CellTags");
        for (int i = 0; i < count; ++i) {
            const char* key = ini.GetKeyName("CellTags", i);
            char id[128];
            auto* type = ini.ReadString("CellTags", key, "", id, sizeof(id)) ? TagTypeClass::Find(id) : nullptr;
            const int packed = std::atoi(key);
            const int stride = ScenarioClass::NewINIFormat < 4 ? 128 : 1000;
            const CellStruct at{short(packed % stride), short(packed / stride)};
            if (!type) continue;
            auto* cell = GetCellAt(at);
            if (cell->AttachedTag) continue;
            if (auto* tag = TagClass::GetInstance(type)) {
                tag->DefaultCoords = at;
                GetCellAt(at)->ReplaceTag(tag);
                // ReplaceTag also appends to this list in the original.
                // Keep the loader's second append and its original order.
                TaggedCells.AddItem(at);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}
