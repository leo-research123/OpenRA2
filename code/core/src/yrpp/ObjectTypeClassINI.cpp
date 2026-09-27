// Supplied 005F92D0 and 005F9070, mapped to the existing original fields.
#include "yrpp/ObjectTypeClass.h"
#include "type_resources.hpp"
#include <cstring>
#include <cstdio>

bool ObjectTypeClass::LoadTypeImage() {
    const auto kind = WhatAmI();
    const auto theater = game::type_theater();
    if (AlternateArcticArt && (static_cast<int>(theater) < 0 || static_cast<int>(theater) >= 6)) {
        game::type_resource_result(game::TypeResourceStatus::unavailable); return false;
    }
    if (AlternateArcticArt && theater == TheaterType::Snow && !ImageAllocated) {
        if (!ArcticArtInUse) {
            const auto& runtime = game::type_resources();
            if (!runtime.arctic_image_name) {
                // The omitted target format string cannot be inferred from a
                // pointer reference. Original callers may explicitly use the
                // retained LoadTypeImageOriginal entry; native callers fail.
                game::type_resource_result(game::TypeResourceStatus::unavailable); return false;
            }
            char arctic[sizeof(ImageFile)]{};
            if (!runtime.arctic_image_name(runtime.context, ImageFile, arctic, sizeof(arctic)) ||
                    !std::memchr(arctic, 0, sizeof(arctic))) {
                game::type_resource_result(game::TypeResourceStatus::unavailable); return false;
            }
            std::memcpy(ImageFile, arctic, sizeof(ImageFile));
            ArcticArtInUse = true;
        }
    } else ArcticArtInUse = false;
    char filename[260]{};
    if (!game::type_image_filename(*this, filename, sizeof(filename), true)) return false;
    if (ImageAllocated) YRMemory::Deallocate(Image);
    Image = nullptr; ImageAllocated = false;
    // These original subclasses perform their own resource acquisition.
    if (kind == AbstractType::SmudgeType || kind == AbstractType::TerrainType) return true;
    const bool reference = kind == AbstractType::OverlayType || kind == AbstractType::AnimType;
    Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, reference));
    if (!Image && std::strlen(filename) > 1) {
        filename[1] = 'G';
        Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, reference));
    }
    if (Image) {
        MaxDimension = Image->Width > Image->Height ? Image->Width : Image->Height;
        if (MaxDimension < 8) MaxDimension = 8;
    }
    return true;
}
bool ObjectTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    try {
        if (!AbstractTypeClass::LoadFromINI(ini)) return false;
        ini->ReadString(ID, "Image", ImageFile, ImageFile, sizeof(ImageFile));
        ini->ReadString(ID, "AlphaImage", AlphaImageFile, AlphaImageFile, sizeof(AlphaImageFile));
        if (!game::read_type_sound(*ini, ID, "CrushSound", CrushSound) ||
            !game::read_type_sound(*ini, ID, "AmbientSound", AmbientSound)) return false;
#define READ_BOOL(field) field = ini->ReadBool(ID, #field, field)
        READ_BOOL(Crushable); READ_BOOL(Bombable); READ_BOOL(NoSpawnAlt);
        READ_BOOL(AlternateArcticArt); READ_BOOL(RadarInvisible); READ_BOOL(Selectable);
        READ_BOOL(LegalTarget); READ_BOOL(Immune); READ_BOOL(Insignificant);
        READ_BOOL(HasRadialIndicator); READ_BOOL(IgnoresFirestorm);
#undef READ_BOOL
        Armor = static_cast<::Armor>(ini->ReadArmorType(ID, "Armor", static_cast<int>(Armor)));
        Strength = ini->ReadInteger(ID, "Strength", Strength);
        RadialColor = ini->ReadColor(ID, "RadialColor", RadialColor);
        auto& art = game::type_art_ini();
        UseLineTrail = art.ReadBool(ImageFile, "UseLineTrail", UseLineTrail);
        LineTrailColor = art.ReadColor(ImageFile, "LineTrailColor", LineTrailColor);
        LineTrailColorDecrement = art.ReadInteger(ImageFile, "LineTrailColorDecrement", LineTrailColorDecrement);
        Theater = art.ReadBool(ImageFile, "Theater", Theater);
        NewTheater = art.ReadBool(ImageFile, "NewTheater", NewTheater);
        Voxel = art.ReadBool(ImageFile, "Voxel", Voxel);
        if (!Voxel && WhatAmI() != AbstractType::AnimType && !LoadTypeImage()) return false;
        if (*AlphaImageFile) {
            char filename[260];
            std::snprintf(filename, sizeof(filename), "%s.shp", AlphaImageFile);
            AlphaImage = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
        }
        return true;
    } catch (...) { game::type_resource_result(game::TypeResourceStatus::failure); return false; }
}
