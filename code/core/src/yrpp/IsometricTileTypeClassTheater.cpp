// YRpp 9402d7da original type/registry model, catalog control from fixed YR
// 545535..546CB9. EA's TemplateTypeClass predates this INI/TMP catalog;
// no same-class implementation exists in the pinned EA/XCC snapshots.
// Palette, slope-Z and shadow-SHP preparation in 545150 remain separate.
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/CCINIClass.h"
#include "map_runtime.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
#include <stdexcept>

namespace {
struct Field { const char* key; int* value; int initial; bool is_set; };
const Field fields[]{
#define YR_TILE_FIELD(name, address, initial, is_set) {#name, &IsometricTileTypeClass::name, initial, is_set},
#include "IsometricTileTypeClassTheaterData.inc"
#undef YR_TILE_FIELD
};
template<class T> void reserve_one(DynamicVectorClass<T*>& array) {
    if (array.Count < array.Capacity) return;
    if (array.Count > 1048576 || !array.SetCapacity(array.Count + 16)) throw std::bad_alloc();
}
IsometricTileTypeClass* create_tile(int index, const char* name, bool variant) {
    // All registry growth occurs before the existing noexcept constructors.
    // Their original AddItem calls then cannot allocate or strand a partial type.
    reserve_one(AbstractTypeClass::Array); reserve_one(ObjectTypeClass::Array);
    reserve_one(IsometricTileTypeClass::AllTypes);
    if (!variant) reserve_one(IsometricTileTypeClass::Array);
    void* storage = YRMemory::Allocate(sizeof(IsometricTileTypeClass));
    if (!storage) throw std::bad_alloc();
    return ::new (storage) IsometricTileTypeClass(index,191,0,name,variant ? 1 : 0);
}
bool exists(const char* name) {
    if (auto query = game::map_runtime().tile_file_exists) return query(name);
    CCFileClass file(name); return file.Exists();
}
bool resolve(const char* stem, const Theater& theater, int non_marble, char (&filename)[160]) {
    std::snprintf(filename,sizeof(filename),"%s.%s",stem,theater.Extension);
    if (exists(filename)) return true;
    // NonMarbleMadness=0 deliberately suppresses the fallback sequence.
    if (!non_marble) return false;
    for (const char* suffix : {theater.MMExtension,"TEM","URB"}) {
        std::snprintf(filename,sizeof(filename),"%s.%s",stem,suffix);
        if (exists(filename)) return true;
    }
    return false;
}
void animation(CCINIClass& ini, const char* section, int number, IsometricTileTypeClass& tile) {
    if (!ini.GetSection(section)) return;
    char key[32], id[128]; std::snprintf(key,sizeof(key),"Tile%02dAnim",number);
    if (!ini.ReadString(section,key,"",id,sizeof(id))) return;
    // 546538 calls FindOrAllocate, not Find: catalog animations may precede
    // the later theater art pass. Reserve base registries before construction.
    if (!AnimTypeClass::Find(id) && _strcmpi(id,"none") && _strcmpi(id,"<none>")) {
        reserve_one(AbstractTypeClass::Array); reserve_one(ObjectTypeClass::Array);
        reserve_one(AnimTypeClass::Array); reserve_one(AbstractClass::TypeExpirationListeners);
    }
    auto* type = AnimTypeClass::FindOrAllocate(id);
    if (!type) return; // Original leaves fields unchanged for none/allocation failure.
    tile.TileAnimIndex = type->GetArrayIndex();
    const struct { const char* suffix; int* value; } values[]{
        {"XOffset",&tile.TileXOffset},{"YOffset",&tile.TileYOffset},
        {"AttachesTo",&tile.TileAttachesTo},{"ZAdjust",&tile.TileZAdjust}};
    for (const auto& value : values) {
        std::snprintf(key,sizeof(key),"Tile%02d%s",number,value.suffix);
        *value.value = ini.ReadInteger(section,key,*value.value);
    }
}
}
void IsometricTileTypeClass::ClearTileSetCatalog() noexcept {
    while (Array.Count) {
        auto* tile = Array[Array.Count-1];
        if (tile) GameDelete(tile); else Array.RemoveItem(Array.Count-1);
    }
    Array.Clear();
    for (auto* insertion : TileInsertions) YRMemory::Deallocate(insertion);
    TileInsertions.Clear();
    std::fill_n(TileSetStarts,256,0); TileSetCount = 0;
    std::fill_n(ShadowTileSets,5,0);
    for (const auto& field : fields) *field.value = field.initial;
}
bool IsometricTileTypeClass::LoadTileSetCatalog(CCINIClass& ini, TheaterType theater_id) noexcept {
    ClearTileSetCatalog();
    ini.Reset();
    // INI caches the section pointer identity. Never leave a borrowed stack
    // section name in the caller's INI after this catalog operation.
    struct ResetINI { CCINIClass& ini; ~ResetINI() { ini.Reset(); } } reset{ini};
    if (theater_id < TheaterType::Temperate || theater_id > TheaterType::Lunar) return false;
    try {
        const auto& theater = Theater::GetTheater(theater_id);
        int requested[std::size(fields)]{};
        for (std::size_t i=0;i<std::size(fields);++i) {
            const auto& field = fields[i];
            requested[i] = ini.ReadInteger("General",field.key,field.initial);
            if (!field.is_set) *field.value = requested[i];
        }
        int original_count=0, shadow_count=0;
        for (int set=0;;++set) {
            if (set >= 256) throw std::runtime_error("Too many original tile sets");
            const int start = Array.Count;
            TileSetStarts[set] = start; TileSetCount = set+1;
            for (std::size_t i=0;i<std::size(fields);++i)
                if (fields[i].is_set && requested[i]==set) *fields[i].value = start;
            char section[32]; std::snprintf(section,sizeof(section),"TileSet%04d",set);
            ini.Reset(); // 545FBD: the same stack buffer now names another set.
            const int count = ini.ReadInteger(section,"TilesInSet",-1);
            if (count == -1) break;
            if (count < 0 || count > 1048576 - Array.Count) throw std::runtime_error("Invalid tile-set size");
            const int last = ini.ReadInteger(section,"LastTilesInSet",-1);
            if (last < -1 || last > 1048576-original_count) throw std::runtime_error("Invalid tile insertion");
            if (last != -1 && last != count) {
                reserve_one(TileInsertions);
                auto* insertion = static_cast<TileInsertType*>(YRMemory::Allocate(sizeof(TileInsertType)));
                if (!insertion) throw std::bad_alloc();
                *insertion = {original_count+last,count-last};
                TileInsertions.AddItem(insertion);
                original_count += last;
            } else original_count += count;
            char set_name[64], base[64];
            ini.ReadString(section,"SetName","No Name",set_name,sizeof(set_name));
            ini.ReadString(section,"FileName","TILE",base,sizeof(base));
            const int marble=ini.ReadInteger(section,"MarbleMadness",0xffff);
            const int non_marble=ini.ReadInteger(section,"NonMarbleMadness",0xffff);
            const bool morphable=ini.ReadBool(section,"Morphable",false);
            const bool place=ini.ReadBool(section,"AllowToPlace",true);
            const bool burrow=ini.ReadBool(section,"AllowBurrowing",true);
            const bool tiberium=ini.ReadBool(section,"AllowTiberium",false);
            const bool rmg=ini.ReadBool(section,"RequiredForRMG",false);
            const int snow=ini.ReadInteger(section,"ToSnowTheater",-1);
            const int temperate=ini.ReadInteger(section,"ToTemperateTheater",-1);
            const bool shadow=ini.ReadBool(section,"ShadowCaster",false);
            if (shadow) {
                if (shadow_count>=5) throw std::runtime_error("Too many shadow tile sets");
                ShadowTileSets[shadow_count++] = start;
            }
            const bool shadow_tiles=shadow && ini.ReadInteger(section,"ShadowTiles",0)!=0;
            for (int number=1;number<=count;++number) {
                IsometricTileTypeClass* first=nullptr;
                IsometricTileTypeClass* previous=nullptr;
                int variant_count=0;
                const int index=Array.Count;
                for (int variant=0;;++variant) {
                    if (variant>=127) throw std::runtime_error("Tile variant count exceeds signed byte");
                    char stem[96],filename[160];
                    if (variant) std::snprintf(stem,sizeof(stem),"%s%02d%c",base,number,'a'+variant-1);
                    else std::snprintf(stem,sizeof(stem),"%s%02d",base,number);
                    // The base registers even when its file is absent. Variants
                    // register only after a successful original Exists lookup.
                    auto* tile=variant ? nullptr : create_tile(index,stem,false);
                    bool present=false;
                    if (variant) {
                        present=resolve(stem,theater,non_marble,filename);
                        if (!present) break;
                        tile=create_tile(index,stem,true);
                    }
                    if (previous) previous->NextVariant=tile; else first=tile;
                    previous=tile; ++variant_count;
                    std::snprintf(tile->Name,sizeof(tile->Name),"%.28s %02d",set_name,number);
                    tile->MarbleMadnessTile=marble; tile->NonMarbleMadnessTile=non_marble;
                    tile->Morphable=morphable; tile->AllowToPlace=place;
                    tile->AllowBurrowing=burrow; tile->AllowTiberium=tiberium;
                    tile->RequiredByRMG=rmg; tile->ShadowCaster=shadow_tiles;
                    tile->unk_2A0=start; tile->unk_2F4=true;
                    if (!variant) {
                        tile->ToSnowTheater=snow; tile->ToTemperateTheater=temperate;
                        animation(ini,set_name,number,*tile);
                        present=resolve(stem,theater,non_marble,filename);
                    }
                    if (present) std::strncpy(tile->FileName,filename,sizeof(tile->FileName));
                    if (!present) break; // Original retains the missing base type.
                }
                for (auto* tile=first;tile;tile=tile->NextVariant) tile->unk_2F0=variant_count--;
            }
        }
        for (auto* tile : Array) {
            const int offset=tile->ArrayIndex-static_cast<int>(tile->unk_2A0);
            for (int* value : {&tile->MarbleMadnessTile,&tile->NonMarbleMadnessTile}) {
                if (*value==0xffff) continue;
                if (*value<0 || *value>=TileSetCount) throw std::runtime_error("Invalid marble tile set");
                *value=TileSetStarts[*value]+offset;
            }
        }
        if (theater_id==TheaterType::Lunar) {
            ShorePieces=WaterSet=CliffSet=WaterCliffs=WaterBridge=BridgeSet=WoodBridgeSet=-1;
        }
        if (!Array.Count) throw std::runtime_error("Empty tile catalog");
        return true;
    } catch (...) { ClearTileSetCatalog(); return false; }
}
