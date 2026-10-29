// ObjectTypeClass voxel loading: LoadVoxel (YR 0x5F8110) and its charger
// helpers 0x5F7A90, 0x5F7DB0 and 0x5F8080.
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/Memory.h"
#include "building_voxel.hpp"

namespace {
constexpr int ChargerVoxelCount = 18;
constexpr std::size_t PathCapacity = 260;

char* AppendText(char* cursor, const char* end, const char* text) {
    while (*text && cursor + 1 < end)
        *cursor++ = *text++;
    return cursor;
}

// "%s<suffix>[<index>]<extension>"; the index is omitted when it is zero.
void ComposePath(char (&path)[PathCapacity], const char* base, const char* suffix, int index, const char* extension) {
    const char* end = path + PathCapacity;
    char* cursor = AppendText(path, end, base);
    cursor = AppendText(cursor, end, suffix);
    if (index > 0) {
        char digits[12];
        int count = 0;
        for (unsigned value = static_cast<unsigned>(index); value; value /= 10)
            digits[count++] = static_cast<char>('0' + value % 10);
        while (count && cursor + 1 < end)
            *cursor++ = digits[--count];
    }
    cursor = AppendText(cursor, end, extension);
    *cursor = 0;
}

bool IsText(const char* text, const char* expected) {
    while (*text && *text == *expected) {
        ++text;
        ++expected;
    }
    return *text == *expected;
}

void ScaleToVoxel(const VoxelStruct& pair) { pair.HVA->Scale(pair.VXL->leaSectionTailer(0, 0)->HVAMultiplier); }

// Replaces one VXL/HVA pair. The previous objects are destroyed before the new
// constructors run; `failed` is the local failure flag of LoadVoxel.
void LoadStage(CCFileClass& vxlFile, const char* base, const char* suffix, VoxelStruct& pair, bool& failed) {
    char path[PathCapacity];
    GameDelete(pair.VXL);
    pair.VXL = GameCreate<VoxLib>(&vxlFile, false);
    if (!pair.VXL || pair.VXL->Initialized) failed = true;
    ComposePath(path, base, suffix, 0, ".HVA");
    CCFileClass hvaFile(path);
    GameDelete(pair.HVA);
    pair.HVA = GameCreate<MotLib>(&hvaFile);
    if (!pair.HVA || pair.HVA->LoadedFailed) failed = true;
    // The original scales with whatever VXL was constructed. A VXL that failed
    // to initialize has no section tables, so scaling is skipped for it.
    else if (pair.VXL && !pair.VXL->Initialized)
        ScaleToVoxel(pair);
}

// A stage whose VXL file may be absent without failing the whole load.
void LoadOptionalStage(const char* base, const char* suffix, VoxelStruct& pair, bool& failed) {
    char path[PathCapacity];
    ComposePath(path, base, suffix, 0, ".VXL");
    CCFileClass vxlFile(path);
    if (vxlFile.CCFileClass::Exists()) LoadStage(vxlFile, base, suffix, pair, failed);
}

// Unlike LoadVoxel, a missing or failed VXL leaves the previous HVA attached.
void LoadSpriteTurretStage(CCFileClass& vxlFile, const char* base, const char* suffix, VoxelStruct& pair) {
    if (!vxlFile.CCFileClass::Exists()) return;
    GameDelete(pair.VXL);
    pair.VXL = GameCreate<VoxLib>(&vxlFile, false);
    if (!pair.VXL || pair.VXL->Initialized) return;
    char path[PathCapacity];
    ComposePath(path, base, suffix, 0, ".HVA");
    CCFileClass hvaFile(path);
    GameDelete(pair.HVA);
    pair.HVA = GameCreate<MotLib>(&hvaFile);
    // 0x5F8CE0 scales even a failed HVA; its constructor owns that state.
    ScaleToVoxel(pair);
}

void ReleaseVoxel(VoxelStruct& pair) {
    GameDelete(pair.VXL);
    pair.VXL = nullptr;
    GameDelete(pair.HVA);
    pair.HVA = nullptr;
}
}

void ObjectTypeClass::LoadSpriteTurretVoxels() noexcept {
    try {
        if (WhatAmI() != AbstractType::UnitType || !GetImage()) return;
        if (!static_cast<TechnoTypeClass*>(this)->Turret) return;
        game::voxel_assets_changed();
        char path[PathCapacity];
        ComposePath(path, ImageFile, "TUR", 0, ".VXL");
        CCFileClass turretFile(path);
        LoadSpriteTurretStage(turretFile, ImageFile, "TUR", TurretVoxel);
        ComposePath(path, ImageFile, "BARL", 0, ".VXL");
        CCFileClass barrelFile(path);
        LoadSpriteTurretStage(barrelFile, ImageFile, "BARL", BarrelVoxel);
    } catch (...) {
        // Optional resource loading must not propagate host exceptions.
    }
}

void ObjectTypeClass::LoadVoxel() {
    // Host presentation caches borrow the voxel objects replaced below.
    game::voxel_assets_changed();
    bool failed = false;
    char path[PathCapacity];
    ComposePath(path, ImageFile, "", 0, ".VXL");
    CCFileClass mainFile(path);
    if (mainFile.CCFileClass::Exists())
        LoadStage(mainFile, ImageFile, "", MainVoxel, failed);
    else
        failed = true;

    const bool isUnit = WhatAmI() == AbstractType::UnitType;
    auto* techno = static_cast<TechnoTypeClass*>(this);
    const bool charger = isUnit && techno->HasMultipleTurrets() && !techno->IsGattling;
    // The original indexes the 18-entry charger tables with TurretCount unchecked.
    const bool chargerOverflow = charger && techno->TurretCount > ChargerVoxelCount;

    if (!isUnit || techno->Turret) {
        if (!charger) {
            LoadOptionalStage(ImageFile, "TUR", TurretVoxel, failed);
        } else if (chargerOverflow) {
            failed = true;
        } else {
            for (int i = 0; i < techno->TurretCount; ++i) {
                if (!LoadChargerTurretVoxel(ImageFile, i)) {
                    failed = true;
                    break;
                }
            }
        }
    } else {
        if (IsText(ID, "APC")) LoadOptionalStage(ImageFile, "W", TurretVoxel, failed);
        if (NoSpawnAlt) LoadOptionalStage(ImageFile, "WO", TurretVoxel, failed);
    }

    if (!isUnit || techno->Turret) {
        if (!charger) {
            LoadOptionalStage(ImageFile, "BARL", BarrelVoxel, failed);
        } else if (!chargerOverflow) {
            for (int i = 0; i < techno->TurretCount; ++i) {
                if (!LoadChargerBarrelVoxel(ImageFile, i)) {
                    failed = true;
                    break;
                }
            }
        }
    }

    if (failed) {
        ReleaseVoxel(MainVoxel);
        // Barrels are left as loaded, as in the original.
        if (charger)
            ReleaseChargerVoxels();
        else
            ReleaseVoxel(TurretVoxel);
        return;
    }

    int dimension = static_cast<unsigned char>(MainVoxel.VXL->leaSectionTailer(0, 0)->size_X);
    for (int i = 0; i < static_cast<int>(MainVoxel.VXL->CountHeaders); ++i) {
        const auto* tailer = MainVoxel.VXL->leaSectionTailer(i, 0);
        for (const char size : {tailer->size_X, tailer->size_Y, tailer->size_Z}) {
            const int value = static_cast<unsigned char>(size);
            if (dimension <= value) dimension = value;
        }
    }
    if (dimension <= 8) dimension = 8;
    MaxDimension = dimension;

    VoxelMainCache.Clear();
    VoxelTurretWeaponCache.Clear();
    VoxelShadowCache.Clear();
    VoxelTurretBarrelCache.Clear();
}

bool ObjectTypeClass::LoadChargerTurretVoxel(const char* name, int index) {
    char path[PathCapacity];
    ComposePath(path, name, "TUR", index, ".VXL");
    CCFileClass vxlFile(path);
    if (!vxlFile.CCFileClass::Exists()) return false;
    VoxelStruct& pair = ChargerTurrets[index];
    GameDelete(pair.VXL);
    pair.VXL = nullptr;
    pair.VXL = GameCreate<VoxLib>(&vxlFile, false);
    const bool vxlLoaded = pair.VXL && !pair.VXL->Initialized;
    ComposePath(path, name, "TUR", index, ".HVA");
    CCFileClass hvaFile(path);
    GameDelete(pair.HVA);
    pair.HVA = nullptr;
    pair.HVA = GameCreate<MotLib>(&hvaFile);
    const bool hvaLoaded = pair.HVA && !pair.HVA->LoadedFailed;
    if (vxlLoaded && hvaLoaded) {
        ScaleToVoxel(pair);
        return true;
    }
    ReleaseVoxel(pair);
    return false;
}

bool ObjectTypeClass::LoadChargerBarrelVoxel(const char* name, int index) {
    char path[PathCapacity];
    ComposePath(path, name, "BARL", index, ".VXL");
    CCFileClass vxlFile(path);
    if (!vxlFile.CCFileClass::Exists()) return true;
    VoxelStruct& pair = ChargerBarrels[index];
    // The original does not release a previous pair before replacing it.
    pair.VXL = GameCreate<VoxLib>(&vxlFile, false);
    const bool vxlLoaded = pair.VXL && !pair.VXL->Initialized;
    ComposePath(path, name, "BARL", index, ".HVA");
    CCFileClass hvaFile(path);
    pair.HVA = GameCreate<MotLib>(&hvaFile);
    const bool hvaLoaded = pair.HVA && !pair.HVA->LoadedFailed;
    if (vxlLoaded && hvaLoaded) {
        ScaleToVoxel(pair);
        return true;
    }
    // The original leaves both slots pointing at the destroyed objects, which
    // the following charger release would destroy a second time.
    ReleaseVoxel(pair);
    return false;
}

void ObjectTypeClass::ReleaseChargerVoxels() {
    for (int i = 0; i < ChargerVoxelCount; ++i) {
        ReleaseVoxel(ChargerTurrets[i]);
        ReleaseVoxel(ChargerBarrels[i]);
    }
}
