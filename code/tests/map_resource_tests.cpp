#include "support/test_support.hpp"
// Cells now use normal construction. The other type fixtures only populate
// original fields; their noinit paths do not certify instance lifecycles.
#include "api/filesystem.hpp"
#include "yrpp/DisplayClass.h"
#include "yrpp/IsometricTileClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/SmudgeClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/PreviewClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/Pipes.h"
#include "yrpp/Straws.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AbstractClass) == 0x24 && sizeof(ObjectTypeClass) == 0x294);
static_assert(offsetof(AbstractClass, UniqueID) == 0x10);
static_assert(offsetof(AbstractTypeClass, ID) == 0x24);
static_assert(offsetof(ObjectTypeClass, Image) == 0xa4);
static_assert(offsetof(IsometricTileTypeClass, ArrayIndex) == 0x294);
static_assert(offsetof(CellClass, MapCoords) == 0x24);
static_assert(offsetof(CellClass, Level) == 0x11b && offsetof(CellClass, SlopeIndex) == 0x11c);
static_assert(offsetof(MapClass, MapRect) == 0xec && offsetof(MapClass, Cells) == 0x138);
static_assert(offsetof(MapClass, CellIterator_NextCell) == 0x118);
static_assert(offsetof(ScenarioClass, Waypoints) == 0x632 && offsetof(ScenarioClass, Theater) == 0x1258);
static_assert(offsetof(ScenarioClass, StartX) == 0x112c && offsetof(ScenarioClass, NumCoopHumanStartSpots) == 0x11e4);
static_assert(sizeof(CSFHeader) == 24 && sizeof(CSFLabel) == 40 && sizeof(CSFString) == 0x208);
static_assert(offsetof(CSFString, Text) == 4 && sizeof(IsometricTileTypeClass::TileInsertType) == 8);
static_assert(offsetof(CellClass, IsoTileTypeIndex) == 0x38 && offsetof(CellClass, Height) == 0x11a);
#endif

namespace {
std::filesystem::path fixture_dir = RA2_TEST_FIXTURE_DIR;

template<class T> struct ResourceFixture final : T {
    ResourceFixture() : T(noinit_t{}) {
        // noinit fixtures must not pass an indeterminate owned trail pointer
        // into the now-real ObjectClass destructor.
        if constexpr(std::is_base_of_v<ObjectClass,T>){
            this->LineTrailer=nullptr;this->LastLayer=Layer::Ground;this->InLimbo=true;
        }
    }
};
template<> struct ResourceFixture<CellClass> final : CellClass {
    ResourceFixture() : CellClass() {}
};
struct DisplayFixture final : DisplayClass {
    // The resource-only fixture borrows stack Cells in an owned pointer table.
    // Dispose that table before the real Map destructor sees owned live cells.
    ~DisplayFixture() override { Cells.Clear(); }
    bool SetCursor(MouseCursorType, bool) override { throw std::logic_error("UI excluded"); }
    bool UpdateCursor(MouseCursorType, bool) override { throw std::logic_error("UI excluded"); }
    bool RestoreCursor() override { throw std::logic_error("UI excluded"); }
    void UpdateCursorMinimapState(bool) override { throw std::logic_error("UI excluded"); }
    MouseCursorType GetLastMouseCursor() override { throw std::logic_error("UI excluded"); }
};

template<class T> void identity(DWORD first, DWORD second = 0x11d20634u, DWORD third = 0x6000a4acu, DWORD fourth = 0xb55b0508u) {
    ResourceFixture<T> value;
    value.UniqueID = 0xf1234567u;
    CLSID id{};
    EXPECT_TRUE((value.GetClassID(nullptr) == static_cast<HRESULT>(0x80004003u))) << "CLSID null destination";
    EXPECT_TRUE((value.GetClassID(&id) == 0)) << "CLSID status";
    DWORD parts[4]; std::memcpy(parts, &id, sizeof(parts));
    EXPECT_TRUE((parts[0] == first && parts[1] == second && parts[2] == third && parts[3] == fourth)) << "CLSID bytes from original vtable entry";
    IRTTITypeInfo* rtti = &value;
    EXPECT_TRUE((rtti->What_Am_I() == T::AbsID && static_cast<DWORD>(rtti->Fetch_ID()) == value.UniqueID)) << "secondary RTTI interface dispatch";
    IPersistStream* persist = &value;
    value.Dirty = false; EXPECT_TRUE((persist->IsDirty() == 1)) << "clean object returns S_FALSE";
    value.Dirty = true; EXPECT_TRUE((persist->IsDirty() == 0)) << "dirty object returns S_OK";
    ULARGE_INTEGER size{};
    EXPECT_TRUE((persist->GetSizeMax(nullptr) == static_cast<HRESULT>(0x80004003u))) << "size null destination";
    EXPECT_TRUE((persist->GetSizeMax(&size) == 0 && size.QuadPart == sizeof(T) + 4u)) << "native serialized size bound";
    if constexpr (std::is_base_of_v<ObjectClass, T>) {
        using Type = std::remove_pointer_t<decltype(value.Type)>;
        ResourceFixture<Type> type;
        value.Type = &type;
        ObjectClass* object = &value;
        EXPECT_TRUE((object->GetType() == &type)) << "original object-to-resource virtual dispatch";
    }
    if constexpr (std::is_base_of_v<ObjectTypeClass, T>) {
        value.ArrayIndex = 17;
        AbstractClass* base = &value;
        EXPECT_TRUE((base->GetArrayIndex() == 17)) << "original array-index virtual override";
        std::strcpy(value.ID, "RESOURCE");
        EXPECT_TRUE((std::strcmp(value.get_ID(), "RESOURCE") == 0)) << "resource identifier";
    }
}
struct TypeLifecycleFixture final : AbstractTypeClass {
    explicit TypeLifecycleFixture(const char* id) : AbstractTypeClass(id) {}
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override { throw std::logic_error("fixture identity"); }
    HRESULT YRPP_STDCALL Load(IStream*) override { throw std::logic_error("stream load excluded"); }
    HRESULT YRPP_STDCALL Save(IStream*, BOOL) override { throw std::logic_error("stream save excluded"); }
    AbstractType WhatAmI() const override { return AbstractType::Abstract; }
    int Size() const override { return sizeof(*this); }
};
void type_lifecycle() {
    auto& array = AbstractTypeClass::Array;
    EXPECT_TRUE((array.Count == 0)) << "base resource registry starts empty";
    {
        auto first = std::make_unique<TypeLifecycleFixture>("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TypeLifecycleFixture second("short");
        TypeLifecycleFixture generated(nullptr);
        EXPECT_TRUE((array.Count == 3 && array[0] == first.get() && array[1] == &second && array[2] == &generated)) << "resource registration order";
        EXPECT_TRUE((std::strcmp(first->get_ID(), "ABCDEFGHIJKLMNOPQRSTUVWX") == 0 &&
            std::strcmp(first->Name, "ABCDEFGHIJKLMNOPQRSTUVWX") == 0)) << "24-byte ID and separate terminator";
        EXPECT_TRUE((second.UniqueID == 0xffffffffu && second.RefCount == 0 && !second.Dirty &&
            second.UINameLabel[0] == 0 && second.UIName[0] == 0)) << "base constructor defaults";
        EXPECT_TRUE((std::strlen(generated.ID) == 8 && std::strcmp(generated.Name, generated.ID) == 0)) << "generated ID is copied to the name";
        first.reset();
        EXPECT_TRUE((array.Count == 2 && array[0] == &second && array[1] == &generated)) << "middle-independent removal preserves registry order";
    }
    EXPECT_TRUE((array.Count == 0)) << "resource destructor unregisters every type";
}
void theaters() {
    constexpr const char* ids[] = {"TEMPERATE", "SNOW", "URBAN", "DESERT", "NEWURBAN", "LUNAR"};
    constexpr const char* extensions[] = {"TEM", "SNO", "URB", "DES", "UBN", "LUN"};
    for (int i = 0; i < 6; ++i) {
        auto type = static_cast<TheaterType>(i);
        EXPECT_TRUE((std::strcmp(Theater::Get(type)->ID, ids[i]) == 0)) << "theater IDs";
        EXPECT_TRUE((std::strcmp(Theater::GetTheater(type).Extension, extensions[i]) == 0)) << "theater extensions";
        EXPECT_TRUE((Theater::FindIndex(ids[i]) == i)) << "theater lookup";
    }
    EXPECT_TRUE((Theater::FindIndex("sNoW") == 1 && Theater::FindIndex("absent") == -1)) << "case-insensitive theater lookup";
    EXPECT_TRUE((&ScenarioClass::LastTheater == &Theater::LastTheater)) << "shared last-theater state";
    EXPECT_TRUE((GroundType::GetLandTypeFromName("tUnNeL") == LandType::Tunnel)) << "ground name lookup";
    EXPECT_TRUE((GroundType::GetLandTypeFromName(nullptr) == LandType::None &&
        GroundType::GetLandTypeFromName("<none>") == LandType::None)) << "missing ground type";
}
void cell_geometry() {
    ResourceFixture<CellClass> cell;
    cell.Flags = static_cast<CellFlags>(0);
    std::ifstream reference(fixture_dir / "map_floor_reference.txt");
    EXPECT_TRUE((bool(reference))) << "open original floor samples";
    int level, slope, x, y, expected, count = 0;
    while (reference >> level >> slope >> x >> y >> expected) {
        cell.Level = static_cast<char>(level); cell.SlopeIndex = static_cast<BYTE>(slope);
        EXPECT_TRUE((cell.GetFloorHeight({x, y}) == expected)) << "original floor sample mismatch";
        ++count;
    }
    EXPECT_TRUE((reference.eof() && count == 672)) << "complete original floor sample set";
    cell.SetMapCoords({-2, 7}); cell.Level = 3; cell.SlopeIndex = 1;
    const CoordStruct coords = cell.GetCoords();
    EXPECT_TRUE((coords.X == -384 && coords.Y == 1920 && coords.Z == 364)) << "signed map coordinates and slope center";
    cell.Flags = CellFlags::BridgeHead;
    EXPECT_TRUE((cell.GetCenterCoords().Z == coords.Z + 416)) << "bridge center height";
}
void map_geometry() {
    DisplayFixture map;
    EXPECT_TRUE((map.Cells.SetCapacity(MapClass::MaxCells, nullptr))) << "allocate map slot fixture";
    ResourceFixture<CellClass> cell;
    for (int i = 0; i < MapClass::MaxCells; ++i) map.Cells.Items[i] = &cell;
    std::ifstream reference(fixture_dir / "map_iterator_reference.txt");
    EXPECT_TRUE((bool(reference))) << "open original iterator samples";
    int width, step, before, result, next_x, next_y, remaining, after, count = 0;
    while (reference >> width >> step >> before >> result >> next_x >> next_y >> remaining >> after) {
        if (!step) { map.MapRect = {0, 0, width, 3}; map.CellIteratorReset(); }
        EXPECT_TRUE((map.CellIterator_NextCell == map.Cells.Items + before && result == before)) << "original diamond iterator slot";
        EXPECT_TRUE((map.CellIteratorNext() == &cell)) << "iterator returns stored cell";
        EXPECT_TRUE((map.CellIterator_NextX == next_x && map.CellIterator_NextY == next_y &&
            map.CellIterator_CurrentY == remaining && map.CellIterator_NextCell == map.Cells.Items + after)) << "original diamond iterator state";
        ++count;
    }
    EXPECT_TRUE((reference.eof() && count == 69)) << "complete iterator reference";
    EXPECT_TRUE((MapClass::GetCellIndex({-1, 1}) == 511)) << "original flattened indexing";
    EXPECT_TRUE((MapClass::GetCellIndex({0, -1}) == -512 && !map.TryGetCellAt(CellStruct{0, -1}))) << "negative cell index";
    EXPECT_TRUE((!map.TryGetCellAt(CellStruct{0, 512}))) << "out-of-range cell index";
    CellStruct foundation[] = {{-2, -1}, {1, 4}, {0x7fff, 0x7fff}};
    EXPECT_TRUE((map.FoundationBoundsSize(foundation) == CellStruct{4, 6})) << "foundation bounds";
    map.FoundationBoundsSize(foundation[0], foundation);
    EXPECT_TRUE((foundation[0] == CellStruct{4, 6})) << "foundation output may alias input";
    EXPECT_TRUE((map.FoundationBoundsSize(nullptr) == CellStruct{0, 0})) << "null foundation";
    CellStruct empty[] = {{0x7fff, 0x7fff}};
    EXPECT_TRUE((map.FoundationBoundsSize(empty) == CellStruct{1, 1})) << "original empty foundation minimum";
}
void map_pack() {
    DisplayFixture map;
    EXPECT_TRUE((map.Cells.SetCapacity(MapClass::MaxCells, nullptr))) << "allocate IsoMapPack slots";
    for (int i = 0; i < map.Cells.Capacity; ++i) map.Cells.Items[i] = nullptr;
    ResourceFixture<CellClass> first, empty, outside;
    first.MapCoords = {2, 3}; first.IsoTileTypeIndex = 42; first.Height = 7; first.Level = 2; first.IsIceGrowthAllowed = 1;
    empty.MapCoords = {3, 2}; empty.IsoTileTypeIndex = 0xffff; empty.Height = 0; empty.Level = -1; empty.IsIceGrowthAllowed = 0;
    outside.MapCoords = {1, 1}; outside.IsoTileTypeIndex = 18; outside.Height = 1; outside.Level = 5; outside.IsIceGrowthAllowed = 0;
    map.MapRect = {0, 0, 3, 2};
    map.Cells.Items[MapClass::GetCellIndex(first.MapCoords)] = &first;
    map.Cells.Items[MapClass::GetCellIndex(empty.MapCoords)] = &empty;
    map.Cells.Items[MapClass::GetCellIndex(outside.MapCoords)] = &outside;
    unsigned char packed[1024]{}; BufferPipe destination(packed, sizeof(packed));
    const int size = map.WriteIsoMapPack5(destination);
    EXPECT_TRUE((size > 0 && size == destination.Index)) << "IsoMapPack output byte count";
    BufferStraw framed(packed, size); LZOStraw decoder(1, 8192); decoder.Get_From(framed);
    unsigned char bytes[20]{};
    const unsigned char expected[]{2, 0, 3, 0, 42, 0, 0, 0, 7, 2, 1, 0, 0, 0, 0};
    EXPECT_TRUE((decoder.Get(bytes, sizeof(bytes)) == sizeof(expected) && std::memcmp(bytes, expected, sizeof(expected)) == 0)) << "original record layout, diamond filter, signed level and terminator";
    CCINIClass ini;
    EXPECT_TRUE((ini.WriteUUBlock("IsoMapPack5", packed, size) > 0)) << "existing INI UU writer";
    unsigned char reread[1024]{};
    EXPECT_TRUE((ini.ReadUUBlock("IsoMapPack5", reread, sizeof(reread)) == size && std::memcmp(packed, reread, size) == 0)) << "existing INI UU reader";
    IsometricTileTypeClass::TileInsertType at40{40, 2}, at44{44, 6};
    auto& insertions = IsometricTileTypeClass::TileInsertions;
    EXPECT_TRUE((insertions.AddItem(&at40) && insertions.AddItem(&at44))) << "original tile insertion records";
    struct Clear { ~Clear() { IsometricTileTypeClass::TileInsertions.Clear(); } } clear;
    EXPECT_TRUE((IsometricTileTypeClass::ConvertTileIndex(0xffff) == 0xffff && IsometricTileTypeClass::ConvertTileIndex(44) == 52)) << "tile insertion accumulation and clear-tile sentinel";
    first.IsoTileTypeIndex = -1; first.Height = first.Level = first.IsIceGrowthAllowed = 0;
    BufferStraw source(reread, size);
    EXPECT_TRUE((map.ReadIsoMapPack5(source))) << "read IsoMapPack into original cells";
    EXPECT_TRUE((first.IsoTileTypeIndex == 44 && first.Height == 7 && first.Level == 2 && first.IsIceGrowthAllowed == 1)) << "tile thresholds compare original index, not running result";
    BufferStraw truncated(reread, size - 1);
    EXPECT_TRUE((!map.ReadIsoMapPack5(truncated))) << "truncated IsoMapPack fails";
    unsigned char no_end[128]; BufferPipe target(no_end, sizeof(no_end)); LZOPipe writer(0, 8192); writer.Put_To(target);
    writer.Put(expected, 11); writer.Flush();
    BufferStraw unterminated(no_end, target.Index);
    EXPECT_TRUE((!map.ReadIsoMapPack5(unterminated))) << "record stream requires terminator";

    unsigned char old_record[]{2, 0, 3, 0, 42, 0, 0, 0, 7, 2, 0, 0, 0, 0};
    for (int version : {2, 3, 4}) {
        unsigned char old_packed[128]; int old_size = sizeof(old_record);
        BufferPipe out(old_packed, sizeof(old_packed));
        if (version == 2) { LCWPipe pipe(0, 8192); pipe.Put_To(out); pipe.Put(old_record, sizeof(old_record)); pipe.Flush(); old_size = out.Index; }
        else if (version == 4) { LZOPipe pipe(0, 8192); pipe.Put_To(out); pipe.Put(old_record, sizeof(old_record)); pipe.Flush(); old_size = out.Index; }
        else std::memcpy(old_packed, old_record, sizeof(old_record));
        first.IsoTileTypeIndex = -1; first.IsIceGrowthAllowed = 9;
        BufferStraw old_source(old_packed, old_size);
        const bool ok = version == 2 ? map.ReadIsoMapPack2(old_source) : version == 3 ? map.ReadIsoMapPack3(old_source) : map.ReadIsoMapPack4(old_source);
        EXPECT_TRUE((ok && first.IsoTileTypeIndex == 44 && first.Height == 7 && first.Level == 2 && first.IsIceGrowthAllowed == 9)) << "legacy record formats preserve unrepresented ice field";
    }
    std::vector<unsigned char> planes(128 * 128 * 4, 0), legacy_packed(planes.size() * 2);
    const int legacy_index = first.MapCoords.X + first.MapCoords.Y * 128;
    planes[legacy_index * 2] = 42; planes[128 * 128 * 2 + legacy_index] = 7; planes[128 * 128 * 3 + legacy_index] = 2;
    BufferPipe legacy_out(legacy_packed.data(), int(legacy_packed.size())); LCWPipe legacy_writer(0, 8192); legacy_writer.Put_To(legacy_out);
    legacy_writer.Put(planes.data(), int(planes.size())); legacy_writer.Flush();
    BufferStraw legacy_source(legacy_packed.data(), legacy_out.Index);
    first.IsoTileTypeIndex = -1;
    EXPECT_TRUE((map.ReadIsoMapPack(legacy_source) && first.IsoTileTypeIndex == 44 && first.Height == 7 && first.Level == 2)) << "original 128x128 dense planes map into 512-stride cells";
}
using Bytes = std::vector<unsigned char>;
void append32(Bytes& bytes, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) bytes.push_back(static_cast<unsigned char>(value >> (8 * i)));
}
void csf_label(Bytes& bytes, const char* name, int count) {
    append32(bytes, CSF_LABEL_SIGNATURE); append32(bytes, count); append32(bytes, std::uint32_t(std::strlen(name)));
    bytes.insert(bytes.end(), name, name + std::strlen(name));
}
void csf_value(Bytes& bytes, const std::u16string& text, const char* extra = nullptr) {
    append32(bytes, extra ? CSF_EXVALUE_SIGNATURE : CSF_VALUE_SIGNATURE); append32(bytes, std::uint32_t(text.size()));
    for (char16_t unit : text) { const auto encoded = std::uint16_t(~unit); bytes.push_back(encoded & 255); bytes.push_back(encoded >> 8); }
    if (extra) { append32(bytes, std::uint32_t(std::strlen(extra))); bytes.insert(bytes.end(), extra, extra + std::strlen(extra)); }
}
void string_resources() {
    const auto root = std::filesystem::temp_directory_path() /
        ("ra2-csf-resource-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { StringTable::Unload(); std::filesystem::remove_all(path); } } cleanup{root};
    std::filesystem::create_directories(root);
    Bytes bytes;
    for (unsigned v : {unsigned(CSF_SIGNATURE), 3u, 3u, 3u, 0u, 6u}) append32(bytes, v);
    csf_label(bytes, "Name:Zulu", 2); csf_value(bytes, u"  Hello  world \t next \n line  ", "SPEECH01"); csf_value(bytes, u"\u65e5\u672c\u8a9e", "");
    csf_label(bytes, "Name:Alpha", 1); csf_value(bytes, u"AB"); csf_label(bytes, "Empty", 0);
    std::ofstream(root / "STRINGS.CSF", std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::ofstream(root / "TRUNCATED.CSF", std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), bytes.size() - 1);
    Bytes old = bytes; old[4] = 1;
    std::ofstream(root / "OLD.CSF", std::ios::binary).write(reinterpret_cast<const char*>(old.data()), old.size());
    game::ResourceHandle* output = nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(root.string(), output, error))) << "create CSF resource environment";
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> resources(output, game::destroy_resources);
    EXPECT_TRUE((game::with_resources(*resources, [](void*) {
        StringTable::Unload();
        char* speech = reinterpret_cast<char*>(1);
        EXPECT_TRUE((std::wcsncmp(StringTable::LoadString("x", &speech), L"***FATAL***", 11) == 0 && !speech)) << "unloaded string table status";
        EXPECT_TRUE((!StringTable::LoadFile("missing.csf") && !StringTable::IsLoaded && !StringTable::Labels)) << "missing CSF leaves unloaded state";
        EXPECT_TRUE((!StringTable::LoadFile("truncated.csf") && !StringTable::Labels)) << "truncated CSF rolls back partial arrays";
        static const char name[] = "strings.any.extension";
        EXPECT_TRUE((StringTable::LoadFile(name) && StringTable::FileName == name)) << "first-dot CSF extension and borrowed filename";
        EXPECT_TRUE((StringTable::LabelCount == 3 && StringTable::ValueCount == 3 && StringTable::MaxLabelLen == 10 &&
            StringTable::Language == CSFLanguages::Japanese)) << "CSF header and max label";
        EXPECT_TRUE((!std::strcmp(StringTable::Labels[0].Name, "Empty") && !std::strcmp(StringTable::Labels[1].Name, "Name:Alpha"))) << "case-insensitive label sorting";
        EXPECT_TRUE((!std::wcscmp(StringTable::LoadString("name:ZULU", &speech), L"Hello world\tnext\nline") &&
            speech && !std::strcmp(speech, "SPEECH01"))) << "target whitespace and extra string pointer";
        EXPECT_TRUE((!std::wcscmp(StringTable::Values[1], L"\u65e5\u672c\u8a9e") && !StringTable::ExtraValues[1])) << "UTF-16 inversion and empty extra value";
        EXPECT_TRUE((!std::wcscmp(StringTable::LoadString("name:alpha"), L"AB"))) << "second code unit is retained (assembly calibration)";
        const auto* first = StringTable::LoadString("absent"); const auto* second = StringTable::LoadString("absent");
        EXPECT_TRUE((first != second && !std::wcscmp(first, L"MISSING:'absent'") && StringTable::LastLoadedString->PreviousEntry)) << "missing strings form an owned chain";
        EXPECT_TRUE((!std::wcscmp(StringTable::TryFetchString("absent", L"default"), L"default") &&
            !std::wcscmp(StringTable::FetchString("<NONE>", L"default"), L"default"))) << "fetch helper fallbacks";
        TypeLifecycleFixture type("TYPE"); CCINIClass ini;
        EXPECT_TRUE((!type.LoadFromINI(&ini))) << "absent type section";
        ini.WriteString("TYPE", "Name", "Resource name"); ini.WriteString("TYPE", "UIName", "Name:Alpha");
        ini.WriteString("TYPE", "Unrelated", "removed by original save");
        AbstractTypeClass* base = &type;
        EXPECT_TRUE((base->LoadFromINI(&ini) && !std::strcmp(type.Name, "Resource name") && !std::wcscmp(type.UIName, L"AB"))) << "type name and CSF resource dependency";
        EXPECT_TRUE((base->SaveToINI(&ini) && ini.GetKeyCount("TYPE") == 2)) << "original base save clears section";
        auto* labels = StringTable::Labels; const auto* value = StringTable::Values[0];
        EXPECT_TRUE((!StringTable::ReadFile("truncated.csf") && StringTable::Labels == labels && StringTable::Values[0] == value)) << "failed ReadFile leaves current table intact";
        EXPECT_TRUE((StringTable::LoadFile("missing.csf"))) << "already-loaded short circuit";
        StringTable::Unload();
        EXPECT_TRUE((!StringTable::Labels && !StringTable::Values && !StringTable::ExtraValues && !StringTable::LastLoadedString &&
            StringTable::LabelCount == 3 && StringTable::Language == CSFLanguages::Japanese)) << "Unload frees owned arrays and retains metadata";
        EXPECT_TRUE((StringTable::LoadFile("old.csf") && StringTable::Language == CSFLanguages::US)) << "version-one language fallback";
        EXPECT_TRUE((StringTable::GetLanguage(CSFLanguages::Chinese)->Letter[0] == 'c' && !StringTable::GetLanguage(CSFLanguages::Unknown) &&
            !std::strcmp(StringTable::GetLanguageName(CSFLanguages::Unknown), "unknown"))) << "language sentinel lookup";
        StringTable::Unload();
    }, nullptr, error))) << "CSF resource scope";
}
void resource_art() {
    const auto root = std::filesystem::temp_directory_path() /
        ("ra2-map-resource-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{root};
    std::filesystem::create_directories(root);
    const char payload[] = "original smudge bytes";
    std::ofstream(root / "SCORCH.SNO", std::ios::binary).write(payload, sizeof(payload));
    game::ResourceHandle* output = nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(root.string(), output, error))) << "create resource environment";
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> resources(output, game::destroy_resources);
    ResourceFixture<SmudgeTypeClass> scorch, ignored, missing;
    std::strcpy(scorch.ID, "SCORCH"); std::strcpy(scorch.ImageFile, "DIFFERENT");
    std::strcpy(ignored.ID, "IGNORE"); std::strcpy(missing.ID, "MISSING");
    scorch.Theater = missing.Theater = true; ignored.Theater = false;
    scorch.Image = missing.Image = nullptr;
    ignored.Image = reinterpret_cast<SHPStruct*>(&ignored);
    scorch.ImageAllocated = false; missing.ImageAllocated = true;
    auto& array = SmudgeTypeClass::Array;
    EXPECT_TRUE((array.Count == 0)) << "native resource array starts empty";
    EXPECT_TRUE((array.AddItem(&scorch) && array.AddItem(&ignored) && array.AddItem(&missing))) << "populate borrowed resource types";
    struct ClearArray { ~ClearArray() { SmudgeTypeClass::Array.Clear(); } } clear;
    EXPECT_TRUE((SmudgeTypeClass::Find("sCoRcH") == &scorch && SmudgeTypeClass::FindIndex("none") == -1)) << "original registry lookup";
    EXPECT_TRUE((game::with_resources(*resources, [](void*) {
        SmudgeTypeClass::LoadFromIniList(static_cast<int>(TheaterType::Snow));
    }, nullptr, error))) << "load smudge theater resource";
    std::unique_ptr<void, decltype(&YRMemory::Deallocate)> bytes(scorch.Image, YRMemory::Deallocate);
    EXPECT_TRUE((scorch.Image && std::memcmp(scorch.Image, payload, sizeof(payload)) == 0)) << "smudge loader uses ID and theater extension";
    EXPECT_TRUE((!scorch.ImageAllocated && missing.ImageAllocated && !missing.Image)) << "original image ownership flags and missing file";
    EXPECT_TRUE((ignored.Image == reinterpret_cast<SHPStruct*>(&ignored))) << "non-theater art unchanged";
    EXPECT_TRUE((scorch.GetImage() == scorch.Image)) << "base image resource accessor";
    // The real Smudge destructor now owns Image regardless of ImageAllocated
    // (6B6160). These fixtures deliberately borrowed a sentinel and a buffer
    // already owned by `bytes`, so detach both before their real destructors.
    ignored.Image = nullptr;
    scorch.Image = nullptr;
}
}

TEST(MapResource, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();


            if (argc == 2) fixture_dir = argv[1];
            type_lifecycle(); theaters(); cell_geometry(); map_geometry(); map_pack(); resource_art(); string_resources();
            identity<IsometricTileTypeClass>(0x5af2ce7a); // Constants below are checked against the target export.
            identity<OverlayTypeClass>(0x5af2ce79);
            identity<TerrainTypeClass>(0x5af2ce7b);
            identity<SmudgeTypeClass>(0x5af2ce78);
            identity<CellClass>(0xc1bf99ce, 0x11d21a8c, 0x60007581);
            identity<IsometricTileClass>(0x0e272dc0, 0x11d19c0f, 0xa00009b7, 0xd1afdd24);
            identity<OverlayClass>(0x0e272dc7, 0x11d19c0f, 0xa00009b7, 0xd1afdd24);
            identity<TerrainClass>(0x0e272dce, 0x11d19c0f, 0xa00009b7, 0xd1afdd24);
            identity<SmudgeClass>(0x0e272dc5, 0x11d19c0f, 0xa00009b7, 0xd1afdd24);
}
