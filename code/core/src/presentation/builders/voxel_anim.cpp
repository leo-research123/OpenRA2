// YR 0x00749B70; submit through the same core voxel backend as other objects.
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include "yrpp/Matrix3DArithmetic.hpp"

#include "presentation/builders/voxel_anim.hpp"
namespace game {
void build_voxel_anim(PresentationContext& ctx, const VoxelAnimClass& self, Point2D* point, RectangleStruct* clip) {
    if (!point || !clip || !self.Type || self.Invisible || !self.Type->MainVoxel.VXL) return;
    auto* cell = self.GetCell();
    if (ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar && cell->IsFogged()) return;
    const auto* resources = ctx.resources;
    if (!resources || !resources->voxel || !resources->voxel_camera) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::unavailable);
        return;
    }
    game::ObjectVoxelRequest request;
    // 0x749B70 passes the VXL directly; debris never applies an HVA.
    request.apply_hva = false;
    request.voxel_index = self.Type->VoxelIndex;
    // Both 0x4AF2A0 calls use gradient zero for debris.
    request.flat = true;
    request.resource = &self.Type->MainVoxel;
    request.clip = *clip;
    request.position = *point;
    const auto rotation = self.Bounce.GetDrawingMatrix();
    // 0x753F90 flattens the rotated shadow onto the ground before the camera.
    auto shadow_rotation = rotation;
    for (int column = 0; column < 4; ++column)
        shadow_rotation.row[2][column] = 0;
    request.matrix = drawing_matrix_product(*resources->voxel_camera, shadow_rotation);
    int floor = MapClass::Instance.GetCellFloorHeight(self.Location);
    const bool on_bridge = (unsigned(cell->Flags) & 0x100u) && self.Location.Z >= floor + CellClass::BridgeHeight;
    if (on_bridge) floor += CellClass::BridgeHeight;
    auto ground = self.Location;
    ground.Z = floor + (on_bridge ? 1 : 0);
    Point2D projected;
    TacticalClass::Instance->CoordsToClient(&ground, &projected);
    const auto origin = request.position;
    request.position = projected;
    request.flags = 0x2001;
    request.shadow = true;
    request.depth_adjustment = -4 - TacticalClass::AdjustForZ(floor);
    if (!game::resolve_drawing_palette(ctx, game::DrawingPaletteKind::normal, -1, request.palette)) return;
    game::record_type_drawing_result(ctx, resources->voxel(resources->context, request));
    request.position = origin;
    request.shadow = false;
    request.matrix = drawing_matrix_product(*resources->voxel_camera, rotation);
    request.depth_adjustment = -2 - TacticalClass::AdjustForZ(self.Location.Z);
    request.flags = self.Type->Translucent ? 0x2804 : 0x2800;
    request.intensity = self.OwnerHouse || self.Type->IsTiberium ? static_cast<short>(cell->Intensity_Normal) : 1000;
    const auto palette = self.OwnerHouse         ? game::DrawingPaletteKind::color_scheme
                         : self.Type->IsTiberium ? game::DrawingPaletteKind::animation
                                            : game::DrawingPaletteKind::normal;
    if (!game::resolve_drawing_palette(ctx, palette, self.OwnerHouse ? self.OwnerHouse->ColorSchemeIndex : -1, request.palette))
        return;
    game::record_type_drawing_result(ctx, resources->voxel(resources->context, request));
}
}
