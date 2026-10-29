#include "map_view.hpp"
#include "presentation/recorder.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace game {
namespace {
struct Announce {
    const TypeDrawingContext& context;
    unsigned images = 0, failed = 0, refused = 0;
    void add(SHPStruct* image, ShapeSequenceUse use = shape_sequence_color) {
        if (!image) return;
        // Load first: an unloaded reference has no frames to hand over.
        image->Load();
        ++images;
        const auto status = record_shape_sequence(context, image, use);
        if (status == DrawingStatus::unavailable)
            ++refused;
        else if (status != DrawingStatus::drawn)
            ++failed;
    }
    // The art the original draw code reads from a techno type: its image (a building is
    // drawn with the SHA image), building parts and cameo.
    void add_techno(TechnoTypeClass* type) {
        if (!type) return;
        const bool building = type->WhatAmI() == AbstractType::BuildingType;
        add(type->Image, building ? shape_sequence_color_depth : shape_sequence_color);
        add(type->Cameo);
        add(type->AltCameo);
        if (!building) return;
        auto* type_building = static_cast<BuildingTypeClass*>(type);
        for (auto* image : {type_building->Buildup, type_building->DeployingAnim, type_building->UnderDoorAnim,
                            type_building->Rubble, type_building->RoofDeployingAnim, type_building->UnderRoofDoorAnim,
                            type_building->DoorAnim, type_building->SpecialZOverlay})
            add(image, shape_sequence_color_depth);
        add(type_building->BibShape);
    }
};
struct Call {
    Announce& announce;
};
// Announcement order is also the priority: a host with a budget refuses the tail.
// 1. the types of the objects the map starts with (techno and terrain), 2. the types the player can expect to build,
// 3. the remaining animation, bullet and particle art, 4. the building depth image.
void announce_types(void* pointer) {
    auto& a = static_cast<Call*>(pointer)->announce;
    a.add(FileSystem::BUILDINGZ_SHA, shape_sequence_depth);
    for (int i = 0; i < TechnoClass::Array.Count; ++i)
        if (auto* techno = TechnoClass::Array[i]) a.add_techno(techno->GetTechnoType());
    for (int i = 0; i < TerrainClass::Array.Count; ++i)
        if (auto* terrain = TerrainClass::Array[i])
            if (auto* type = terrain->Type) a.add(type->Image);
    // Types the player's country may build at all (owner mask of its parent country, tech level in
    // range); prerequisites are not evaluated, so this is a superset of what the sidebar will offer.
    if (auto* player = HouseClass::CurrentPlayer; player && player->Type) {
        const int parent = player->Type->FindParentCountryIndex();
        if (parent >= 0 && parent < 32)
            for (int i = 0; i < TechnoTypeClass::Array.Count; ++i) {
                auto* type = TechnoTypeClass::Array[i];
                if (type && !type->Unbuildable && type->TechLevel >= 0 && type->InOwners(1u << parent))
                    a.add_techno(type);
            }
    }
    for (int i = 0; i < AnimTypeClass::Array.Count; ++i)
        if (auto* type = AnimTypeClass::Array[i]) a.add(type->Image);
    for (int i = 0; i < BulletTypeClass::Array.Count; ++i)
        if (auto* type = BulletTypeClass::Array[i]) a.add(type->Image);
    for (int i = 0; i < ParticleTypeClass::Array.Count; ++i)
        if (auto* type = ParticleTypeClass::Array[i]) a.add(type->Image);
}
}
void prepare_map_shapes(MapViewHandle& view, const MapDrawingContext& drawing) noexcept {
    if (view.shapes_prepared || !view.terrain_loaded) return;
    view.shapes_prepared = true;
    if (!supports_drawing(drawing.types, DrawingKind::shape_sequence)) return;
    try {
        const auto started = std::chrono::steady_clock::now();
        Announce announce{drawing.types};
        Call call{announce};
        // Reads the types' art the way the original loaders do, inside the map's world.
        with_map_view(view, announce_types, &call);
        if (const auto* text = std::getenv("RA2_SHAPE_STATS"); text && text[0] != '0')
            std::fprintf(stderr, "SHAPE_PREWARM announced=%u failed=%u refused=%u ms=%.1f\n", announce.images,
                         announce.failed, announce.refused,
                         std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
    } catch (...) {
    }
}
}
