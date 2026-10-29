// ObjectType INI and image acquisition, YR 0x005F92D0 and 0x005F9070.
#include "yrpp/ObjectTypeClass.h"
#include "type_resources.hpp"
#include <cstring>
#include <cstdio>

bool ObjectTypeClass::LoadTypeImage() {
    try {
        const auto kind = WhatAmI();
        const auto theater = game::type_theater();
        if (AlternateArcticArt && theater == TheaterType::Snow && !ImageAllocated) {
            if (!ArcticArtInUse) {
                const auto& runtime = game::type_resources();
                char arctic[sizeof(ImageFile)]{};
                if (runtime.arctic_image_name) {
                    if (!runtime.arctic_image_name(runtime.context, ImageFile, arctic, sizeof(arctic)) ||
                        !std::memchr(arctic, 0, sizeof(arctic))) {
                        game::type_resource_result(game::TypeResourceStatus::unavailable);
                        return false;
                    }
                } else {
                    // 0x00832AE8 contains the format bytes "%sA", not a pointer.
                    std::snprintf(arctic, sizeof(arctic), "%sA", ImageFile);
                }
                std::memcpy(ImageFile, arctic, sizeof(ImageFile));
                ArcticArtInUse = true;
            }
        } else
            ArcticArtInUse = false;
        char filename[260]{};
        if (!game::type_image_filename(*this, filename, sizeof(filename), true)) return false;
        if (ImageAllocated && Image) YRMemory::Deallocate(Image);
        Image = nullptr;
        ImageAllocated = false;
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
    } catch (...) {
        game::type_resource_failure(ID);
        return false;
    }
}
bool ObjectTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    try {
        ini->Reset();
        if (!AbstractTypeClass::LoadFromINI(ini)) return false;
        char fallback[sizeof(ImageFile)];
        std::strcpy(fallback, ImageFile);
        ini->ReadString(ID, "Image", fallback, ImageFile, sizeof(ImageFile));
        std::strcpy(fallback, AlphaImageFile);
        ini->ReadString(ID, "AlphaImage", fallback, AlphaImageFile, sizeof(AlphaImageFile));
        if (!game::read_type_sound(*ini, ID, "CrushSound", CrushSound, 128) ||
            !game::read_type_sound(*ini, ID, "AmbientSound", AmbientSound, 128))
            return false;
#define READ_BOOL(field) field = ini->ReadBool(ID, #field, field)
        READ_BOOL(Crushable);
        READ_BOOL(Bombable);
        READ_BOOL(NoSpawnAlt);
        READ_BOOL(AlternateArcticArt);
        READ_BOOL(RadarInvisible);
        READ_BOOL(Selectable);
        READ_BOOL(LegalTarget);
        Armor = static_cast<::Armor>(ini->ReadArmorType(ID, "Armor", static_cast<int>(Armor)));
        Strength = ini->ReadInteger(ID, "Strength", Strength);
        READ_BOOL(Immune);
        READ_BOOL(Insignificant);
        READ_BOOL(HasRadialIndicator);
        RadialColor = ini->ReadColor(ID, "RadialColor", RadialColor);
        READ_BOOL(IgnoresFirestorm);
#undef READ_BOOL
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
            std::snprintf(filename, sizeof(filename), "%s.SHP", AlphaImageFile);
            AlphaImage = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
        }
        return true;
    } catch (...) {
        game::type_resource_failure(ID);
        return false;
    }
}
