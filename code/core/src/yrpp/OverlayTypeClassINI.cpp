// Supplied 005FE770; scalar keys, Art section and reference allocation order.
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "type_resources.hpp"
bool OverlayTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    try {
        if (!ObjectTypeClass::LoadFromINI(ini)) return false;
        LandType = static_cast<::LandType>(ini->ReadLandType(ID, "Land", static_cast<int>(LandType)));
        Strength = ini->ReadInteger(ID, "Strength", Strength);
#define READ_BOOL(field) field = ini->ReadBool(ID, #field, field)
        READ_BOOL(Wall); READ_BOOL(Tiberium); READ_BOOL(Crate); READ_BOOL(CrateTrigger);
        READ_BOOL(Explodes); READ_BOOL(Overrides);
        char animation[512]{};
        if (ini->ReadString(ID, "CellAnim", "", animation, sizeof(animation)))
            CellAnim = AnimTypeClass::FindOrAllocate(animation);
        DamageLevels = game::type_art_ini().ReadInteger(ImageFile, "DamageLevels", DamageLevels);
        if (Tiberium) {
            Armor = static_cast<::Armor>(6);
            if (LandType == static_cast<::LandType>(0)) LandType = static_cast<::LandType>(5);
        }
        if (!Theater && !ImageLoaded) {
            char filename[260];
            // This particular original branch does not apply NewTheater renaming.
            const bool saved = NewTheater;
            NewTheater = false;
            const bool named = game::type_image_filename(*this, filename, sizeof(filename), false);
            NewTheater = saved;
            if (!named) return false;
            Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, true));
        }
        RadarColor = ini->ReadColor(ID, "RadarColor", RadarColor);
        READ_BOOL(NoUseTileLandType); READ_BOOL(IsVeinholeMonster); READ_BOOL(IsVeins);
        READ_BOOL(ChainReaction); READ_BOOL(DrawFlat); READ_BOOL(IsARock); READ_BOOL(IsRubble);
#undef READ_BOOL
        return true;
    } catch (...) { game::type_resource_result(game::TypeResourceStatus::failure); return false; }
}
