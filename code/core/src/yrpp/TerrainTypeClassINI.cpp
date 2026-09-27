// Supplied 0071DEA0. 0x00B0EDC0 foundation cells must be provided, not guessed.
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/RulesClass.h"
#include "type_resources.hpp"
bool TerrainTypeClass::LoadFromINI(CCINIClass* ini) {
    if (!ini) return false;
    const auto& resources = game::type_resources();
    if (!resources.terrain_foundation) {
        game::type_resource_result(game::TypeResourceStatus::unavailable); return false;
    }
    try {
        YRMemory::Deallocate(Image); Image = nullptr; ImageAllocated = false;
        if (!ObjectTypeClass::LoadFromINI(ini)) return false;
        if (Strength == -1) {
            if (!RulesClass::Instance) { game::type_resource_result(game::TypeResourceStatus::unavailable); return false; }
            Strength = RulesClass::Instance->TreeStrength;
        }
        IsVeinhole = ini->ReadBool(ID, "IsVeinhole", IsVeinhole);
        if (IsVeinhole) { IsLogic = false; LegalTarget = true; }
        WaterBound = ini->ReadBool(ID, "WaterBound", WaterBound);
        SpawnsTiberium = ini->ReadBool(ID, "SpawnsTiberium", SpawnsTiberium);
        IsFlammable = ini->ReadBool(ID, "IsFlammable", IsFlammable);
        Foundation = game::type_art_ini().ReadFoundation(ImageFile, "Foundation", Foundation);
        CellStruct* foundation = nullptr;
        if (!resources.terrain_foundation(resources.context, Foundation, foundation) || !foundation) {
            game::type_resource_result(game::TypeResourceStatus::unavailable); return false;
        }
        FoundationData = foundation;
        { // Theater and non-theater terrain both own their shape storage.
            char filename[260];
            if (!game::type_image_filename(*this, filename, sizeof(filename), false)) return false;
            if (!game::load_owned_type_shape(filename, Image)) return false;
        }
        if (Image) Image->GetColor(RadarColor, 0);
        RadarColor = ini->ReadColor(ID, "RadarColor", RadarColor);
        IsAnimated = ini->ReadBool(ID, "IsAnimated", IsAnimated);
        AnimationRate = ini->ReadInteger(ID, "AnimationRate", AnimationRate);
        AnimationProbability = static_cast<float>(ini->ReadDouble(ID, "AnimationProbability", AnimationProbability));
        TemperateOccupationBits = ini->ReadInteger(ID, "TemperateOccupationBits", TemperateOccupationBits);
        SnowOccupationBits = ini->ReadInteger(ID, "SnowOccupationBits", SnowOccupationBits);
        return true;
    } catch (...) { game::type_resource_result(game::TypeResourceStatus::failure); return false; }
}
