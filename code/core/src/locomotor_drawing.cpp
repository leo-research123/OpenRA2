#include "locomotor_drawing.hpp"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/ShipLocomotionClass.h"
#include "yrpp/HoverLocomotionClass.h"
#include <cstring>
namespace game {
Matrix3D locomotor_drawing_matrix(ILocomotion* locomotor, VoxelIndexKey* key, const TechnoTypeClass& type) {
    // Every core locomotor has the original LocomotionClass bases. Query the class identity directly:
    // QueryInterface would transiently change RefCount and Game::COMReferenceCount during extraction.
    CLSID id{};
    if (static_cast<LocomotionClass*>(locomotor)->GetClassID(&id) >= 0) {
        if (!std::memcmp(&id, &LocomotionClass::CLSIDs::Drive, sizeof(id)))
            return drive_drawing_matrix(*static_cast<DriveLocomotionClass*>(locomotor), key, type);
        if (!std::memcmp(&id, &LocomotionClass::CLSIDs::Ship, sizeof(id)))
            return ship_drawing_matrix(*static_cast<ShipLocomotionClass*>(locomotor), key, type);
        if (!std::memcmp(&id, &LocomotionClass::CLSIDs::Hover, sizeof(id)))
            return hover_drawing_matrix(*static_cast<HoverLocomotionClass*>(locomotor), key, type);
    }
    return locomotor->Draw_Matrix(key);
}
}
