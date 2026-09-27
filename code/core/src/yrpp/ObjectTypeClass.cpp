// Original resource queries: 41CFA0 and 5F9970 (delegates to 4103E0).
#include "yrpp/ObjectTypeClass.h"
// ObjectType::Occupy_List, YR 0x5F7640: shared one-cell footprint.
CellStruct* ObjectTypeClass::GetFoundationData(bool) const {
    static CellStruct footprint[]{{0,0},{0x7FFF,0x7FFF}};
    return footprint;
}
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include <cstring>

namespace { DynamicVectorClass<ObjectTypeClass*> object_types; }
DynamicVectorClass<ObjectTypeClass*>& ObjectTypeClass::Array = object_types;

// RA1's ObjectType has a different fixed-heap design. These YR fields and the
// dynamic registration/ownership order are calibrated at 5F7090/5F7400.
ObjectTypeClass::ObjectTypeClass(const char* id) noexcept : AbstractTypeClass(id),
    RadialColor(0, 0, 0), Armor(static_cast<::Armor>(0)), Strength(0),
    Image(nullptr), ImageAllocated(false), AlphaImage(nullptr),
    MainVoxel{}, TurretVoxel{}, BarrelVoxel{}, ChargerTurrets{}, ChargerBarrels{},
    NoSpawnAlt(false), MaxDimension(0), CrushSound(-1), AmbientSound(-1),
    ImageFile{}, AlternateArcticArt(false), ArcticArtInUse(false), AlphaImageFile{},
    Theater(false), Crushable(false), Bombable(true), RadarInvisible(false),
    Selectable(true), LegalTarget(true), Insignificant(false), Immune(false),
    IsLogic(false), AllowCellContent(true), Voxel(false), NewTheater(false),
    HasRadialIndicator(false), IgnoresFirestorm(false), UseLineTrail(false),
    LineTrailColor(128, 128, 128), LineTrailColorDecrement(16) {
    std::memcpy(ImageFile, ID, sizeof(ID));
    Array.AddItem(this);
}
ObjectTypeClass::~ObjectTypeClass() {
    Array.Remove(this);
    if (ImageAllocated) YRMemory::Deallocate(Image);
    Image = nullptr; ImageAllocated = false;
    const auto release = [](VoxelStruct& voxel) {
        GameDelete(voxel.VXL); voxel.VXL = nullptr;
        GameDelete(voxel.HVA); voxel.HVA = nullptr;
    };
    for (int i = 0; i < 18; ++i) { release(ChargerTurrets[i]); release(ChargerBarrels[i]); }
    // These members follow the voxel fields in the original class, so their
    // tables are released before the main voxel destructors (5F7400).
    VoxelTurretBarrelCache.Clear(); VoxelShadowCache.Clear();
    VoxelTurretWeaponCache.Clear(); VoxelMainCache.Clear();
    release(BarrelVoxel); release(TurretVoxel); release(MainVoxel);
}

SHPStruct* ObjectTypeClass::GetImage() const { return Image; }
HRESULT YRPP_STDCALL ObjectTypeClass::GetSizeMax(ULARGE_INTEGER* dest) {
    return AbstractClass::GetSizeMax(dest);
}

CoordStruct* ObjectTypeClass::vt_entry_6C(CoordStruct* dest, CoordStruct* source) const {
    *dest = *source; return dest;
}
