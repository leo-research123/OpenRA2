#include "support/test_support.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/Straws.h"
#include "yrpp/Pipes.h"
#include "api/ini_runtime.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string value(INIClass& ini, const char* section = "S", const char* key = "K") {
    char text[20480]{}; ini.ReadString(section, key, "", text, sizeof(text)); return text;
}
struct TextPipe : Pipe {
    std::string text;
    int Put(const void* data, int count) override { text.append(static_cast<const char*>(data), size_t(count)); return count; }
};
std::string serialize(INIClass& ini) { TextPipe pipe; ini.WritePipe(pipe); return pipe.text; }
std::string hex(const void* data, size_t length) {
    std::string text;
    const auto* bytes = static_cast<const byte*>(data);
    for (size_t i = 0; i < length; ++i) { text += "0123456789abcdef"[bytes[i] >> 4]; text += "0123456789abcdef"[bytes[i] & 15]; }
    return text;
}
void load(INIClass& ini, const std::string& text) {
    BufferStraw input(const_cast<char*>(text.data()), int(text.size())); ini.ReadStraw(input);
}
using ReadEnum = int (INIClass::*)(const char*, const char*, int);
struct NamedReader { const char* name; ReadEnum read; };
#define READER(Name) {#Name, &INIClass::Read##Name}
constexpr NamedReader readers[] = {
    READER(Pip), READER(PipScale), READER(Category), READER(ColorString), READER(Foundation),
    READER(MovementZone), READER(SpeedType), READER(SWAction), READER(SWType), READER(VoxName),
    READER(Factory), READER(BuildCat), READER(HouseTypesList), READER(HousesList), READER(ArmorType),
    READER(LandType), READER(HouseType), READER(Side), READER(Movie), READER(Theater), READER(Theme),
    READER(Edge), READER(Powerup), READER(Layer), READER(VHPScan)};
#undef READER
void values() {
    INIClass ini;
    int defaults[] = {7, 8, 9}, numbers[3]{};
    EXPECT_TRUE((ini.Read3Integers(numbers,"S","K",defaults) == numbers && numbers[2] == 9)) << "tuple fallback";
    ini.WriteString("S","K","-12, 34,256");
    ini.Read3Integers(numbers,"S","K",defaults);
    EXPECT_TRUE((numbers[0] == -12 && numbers[1] == 34 && numbers[2] == 256)) << "three integers";
    Point2D p2{1,2}; ini.GetPoint2D("S","K",p2);
    CoordStruct p3{1,2,3}; ini.GetPoint3D("S","K",p3);
    EXPECT_TRUE((p2.X == -12 && p2.Y == 34 && p3.Z == 256)) << "point wrappers and aliased defaults";
    EXPECT_TRUE((ini.Write2Integers("S","K",defaults) && value(ini) == "7,8")) << "tuple writer";
    ini.Read2Integers(numbers,"S","K",numbers);
    EXPECT_TRUE((numbers[0] == 7 && numbers[1] == 8)) << "two integers";
    ini.WriteString("S","K","12,bad");
    ini.Read3Integers(numbers,"S","K",defaults);
    EXPECT_TRUE((numbers[0] == 12 && numbers[1] == 8 && numbers[2] == 9)) << "malformed fields have deterministic defaults";
    byte bytes[3]{}, bdefault[] = {11,22,33};
    ini.Read3Bytes(bytes,"S","K",bdefault);
    EXPECT_TRUE((bytes[0] == 12 && bytes[1] == 22 && bytes[2] == 33)) << "malformed byte defaults";
    ColorStruct fallback{11,22,33}, color;
    ini.ReadColor(&color,"S","K",fallback);
    EXPECT_TRUE((color.R == 12 && !color.G && !color.B)) << "ReadColor initializes absent components to zero";
    ini.WriteString("S","K","-1,256,511"); ini.GetColor("S","K",color);
    EXPECT_TRUE((color.R == 255 && !color.G && color.B == 255)) << "color wraps to bytes";
    EXPECT_TRUE((ini.WriteColor("S","K",fallback) && value(ini) == "11,22,33")) << "color writer";
    EXPECT_TRUE((ini.Write3Bytes("S","K",bdefault))) << "byte writer";
    ini.Clear(); EXPECT_TRUE((ini.ReadTime("S","K",77) == 77)) << "time fallback";
    ini.WriteString("S","K","01:02:03"); EXPECT_TRUE((ini.ReadTime("S","K",0) == 223380)) << "60 ticks per second";
    ini.WriteTime("S","K",223439); EXPECT_TRUE((value(ini) == "01:02:03")) << "time discards subsecond ticks";
    ini.WriteString("S","K","123:45:56"); EXPECT_TRUE((ini.ReadTime("S","K",0) == 2592000)) << "two-digit hour scan width";
    EXPECT_TRUE((ini.WriteUnicodeString("S","K",L"A\x4e2d\xd83d\xde00") && value(ini) == "41,4e2d,d83d,de00,")) << "escaped UTF-16 writer";
    wchar_t text[8]{};
    EXPECT_TRUE((ini.ReadUnicodeString("S","K",L"",text,8) == 4 && text[1] == 0x4e2d && text[3] == 0xde00)) << "escaped UTF-16 reader";
    struct Guard { wchar_t text[2]; wchar_t canary; } guard{{L'x',L'x'},L'!'};
    EXPECT_TRUE((ini.ReadUnicodeString("S","K",L"",guard.text,2) == 1 && guard.text[1] == 0 && guard.canary == L'!')) << "Unicode capacity includes terminator";
    ini.Clear(); std::wcscpy(text,L"abc");
    EXPECT_TRUE((ini.ReadUnicodeString("S","K",text,text,3) == 2 && !std::wcscmp(text,L"ab"))) << "aliased Unicode fallback truncates safely";
    ini.WriteString("S","K",",41,,0,42,");
    EXPECT_TRUE((ini.ReadUnicodeString("S","K",L"",text,8) == 1 && text[2] == L'B')) << "Unicode embedded zero return length";
    byte abilities[18], previous[18]; std::memset(previous,3,18);
    ini.ReadAbilities(abilities,"S","missing",previous);
    EXPECT_TRUE((!std::memcmp(abilities,previous,18))) << "ability fallback bytes";
    ini.WriteString("S","K","FASTER,,STRONGER, FASTER,unknown");
    ini.ReadAbilities(abilities,"S","K",previous);
    int count=0; for(byte entry:abilities) count += entry;
    EXPECT_TRUE((count == 2 && abilities[0] == 1 && abilities[1] == 1)) << "abilities list and token spacing";
    EXPECT_TRUE((ini.ReadPip("S","missing",0) == 1)) << "Pip default is a table index";
    ini.WriteString("S","K","Subterannean"); EXPECT_TRUE((ini.ReadMovementZone("S","K",0) == 6)) << "original movement spelling";
    ini.WriteString("S","K","unknown");
    EXPECT_TRUE((ini.ReadArmorType("S","K",5) == 0 && ini.ReadMovie("S","missing",17) == 17)) << "per-method fallbacks";
}
void blocks_and_digest() {
    CCINIClass ini;
    EXPECT_TRUE((ini.GetCRC() == 0xb8036321u && hex(ini.Digest,20) == "da39a3ee5e6b4b0d3255bfef95601890afd80709")) << "empty SHA-1/CRC";
    ini.WriteString("S","K","value");
    EXPECT_TRUE((ini.GetCRC() == 0xb8036321u)) << "original digest remains cached after mutation";
    ini.Digested = false;
    EXPECT_TRUE((serialize(ini) == "[S]\r\nK=value\r\n")) << "digest serialized input";
    EXPECT_TRUE((ini.GetCRC() == 0xa7f3f2e1u && hex(ini.Digest,20) == "5321eeb2b55635cd9946c6da624907e419c6fb6f")) << "serialized SHA-1/CRC";
    for (size_t length: {size_t(1),size_t(2),size_t(3),size_t(51),size_t(52),size_t(53),size_t(54),size_t(1024),size_t(65537)}) {
        std::vector<byte> source(length), decoded(length + 2, 0xa5);
        for (size_t i=0; i<length; ++i) source[i] = byte(i*73+17);
        EXPECT_TRUE((ini.WriteUUBlock("Block",source.data(),source.size()))) << "UUBlock writer";
        const int lines = int(((length+2)/3*4+69)/70);
        EXPECT_TRUE((ini.GetKeyCount("Block") == lines)) << "70-character line count";
        if(lines>1) EXPECT_TRUE((value(ini,"Block","1").size() == 70)) << "70-character line width";
        EXPECT_TRUE((ini.ReadUUBlock("Block",decoded.data()+1,length) == length && !std::memcmp(source.data(),decoded.data()+1,length)
            && decoded.front()==0xa5 && decoded.back()==0xa5)) << "UUBlock roundtrip and boundaries";
        if(length>1) EXPECT_TRUE((ini.ReadUUBlock("Block",decoded.data()+1,length-1) == length-1)) << "UUBlock clamps return length";
    }
    ini.Clear(); ini.WriteString("B","9","YW"); ini.WriteString("B","1","JjZA==");
    byte result[8]{};
    EXPECT_TRUE((ini.ReadUUBlock("B",result,8) == 4 && !std::memcmp(result,"abcd",4))) << "UUBlock uses insertion order and joins split packets";
    EXPECT_TRUE((!ini.WriteUUBlock("B",result,0) && ini.GetKeyCount("B") == 2)) << "empty UU write preserves section";
    EXPECT_TRUE((!ini.ReadUUBlock("B",nullptr,8))) << "null UU destination";
    auto* old_rules = CCINIClass::INI_Rules;
    CCINIClass::INI_Rules = &ini; CCINIClass::RulesHash = ini.GetCRC();
    CCINIClass::ArtHash = 12; CCINIClass::AIHash = 34;
    for(auto* global : {&CCINIClass::INI_AI, &CCINIClass::INI_Art, &CCINIClass::INI_UIMD, &CCINIClass::INI_RA2MD}) {
        global->WriteString("test","K","V"); EXPECT_TRUE((global->Exists("test","K"))) << "global INI instance"; global->Clear();
    }
    CCINIClass::INI_Rules = old_rules;
    EXPECT_TRUE((CCINIClass::ArtHash == 12 && CCINIClass::AIHash == 34 && CCINIClass::RulesHash == ini.GetCRC())) << "hash globals";
}
using Kind = game::IniTypeKind;
struct RuntimeFixture {
    std::vector<Kind> search;
    int creates = 0;
    alignas(void*) byte techno[4]{}; // Borrowed identity only: never dereferenced as a game object.
    static bool name(void*, Kind, int index, const char*& output) {
        if(index!=2) return false; output="known"; return true;
    }
    static bool find(void* context, Kind kind, const char* text, bool allocate, game::IniTypeResult& output) {
        auto& fixture = *static_cast<RuntimeFixture*>(context); fixture.search.push_back(kind);
        if(allocate && std::strcmp(text,"created")==0) {
            EXPECT_TRUE((kind==Kind::house_type || kind==Kind::side)) << "allocation limited to original types";
            ++fixture.creates; output.index=5; return true;
        }
        if(kind==Kind::house_type && std::strcmp(text,"Random")==0) { output.index=-2; return true; }
        if(std::strcmp(text,"known") && !(kind==Kind::building && !std::strcmp(text,"GAPOWR"))) return false;
        output.index=2; output.techno=reinterpret_cast<TechnoTypeClass*>(fixture.techno); return true;
    }
    static bool stringtable(void*, const char* text, const wchar_t*& output) {
        output=std::strcmp(text,"Name:Test") ? L"MISSING:" : L"Localized"; return true;
    }
    game::IniRuntimeServices services() { return {this,name,find,stringtable}; }
};
void runtime_operation(void* context) {
    auto& fixture = *static_cast<RuntimeFixture*>(context); fixture.creates = 0; CCINIClass ini;
    ini.WriteString("S","K","known");
    for(auto read : {&INIClass::ReadColorString,&INIClass::ReadSWType,&INIClass::ReadVoxName,&INIClass::ReadHouseType,
            &INIClass::ReadSide,&INIClass::ReadMovie,&INIClass::ReadTheme}) EXPECT_TRUE(((ini.*read)("S","K",-1)==2)) << "runtime lookup";
    EXPECT_TRUE((ini.ReadSWType("S","missing",2)==2 && ini.ReadColorString("S","missing",2)==2)) << "runtime fallback name";
    fixture.search.clear();
    EXPECT_TRUE((ini.GetTechnoType("S","K")==reinterpret_cast<TechnoTypeClass*>(fixture.techno)
        && fixture.search==std::vector<Kind>{Kind::infantry})) << "Techno lookup precedence";
    ini.WriteString("S","K","GAPOWR"); fixture.search.clear();
    EXPECT_TRUE((ini.GetTechnoType("S","K") && fixture.search==std::vector<Kind>{Kind::infantry,Kind::unit,Kind::aircraft,Kind::building})) << "all Techno registries";
    ini.WriteString("S","K","created");
    EXPECT_TRUE((ini.ReadHouseType("S","K",-1)==5 && ini.ReadSide("S","K",-1)==5 && fixture.creates==2)) << "country and side allocate";
    ini.WriteString("S","K","<Player @ H>"); EXPECT_TRUE((ini.ReadHouseType("S","K",-1)==4482)) << "MP player placeholder";
    ini.WriteString("S","K","known,missing,Random");
    EXPECT_TRUE((ini.ReadHouseTypesList("S","K",0)==0x40000004)) << "country bits and Random shift";
    EXPECT_TRUE((std::bit_cast<unsigned>(ini.ReadHousesList("S","K",0))==0x80000004u)) << "missing House sets bit 31";
    ini.WriteString("S","K","POWER,FACTORY,BARRACKS,RADAR,TECH,PROC,GAPOWR,bad,GAPOWR");
    alignas(TypeList<int>) byte storage[sizeof(TypeList<int>)];
    TypeList<int> defaults; defaults.AddItem(9); defaults.unknown_18=77;
    auto* result = INIClass::GetPrerequisites(reinterpret_cast<TypeList<int>*>(storage),&ini,"S","K",defaults);
    const int expected[]={-1,-2,-3,-4,-5,-6,2,2};
    EXPECT_TRUE((result->Count==8 && !std::memcmp(result->Items,expected,sizeof(expected)))) << "prerequisite generic codes, ordering and duplicate preservation";
    result->~TypeList<int>();
    result=INIClass::GetPrerequisites(reinterpret_cast<TypeList<int>*>(storage),&ini,"S","missing",defaults);
    EXPECT_TRUE((result->Count==1 && result->Items[0]==9 && result->unknown_18==0 && defaults.unknown_18==77
        && result->Items!=defaults.Items)) << "prerequisite fallback deep copy and initialized extension field";
    result->~TypeList<int>();
    TypeList<int> empty; empty.CapacityIncrement=23;
    result=INIClass::GetPrerequisites(reinterpret_cast<TypeList<int>*>(storage),&ini,"S","missing",empty);
    EXPECT_TRUE((result->Count==0 && result->Capacity==0 && result->CapacityIncrement==23)) << "empty prerequisite fallback preserves growth policy";
    result->~TypeList<int>();
    ini.WriteString("S","K","Name:Test"); wchar_t text[5]{};
    EXPECT_TRUE((ini.ReadStringtableEntry("S","K",text)==4 && !std::wcscmp(text,L"Loca"))) << "StringTable truncation and termination";
    EXPECT_TRUE((ini.ReadStringtableEntry("S","missing",text)==4 && !std::wcscmp(text,L"MISS"))) << "StringTable missing label semantics";
}
void runtime_tests() {
    RuntimeFixture fixture; const auto services=fixture.services(); std::string error;
    EXPECT_TRUE((!game::with_ini_runtime({},runtime_operation,&fixture,error) && !error.empty())) << "invalid services rejected";
    EXPECT_TRUE((game::with_ini_runtime(services,runtime_operation,&fixture,error))) << error.c_str();
    INIClass ini;
    EXPECT_TRUE((ini.GetTechnoType("S","K") == nullptr)) << "scope restores native registries, not fixture callbacks";
    EXPECT_TRUE((game::with_ini_runtime(services,[](void* context) {
        auto& fixture=*static_cast<RuntimeFixture*>(context); auto inner=fixture.services(); std::string error;
        EXPECT_TRUE((!game::with_ini_runtime(inner,[](void*) { throw std::runtime_error("test exception"); },nullptr,error)
            && error=="test exception")) << "nested exception reported";
        runtime_operation(context);
    },&fixture,error))) << error.c_str();
}
std::string file(const char* path) {
    std::ifstream input(path,std::ios::binary); EXPECT_TRUE((bool(input))) << "probe input missing";
    return std::string(std::istreambuf_iterator<char>(input),{});
}
}

TEST(IniValue, ValuesEncodingDigestAndRuntime) {
    CCINIClass ini;
    values();
    blocks_and_digest();
    runtime_tests();
}

namespace {
std::optional<int> legacy_command(int argc, char** argv) {
    if (argc < 3) return std::nullopt;
    CCINIClass ini;
            if(argc>=3 && !std::strcmp(argv[1],"--prerequisites")) {
                ini.WriteString("S","K",argv[2]); RuntimeFixture fixture; std::string error;
                struct Context { INIClass& ini; bool empty; } context{ini,argc>3};
                const bool ok=game::with_ini_runtime(fixture.services(),[](void* raw) {
                    auto& context=*static_cast<Context*>(raw); TypeList<int> defaults;
                    if(!context.empty) defaults.AddItem(9);
                    defaults.CapacityIncrement=23;
                    alignas(TypeList<int>) byte storage[sizeof(TypeList<int>)];
                    auto* result=INIClass::GetPrerequisites(reinterpret_cast<TypeList<int>*>(storage),&context.ini,"S","K",defaults);
                    std::cout<<result->Capacity<<','<<result->Count<<','<<result->CapacityIncrement;
                    for(int i=0;i<result->Count;++i) std::cout<<','<<result->Items[i];
                    result->~TypeList<int>();
                },&context,error);
                EXPECT_TRUE((ok)) << error.c_str(); std::cout<<'\n'; return 0;
            }
            if(argc>=4 && !std::strcmp(argv[1],"--scalar")) {
                ini.WriteString("S","K",argv[3]);
                const std::string name=argv[2];
                if(name=="Time") std::cout<<ini.ReadTime("S","K",77);
                else if(name=="WriteTime") { ini.WriteTime("S","K",std::stoi(argv[3])); std::cout<<value(ini); }
                else if(name=="Unicode") {
                    wchar_t text[128]{};
                    const int count=ini.ReadUnicodeString("S","K",L"default",text,128);
                    std::cout<<count;
                    for(int i=0;i<count;++i) std::cout<<','<<unsigned(text[i]);
                } else if(name=="Abilities") {
                    byte text[18]{}, defaults[18]; std::memset(defaults,3,18);
                    ini.ReadAbilities(text,"S","K",defaults);
                    for(int i=0;i<18;++i) { if(i) std::cout<<','; std::cout<<unsigned(text[i]); }
                } else if(name=="Bytes" || name=="Color") {
                    byte text[3]{}, defaults[3]={7,8,9};
                    if(name=="Bytes") ini.Read3Bytes(text,"S","K",defaults);
                    else { const auto color=ini.ReadColor("S","K",ColorStruct{7,8,9}); text[0]=color.R;text[1]=color.G;text[2]=color.B; }
                    std::cout<<unsigned(text[0])<<','<<unsigned(text[1])<<','<<unsigned(text[2]);
                } else {
                    int text[3]{}, defaults[]={7,8,9};
                    if(name=="2") ini.Read2Integers(text,"S","K",defaults);
                    else ini.Read3Integers(text,"S","K",defaults);
                    for(int i=0;i<(name=="2"?2:3);++i) { if(i) std::cout<<','; std::cout<<text[i]; }
                }
                std::cout<<'\n'; return 0;
            }
            if(argc>=3 && !std::strcmp(argv[1],"--read-digest")) {
                RawFileClass input(argv[2]); const int status=ini.ReadCCFile(&input,true);
                const auto text=serialize(ini);
                std::cout<<status<<' '<<ini.Digested<<' '<<(ini.Digested?hex(ini.Digest,20):"-")<<' '<<hex(text.data(),text.size())<<'\n'; return 0;
            }
            if(argc>=4 && !std::strcmp(argv[1],"--enum")) {
                if(argc>4) ini.WriteString("S","K",argv[4]);
                for(const auto& reader:readers) if(!std::strcmp(argv[2],reader.name)) {
                    std::cout<<(ini.*reader.read)("S","K",std::stoi(argv[3]))<<'\n'; return 0;
                }
                throw std::runtime_error("unknown enum reader");
            }
            if(argc>=3 && !std::strcmp(argv[1],"--digest")) {
                load(ini,file(argv[2])); const auto crc=ini.GetCRC();
                std::cout<<hex(ini.Digest,20)<<' '<<crc<<'\n'; return 0;
            }
            if(argc>=3 && !std::strcmp(argv[1],"--uuencode")) {
                auto data=file(argv[2]); ini.WriteUUBlock("B",data.data(),data.size());
                const auto text=serialize(ini); std::cout<<hex(text.data(),text.size())<<'\n'; return 0;
            }
            if(argc>=4 && !std::strcmp(argv[1],"--uudecode")) {
                load(ini,file(argv[2])); std::vector<byte> data(std::stoul(argv[3]));
                const auto length=ini.ReadUUBlock("B",data.data(),data.size());
                std::cout<<hex(data.data(),length)<<'\n'; return 0;
            }
    return std::nullopt;
}
const ra2::test::CommandRegistration command(legacy_command);
}
