#pragma once
#include "presentation/context.hpp"
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/FootClass.h"
#include "yrpp/TechnoClass.h"
struct SHPStruct;
namespace game {
// The original TechnoClass::DrawObject (YR 0x00705E00): one body SHP frame of a techno and its shadow.
void draw_object(PresentationContext&, const TechnoClass&, SHPStruct* image, int frame, Point2D* location,
                 RectangleStruct* bounds, int, int, int z, ZGradient gradient, int write_depth, int intensity, int tint,
                 SHPStruct* depth_image, int depth_frame, int depth_x, int depth_y, int excluded);
// The original FootClass::Draw_A_SHP (virtual): forwards to the body draw above.
void draw_a_shp(PresentationContext&, const FootClass&, SHPStruct* image, int frame, Point2D* point,
                RectangleStruct* clip, DWORD rotation, DWORD scale, DWORD z, ZGradient gradient, DWORD write_depth,
                int intensity, DWORD tint, SHPStruct* depth_image, DWORD depth_frame, DWORD depth_x, DWORD depth_y,
                DWORD excluded);
// Inputs of a shadow draw that the original reads while the object is lowered to the ground (aircraft): see
// aircraft_shadow_cycle.hpp. Null means: query the object now.
struct ShadowSample {
    bool has_shadow = true;
    Point2D delta{};
    int depth = 0;
};
// The original TechnoClass::DrawVoxel (virtual, YR 0x00706640) and TechnoClass::DrawVoxelShadow (YR 0x00706BD0).
void draw_voxel(PresentationContext&, const TechnoClass&, const VoxelStruct& voxel, DWORD frame, int key,
                const IndexClass<VoxelIndexKey, VoxelCacheStruct*>&, const RectangleStruct& clip, const Point2D& point,
                const Matrix3D& matrix, int intensity, DWORD tint, DWORD excluded);
void draw_voxel_shadow(PresentationContext&, const TechnoClass&, VoxelStruct* voxel, int layer, VoxelIndexKey key,
                       IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip, Point2D* point,
                       Matrix3D* matrix, bool force, Surface* exclusion, Point2D shadowPoint,
                       const ShadowSample* sampled = nullptr);
// FootClass::Draw_A_VXL (virtual) and FootClass::DrawVoxelShadow: the locomotor's draw point first.
void draw_a_vxl(PresentationContext&, const FootClass&, VoxelStruct* voxel, int frame, int key,
                IndexClass<VoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip, Point2D* point,
                Matrix3D* matrix, int intensity, BlitterFlags flags, DWORD tint);
void draw_foot_voxel_shadow(PresentationContext&, const FootClass&, VoxelStruct* voxel, int layer, VoxelIndexKey key,
                            IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip,
                            Point2D* point, Matrix3D* matrix, bool force, Surface* surface, Point2D shadowPoint,
                            const ShadowSample* sampled = nullptr);
}

class TechnoTypeClass;
namespace game {
TechnoTypeClass* drawing_techno_type(const PresentationContext&, const TechnoClass&);
int drawing_z_adjustment(const PresentationContext&, const TechnoClass&);
VisualType drawing_visual_character(const PresentationContext&, const TechnoClass&);
}
