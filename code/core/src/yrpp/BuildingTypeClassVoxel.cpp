// BuildingTypeClass::LoadTurretBarrelVoxels, YR 0x45FA90.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/Memory.h"
#include "building_voxel.hpp"

namespace {
constexpr std::size_t NameCapacity = 128;
constexpr std::size_t PathCapacity = 260;
// The original upper-cases from the fifth character, leaving a short prefix as is.
constexpr std::size_t UppercaseStart = 4;

void CopyText(char* destination, std::size_t capacity, const char* source, std::size_t limit) {
    std::size_t used = 0;
    while (used + 1 < capacity && used < limit && source[used]) {
        destination[used] = source[used];
        ++used;
    }
    destination[used] = 0;
}

void ComposePath(char (&path)[PathCapacity], const char* name, const char* extension) {
    char* cursor = path;
    const char* end = path + PathCapacity;
    for (const char* text : {name, extension})
        while (*text && cursor + 1 < end)
            *cursor++ = *text++;
    *cursor = 0;
}

void UppercaseFrom(char* text, std::size_t start) {
    std::size_t index = 0;
    while (text[index] && index < start)
        ++index;
    for (; text[index]; ++index)
        if (text[index] >= 'a' && text[index] <= 'z') text[index] = static_cast<char>(text[index] - 'a' + 'A');
}

char* FindTurretTag(char* text) {
    for (; *text; ++text)
        if (text[0] == 'T' && text[1] == 'U' && text[2] == 'R') return text;
    return nullptr;
}

// Replaces one VXL/HVA pair. A VXL whose construction failed has no section
// tables, so it is not used for scaling.
void LoadPair(CCFileClass& vxlFile, const char* name, VoxelStruct& pair) {
    char path[PathCapacity];
    GameDelete(pair.VXL);
    pair.VXL = GameCreate<VoxLib>(&vxlFile, false);
    ComposePath(path, name, ".HVA");
    CCFileClass hvaFile(path);
    GameDelete(pair.HVA);
    pair.HVA = GameCreate<MotLib>(&hvaFile);
    if (pair.HVA && !pair.HVA->LoadedFailed && pair.VXL && !pair.VXL->Initialized)
        pair.HVA->Scale(pair.VXL->leaSectionTailer(0, 0)->HVAMultiplier);
}
}

void BuildingTypeClass::LoadTurretBarrelVoxels() {
    if (!TurretAnimIsVoxel && !BarrelAnimIsVoxel) return;
    // Host presentation caches borrow the voxel objects replaced below.
    game::voxel_assets_changed();
    char name[NameCapacity];
    char path[PathCapacity];
    const auto& turretAnim = GetBuildingAnim(BuildingAnimSlot::Turret);
    CopyText(name, NameCapacity, turretAnim.Anim, sizeof(turretAnim.Anim));
    ComposePath(path, name, ".VXL");
    UppercaseFrom(path, UppercaseStart);
    if (FindTurretTag(path + UppercaseStart)) {
        CCFileClass turretFile(path);
        if (turretFile.CCFileClass::Exists()) LoadPair(turretFile, name, TurretVoxel);
        UppercaseFrom(name, UppercaseStart);
        if (char* tag = FindTurretTag(name + UppercaseStart)) {
            tag[0] = 'B';
            tag[1] = 'A';
            tag[2] = 'R';
            tag[3] = 'L';
            tag[4] = 0;
        }
    } else if (BarrelAnimIsVoxel) {
        CopyText(name, NameCapacity, VoxelBarrelFile, sizeof(VoxelBarrelFile));
    }
    ComposePath(path, name, ".VXL");
    CCFileClass barrelFile(path);
    if (barrelFile.CCFileClass::Exists()) LoadPair(barrelFile, name, BarrelVoxel);
}
