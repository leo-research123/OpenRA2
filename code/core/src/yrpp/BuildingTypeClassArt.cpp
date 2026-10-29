// BuildingTypeClass non-virtual resource pass, YR 0x0045F230.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/RulesClass.h"
#include "type_resources.hpp"
#include "x87_integer.hpp"
#include <bit>
#include <cstring>

namespace {
void clear_owned(SHPStruct*& image, bool& owned) {
    if (owned && image) {
        YRMemory::Deallocate(image);
        image = nullptr;
        owned = false;
    }
}
void buildup_frames(BuildingTypeClass& type) {
    if (!type.Buildup) return;
    auto& sequence = type.BuildingAnimFrame[0];
    sequence.dwUnknown = 0;
    sequence.FrameCount = type.Gate ? std::bit_cast<int>(DWORD(type.GateStages) + 1) : type.Buildup->Frames / 2;
    sequence.FrameDuration =
        sequence.FrameCount > 0
            ? game::x87_scaled_quotient_to_integer(RulesClass::Instance->BuildupTime, 900.0, sequence.FrameCount)
            : 1;
}
}

#if !defined(RA2_YRPP_GAME)
void BuildingTypeClass::LoadArt(TheaterType theater) {
    try {
        auto& art = game::type_art_ini();
        char name[64]{}, filename[260]{};
        const auto path = [&](const char* base, bool extension = false) {
            return game::building_shape_filename(base, extension, theater, filename, sizeof(filename));
        };
        const auto fallback = [&] { filename[1] = 'G'; };
        if (!DemandLoadBuildup && *BuildupFile) {
            if (!path(BuildupFile)) return;
            auto* buildup = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
            if (!buildup) {
                fallback();
                buildup = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
            }
            // 0x45F2AC publishes only after both cache lookups have finished.
            Buildup = buildup;
            buildup_frames(*this);
        }
        const auto read = [&](const char* key) {
            art.ReadString(ImageFile, key, "", name, sizeof(name));
            return *name != 0;
        };
        const auto extended = [&](const char* key, SHPStruct*& image, bool& owned, bool* fallback_owned,
                                  bool cached_fallback) {
            if (!read(key)) return true;
            if (!path(name)) return false;
            clear_owned(image, owned);
            image = static_cast<SHPStruct*>(FileSystem::LoadWholeFileEx(filename, owned));
            if (!image) {
                fallback();
                clear_owned(image, owned);
                image =
                    static_cast<SHPStruct*>(cached_fallback ? FileSystem::LoadFile(filename, false)
                                                            : FileSystem::LoadWholeFileEx(filename, *fallback_owned));
            }
            return true;
        };
        const auto cached = [&](const char* key, SHPStruct*& image) {
            if (!read(key)) return true;
            if (!path(name)) return false;
            image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
            if (!image) {
                fallback();
                image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
            }
            return true;
        };
        if (!extended("DeployingAnim", DeployingAnim, DeployingAnimLoaded, &DeployingAnimLoaded, false)) return;
        // 0x0045F4B9 writes the Deploying flag on the RoofDeploying fallback.
        if (!extended("RoofDeployingAnim", RoofDeployingAnim, RoofDeployingAnimLoaded, &DeployingAnimLoaded, false))
            return;
        if (!cached("DoorAnim", DoorAnim)) return;
        if (!extended("UnderDoorAnim", UnderDoorAnim, UnderDoorAnimLoaded, nullptr, true)) return;
        clear_owned(Rubble, RubbleLoaded);
        if (!extended("Rubble", Rubble, RubbleLoaded, nullptr, true)) return;
        if (!extended("UnderRoofDoorAnim", UnderRoofDoorAnim, UnderRoofDoorAnimLoaded, nullptr, true)) return;
        if (!cached("SpecialZOverlay", SpecialZOverlay)) return;
        if (!extended("BibShape", BibShape, BibShapeLoaded, nullptr, true)) return;
        read("Image");
        if (!path(*name ? name : ImageFile, Theater)) return;
        std::strncpy(TheaterSpecificID, filename, sizeof(TheaterSpecificID));
        if (!DemandLoad) {
            Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
            for (int attempt = 0; !Image && attempt < 2; ++attempt) {
                fallback();
                std::strncpy(TheaterSpecificID, filename, sizeof(TheaterSpecificID));
                Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
            }
        }
        LoadTurretBarrelVoxels();
    } catch (...) {
        game::type_resource_failure(ID);
    }
}
#endif
