// Original resource queries: 41CFA0 and 5F9970 (delegates to 4103E0).
#include "yrpp/ObjectTypeClass.h"
// ObjectType::Occupy_List, YR 0x5F7640: shared one-cell footprint.
CellStruct* ObjectTypeClass::GetFoundationData(bool) const {
    static CellStruct footprint[]{{0, 0}, {0x7FFF, 0x7FFF}};
    return footprint;
}
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include <cstring>

namespace {
DynamicVectorClass<ObjectTypeClass*> object_types;
}
DynamicVectorClass<ObjectTypeClass*>& ObjectTypeClass::Array = object_types;

// RA1's ObjectType has a different fixed-heap design. These YR fields and the
// dynamic registration/ownership order are calibrated at 5F7090/5F7400.
ObjectTypeClass::ObjectTypeClass(const char* id) noexcept
    : AbstractTypeClass(id), RadialColor(0, 0, 0), Armor(static_cast<::Armor>(0)), Strength(0), Image(nullptr),
      ImageAllocated(false), AlphaImage(nullptr), MainVoxel{}, TurretVoxel{}, BarrelVoxel{}, ChargerTurrets{},
      ChargerBarrels{}, NoSpawnAlt(false), MaxDimension(0), CrushSound(-1), AmbientSound(-1), ImageFile{},
      AlternateArcticArt(false), ArcticArtInUse(false), AlphaImageFile{}, Theater(false), Crushable(false),
      Bombable(true), RadarInvisible(false), Selectable(true), LegalTarget(true), Insignificant(false), Immune(false),
      IsLogic(false), AllowCellContent(true), Voxel(false), NewTheater(false), HasRadialIndicator(false),
      IgnoresFirestorm(false), UseLineTrail(false), LineTrailColor(128, 128, 128), LineTrailColorDecrement(16) {
    std::memcpy(ImageFile, ID, sizeof(ID));
    Array.AddItem(this);
}
ObjectTypeClass::~ObjectTypeClass() {
    Array.Remove(this);
    if (ImageAllocated) YRMemory::Deallocate(Image);
    Image = nullptr;
    ImageAllocated = false;
    const auto release = [](VoxelStruct& voxel) {
        GameDelete(voxel.VXL);
        voxel.VXL = nullptr;
        GameDelete(voxel.HVA);
        voxel.HVA = nullptr;
    };
    for (int i = 0; i < 18; ++i) {
        release(ChargerTurrets[i]);
        release(ChargerBarrels[i]);
    }
    // These members follow the voxel fields in the original class, so their
    // tables are released before the main voxel destructors (5F7400).
    VoxelTurretBarrelCache.Clear();
    VoxelShadowCache.Clear();
    VoxelTurretWeaponCache.Clear();
    VoxelMainCache.Clear();
    release(BarrelVoxel);
    release(TurretVoxel);
    release(MainVoxel);
}

SHPStruct* ObjectTypeClass::GetImage() const { return Image; }

namespace {
// Ownership flags 0x00AC1488..0x00AC148B, in PIPBRD/PIPS/PIPS2/TALKBUBL order.
bool one_time_owned[4]{};
SHPStruct** one_time_shape(int index) {
    SHPStruct** shapes[]{&FileSystem::PIPBRD_SHP, &FileSystem::PIPS_SHP, &FileSystem::PIPS2_SHP,
                         &FileSystem::TALKBUBL_SHP};
    return shapes[index];
}
}

// 0x005F76B0. Each shape is read into owned memory first and only falls back
// to the name cache when that read fails.
void ObjectTypeClass::One_Time() {
    static const char* const names[]{"PIPBRD.SHP", "PIPS.SHP", "PIPS2.SHP", "TALKBUBL.SHP"};
    for (int i = 0; i < 4; ++i) {
        auto& shape = *one_time_shape(i);
        if (shape) continue;
        CCFileClass file(names[i]);
        shape = static_cast<SHPStruct*>(file.ReadWholeFile());
        one_time_owned[i] = shape != nullptr;
        if (!shape) shape = static_cast<SHPStruct*>(FileSystem::LoadFile(names[i], false));
    }
}

// 0x005F77F0 runs only at process exit and releases each owned buffer as an
// array of 36-byte objects with a count prefix, which a raw file buffer is
// not; the buffer is freed as the raw allocation it is. The original leaves
// the globals set, while the host clears every one, owned or name-cache, so
// the next resource owner loads its own shapes.
void ObjectTypeClass::Free_One_Time() {
    for (int i = 0; i < 4; ++i) {
        auto& shape = *one_time_shape(i);
        if (one_time_owned[i]) YRMemory::Deallocate(shape);
        one_time_owned[i] = false;
        shape = nullptr;
    }
}
HRESULT YRPP_STDCALL ObjectTypeClass::GetSizeMax(ULARGE_INTEGER* dest) { return AbstractClass::GetSizeMax(dest); }

CoordStruct* ObjectTypeClass::vt_entry_6C(CoordStruct* dest, CoordStruct* source) const {
    *dest = *source;
    return dest;
}
