#include "support/test_support.hpp"
#include "yrpp/BitFont.h"
#include "yrpp/Surface.h"
#include "yrpp/Memory.h"
#include "api/type_drawing.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {
using Bytes = std::vector<unsigned char>;

void word(Bytes& data, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) data.push_back(static_cast<unsigned char>(value >> (8 * i)));
}
const std::array<unsigned, 4> codes{32, 65, 88, 0x4e2d};
const std::array<unsigned char, 28> bitmaps{
    3, 0,0, 0,0, 0,0,
    5, 0x70,0, 0x88,0, 0xf8,0,
    9, 0x80,0x80, 0x41,0, 0x22,0,
    9, 0xff,0x80, 0x80,0x80, 0xff,0x80};
Bytes fixture(bool ranges) {
    Bytes data;
    for (auto value : {ranges ? 0x744e6f46u : 0x546e6f66u, 9u,2u,3u,4u,4u,7u}) word(data,value);
    if (ranges) {
        word(data, 40); word(data, 96); // Deliberate holes exercise both seeks.
        data.resize(40,0xdd);
        for (unsigned i = 0; i < codes.size(); ++i) {
            word(data,i); word(data,codes[i]); word(data,codes[i]);
        }
        data.resize(96,0xee);
    } else {
        data.resize(28+0x20000,0);
        for (unsigned i = 0; i < codes.size(); ++i) data[28+2*codes[i]]=i+1;
    }
    data.insert(data.end(),bitmaps.begin(),bitmaps.end());
    return data;
}
struct Temp {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("ra2-bitfont-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp() { std::filesystem::create_directories(root); }
    ~Temp() { std::error_code error; std::filesystem::remove_all(root,error); }
    std::string write(const char* name, const Bytes& data) {
        auto path=root/name;
        std::ofstream file(path,std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));
        EXPECT_TRUE((bool(file))) << "write fixture"; file.close();
        return path.string();
    }
};
static_assert(std::is_same_v<decltype(&BitFont::Lock), void (BitFont::*)(Surface*)>);
static_assert(std::is_same_v<decltype(&BitFont::UnLock), void (BitFont::*)(Surface*)>);
void lock_state_contracts() {
    // No font data is needed to exercise the original surface state contract.
    BitFont font("");
    for (int height : {1, 2, 256, 257, 258}) {
        std::vector<short> pixels(2 * height);
        BSurface surface(2, height, 2, pixels.data());
        font.SetBounds(nullptr);
        font.Lock(&surface);
        EXPECT_TRUE((font.pGraphBuffer == pixels.data() && font.PitchDiv2 == 2 && surface.IsLocked())) << "Lock records surface storage and pitch";
        EXPECT_TRUE((font.Bounds.Left == 0 && font.Bounds.Top == 0 &&
            font.Bounds.Right == 1 && font.Bounds.Bottom == height - 1)) << "zero Bounds becomes surface bounds";
        font.UnLock(&surface);
        EXPECT_TRUE((!font.pGraphBuffer && !font.PitchDiv2 && !surface.IsLocked())) << "UnLock clears surface state";
        LTRBStruct inside{0, 0, 1, height - 1};
        font.SetBounds(&inside);
        font.Lock(&surface);
        EXPECT_TRUE((font.Bounds.Right == inside.Right && font.Bounds.Bottom == inside.Bottom)) << "nonempty Bounds is intersected with surface";
        font.UnLock(&surface);
        LTRBStruct outside{5, 5, 6, 6};
        font.SetBounds(&outside);
        font.Lock(&surface);
        EXPECT_TRUE((font.Bounds.Left == 0 && font.Bounds.Top == 0 &&
            font.Bounds.Right == -1 && font.Bounds.Bottom == -1 && surface.IsLocked())) << "disjoint Bounds yields empty rectangle while retaining lock";
        font.UnLock(&surface);
    }
    // Verify the observable virtual-call order, including a failed Surface
    // lock. BitFont still queries dimensions; it does not invent a bool result.
    struct QuerySurface : Surface {
        BitFont& font;
        std::string calls;
        explicit QuerySurface(BitFont& font) : font(font) {}
        void* Lock(int x, int y) override {
            EXPECT_TRUE((x == 0 && y == 0)) << "Lock origin"; calls += 'L'; return nullptr;
        }
        int GetPitch() override {
            EXPECT_TRUE((!font.pGraphBuffer)) << "store Lock output before querying pitch"; calls += 'P'; return 5;
        }
        int GetWidth() override {
            EXPECT_TRUE((font.PitchDiv2 == 2)) << "store halved pitch before querying dimensions"; calls += 'W'; return 2;
        }
        int GetHeight() override { calls += 'H'; return 257; }
        bool Unlock() override {
            EXPECT_TRUE((font.PitchDiv2 == 2)) << "call Surface Unlock before clearing font state"; calls += 'U'; return false;
        }
    } query(font);
    font.SetBounds(nullptr);
    font.Lock(&query);
    EXPECT_TRUE((query.calls == "LPWH" && !font.pGraphBuffer && font.Bounds.Bottom == 256)) << "failed Surface lock retains original query order and bounds changes";
    font.UnLock(&query);
    EXPECT_TRUE((query.calls == "LPWHU" && !font.pGraphBuffer && !font.PitchDiv2)) << "UnLock clears state even when Surface Unlock reports false";
}
void release(BitFont::InternalData* data) {
    if (!data) return;
    YRMemory::Deallocate(data->SymbolTable); YRMemory::Deallocate(data->Bitmaps); YRMemory::Deallocate(data);
}
void dimensions(BitFont& font, const wchar_t* text, int maximum, int w, int h) {
    int width=-1,height=-1;
    EXPECT_TRUE((font.GetTextDimension(text,&width,&height,maximum))) << "text measurement success";
    EXPECT_TRUE((width==w && height==h)) << "text measurement dimensions";
}
void font_contracts(const std::string& name) {
    auto* standalone=BitFont::LoadInternalData(name.c_str());
    EXPECT_TRUE((standalone && standalone->Count==4)) << "direct load entry"; release(standalone);
    BitFont font(name.c_str());
    EXPECT_TRUE((font.InternalPTR && font.InternalPTR->FontWidth==9 && font.InternalPTR->Stride==2 &&
        font.InternalPTR->FontHeight==3 && font.InternalPTR->Lines==4 && font.InternalPTR->Count==4 &&
        font.InternalPTR->SymbolDataSize==7 && font.InternalPTR->ValidSymbolCount==4)) << "both format layouts";
    EXPECT_TRUE((!std::memcmp(font.InternalPTR->Bitmaps,bitmaps.data(),bitmaps.size()))) << "raw bitmaps retained";
    EXPECT_TRUE((font.field_18==2 && font.field_1C==4 && font.State_2C==1 && font.Unknown_28==64 &&
        font.Color==0x7fff && font.DefaultColor2==0x3555 && font.field_41 && font.Bool_40)) << "constructor state";
    for (unsigned i=0;i<codes.size();++i)
        EXPECT_TRUE((font.GetCharacterBitmap(wchar_t(codes[i]))==
            reinterpret_cast<unsigned char*>(font.InternalPTR->Bitmaps)+7*i)) << "character map";
    EXPECT_TRUE((!font.GetCharacterBitmap(L'?'))) << "missing glyph returns null";
    const unsigned char missing[]{9,0x7f,0x7f,0xbe,0xff,0xdd,0xff};
    EXPECT_TRUE((font.Pointer_8 && !std::memcmp(font.Pointer_8,missing,sizeof(missing)))) << "inverted X fallback";
    dimensions(font,L"",0,0,4); dimensions(font,L"A",0,6,4);
    dimensions(font,L"A A",0,16,4); dimensions(font,L"A\r\nA",0,6,8);
    dimensions(font,L"A\tA",0,70,4); dimensions(font,L"AAA",12,12,8);
    dimensions(font,L"?",0,10,4); dimensions(font,L"\x4e2d",0,10,4);
    int width=17,height=18;
    EXPECT_TRUE((!font.GetTextDimension(nullptr,&width,&height,0) && !width && !height)) << "null text output";
    EXPECT_TRUE((font.GetTextDimension(L"A",nullptr,nullptr,0))) << "optional output pointers";
    EXPECT_TRUE((font.Blit(L'A',3,1,-1)==3)) << "unlocked glyph draw leaves x unchanged";
    std::array<short,240> pixels; pixels.fill(0x1234);
    BSurface surface(24,10,2,pixels.data());
    font.Lock(&surface);
    EXPECT_TRUE((surface.LockLevel==1 && font.pGraphBuffer==pixels.data() && font.PitchDiv2==24 &&
        font.Bounds.Left==0 && font.Bounds.Top==0 && font.Bounds.Right==23 && font.Bounds.Bottom==9)) << "surface lock and bounds";
    EXPECT_TRUE((font.Blit(L'A',3,1,-1)==9)) << "draw advances by width plus spacing";
    const std::array<unsigned,10> set{28,29,30,51,55,75,76,77,78,79};
    for (unsigned i=0;i<pixels.size();++i) {
        bool ink=false; for (auto index:set) ink|=i==index;
        EXPECT_TRUE((pixels[i]==(ink ? 0x7fff : 0x1234))) << "MSB-first glyph pixels and transparent background";
    }
    font.UnLock(&surface);
    EXPECT_TRUE((!font.pGraphBuffer && !font.PitchDiv2 && surface.LockLevel==0)) << "unlock state";
    pixels.fill(0x1234); font.SetBounds(nullptr); font.Lock(&surface);
    EXPECT_TRUE((font.Blit(L'A',-2,-1,0x2345)==4)) << "partially clipped advance";
    for (unsigned i=0;i<pixels.size();++i)
        EXPECT_TRUE((pixels[i]==((i==2 || i==24 || i==25 || i==26) ? 0x2345 : 0x1234))) << "left/top clipping";
    pixels.fill(0x1234);
    LTRBStruct narrow{3,2,7,2}; font.SetRectangle(&narrow);
    EXPECT_TRUE((font.Blit(L'A',3,1,0x3456)==9 && pixels[51]==0x3456 && pixels[55]==0x3456)) << "one inclusive row survives clipping";
    EXPECT_TRUE((font.Blit(L'A',50,50,-1)==56)) << "fully clipped glyph still advances";
    font.SetColor(0x4567); font.SetClipMode(false); pixels.fill(0x1234);
    font.Blit(L'A',3,1,-1); EXPECT_TRUE((pixels[28]==0x4567)) << "clip disabled and stored color";
    font.SetField41(1); font.SetField20(7);
    EXPECT_TRUE((font.Blit(L'\t',8,0,-1)==71)) << "tab origin";
    font.SetBounds(nullptr); font.UnLock(&surface);
    pixels.fill(0x1234); font.Lock(&surface);
    font.Blit(L'?',0,0,0x1234);
    EXPECT_TRUE((pixels[0]==0x1234 && pixels[1]==short(0x1234^0x5555) && pixels[8]==0x1234)) << "missing glyph inverted bitmap and xor color";
    font.UnLock(&surface);
}
void failure_contracts(Temp& temp) {
    EXPECT_TRUE((!BitFont::LoadInternalData(nullptr) && !BitFont::LoadInternalData(""))) << "null/empty filename";
    const auto absent=(temp.root/"absent.fnt").string();
    BitFont missing(absent.c_str());
    int w=1,h=2;
    EXPECT_TRUE((!missing.InternalPTR && !missing.Pointer_8 && !missing.GetCharacterBitmap(L'A') &&
        !missing.GetTextDimension(L"A",&w,&h,0) && !w && !h)) << "failed load leaves valid empty object";
    auto reject=[&](const Bytes& bytes) {
        const auto path=temp.write("invalid.fnt",bytes);
        auto* data=BitFont::LoadInternalData(path.c_str());
        const bool failed=!data; release(data); EXPECT_TRUE((failed)) << "invalid/truncated font must fail";
    };
    const auto good=fixture(false);
    for (auto size : {0u,27u,28u,28u+0x1ffffu,unsigned(good.size()-1)})
        reject(Bytes(good.begin(),good.begin()+size));
    auto bad=good; bad[0]=0; reject(bad);
    bad=good; bad[8]=0; reject(bad); // no row stride
    bad=good; bad[28+2*65]=5; reject(bad); // map beyond allocated glyphs
    bad=good; bad[24]=1; reject(bad); // undersized glyph record
    bad=good; bad[28+0x20000]=17; reject(bad); // width beyond stride
    bad=fixture(true); bad[28]=0xff; reject(bad); // bad range-table offset
    bad=fixture(true); bad.resize(95); reject(bad);
    // Font without X: original had uninitialized fallback bytes; core is blank.
    bad=good; bad[28+2*88]=0;
    const auto path=temp.write("no-x.fnt",bad);
    BitFont no_x(path.c_str());
    EXPECT_TRUE((no_x.InternalPTR && no_x.Pointer_8 && *static_cast<unsigned char*>(no_x.Pointer_8)==0)) << "missing X fallback is deterministic";
}
void high_index(Temp& temp) {
    Bytes data;
    for (auto v:{0x546e6f66u,1u,1u,1u,1u,32769u,2u}) word(data,v);
    data.resize(28+0x20000+32769*2,0);
    data[28+2*65]=1; data[28+2*65+1]=0x80;
    data[data.size()-2]=1; data.back()=0x80;
    const auto path=temp.write("high-index.fnt",data);
    BitFont font(path.c_str());
    EXPECT_TRUE((font.InternalPTR && font.GetCharacterBitmap(L'A') &&
        font.GetCharacterBitmap(L'A')[1]==0x80)) << "symbol index uses all unsigned 16 bits";
}
void real_font(const char* name) {
    BitFont font(name);
    EXPECT_TRUE((font.InternalPTR && font.GetCharacterBitmap(L'X'))) << "real font loads with X";
    int w=0,h=0;
    EXPECT_TRUE((font.GetTextDimension(L"Red Alert 2 \x4e2d\x6587",&w,&h,640) && w>0 && h>0)) << "real font measurement";
    std::array<short,640*64> pixels{}; BSurface surface(640,64,2,pixels.data());
    font.Lock(&surface); int x=0;
    for (auto c:std::wstring(L"Red Alert 2 \x4e2d\x6587")) x=font.Blit(c,x,0,-1);
    font.UnLock(&surface);
    unsigned ink=0; for (auto p:pixels) ink+=p!=0;
    EXPECT_TRUE((ink>0)) << "real glyphs render";
    std::cout<<"real font: "<<font.InternalPTR->Count<<" glyphs, "
        <<font.InternalPTR->ValidSymbolCount<<" mappings, text "<<w<<'x'<<h<<", ink "<<ink<<'\n';
}
void original_reference(const std::string& name, const std::filesystem::path& reference_path) {
    std::ifstream reference(reference_path);
    EXPECT_TRUE((bool(reference))) << "original BitFont reference is present";
    BitFont font(name.c_str());
    std::array<short,240> pixels{}; BSurface surface(24,10,2,pixels.data());
    char kind; unsigned measurements=0,draws=0;
    while (reference>>kind) {
        if (kind=='M') {
            int maximum,success,width,height; std::string encoded;
            reference>>maximum>>encoded>>success>>width>>height;
            std::wstring text;
            if (encoded!="-") {
                std::istringstream units(encoded); std::string unit;
                while (std::getline(units,unit,',')) text+=wchar_t(std::stoul(unit,nullptr,16));
            }
            int actual_width=-1,actual_height=-1;
            const bool actual=font.GetTextDimension(text.c_str(),&actual_width,&actual_height,maximum);
            if (actual!=bool(success) || actual_width!=width || actual_height!=height)
                throw std::runtime_error("original dimension mismatch at sample "+std::to_string(measurements));
            ++measurements;
        } else if (kind=='D') {
            unsigned code; int x,y,advance; LTRBStruct bounds; std::uint32_t expected;
            reference>>code>>x>>y>>bounds.Left>>bounds.Top>>bounds.Right>>bounds.Bottom>>advance>>expected;
            pixels.fill(0x1234); font.SetBounds(&bounds); font.Lock(&surface);
            const auto actual=font.Blit(wchar_t(code),x,y,0x4567);
            font.UnLock(&surface);
            std::uint32_t hash=2166136261u;
            for (auto pixel:pixels) {
                auto value=static_cast<std::uint16_t>(pixel);
                hash=(hash^(value&255u))*16777619u;
                hash=(hash^(value>>8))*16777619u;
            }
            if (actual!=advance || hash!=expected)
                throw std::runtime_error("original raster mismatch at sample "+std::to_string(draws));
            // Exercise the canvas path against the same original-machine-code
            // corpus, including missing glyphs, tabs and disjoint clipping.
            pixels.fill(0x1234);
            game::TypeDrawingContext drawing;
            drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(&surface);
            drawing.backend_context=&pixels;
            drawing.backend.raster=[](void* user,const game::RasterDrawingRequest& r) {
                auto& p=*static_cast<std::array<short,240>*>(user);
                EXPECT_TRUE((!r.pixels)) << "font submits opaque ink runs";
                for (int yy=r.position.Y;yy<r.position.Y+r.height;++yy)
                    for (int xx=r.position.X;xx<r.position.X+r.width;++xx) {
                        EXPECT_TRUE((xx>=0 && xx<24 && yy>=0 && yy<10)) << "glyph clips before submission";
                        p[yy*24+xx]=static_cast<short>(r.color);
                    }
                return game::DrawingStatus::drawn;
            };
            int canvas_advance=0;
            const auto status=font.SubmitGlyph(drawing,wchar_t(code),x,y,
                {bounds.Left,bounds.Top,bounds.Right-bounds.Left+1,bounds.Bottom-bounds.Top+1},0x4567,canvas_advance);
            EXPECT_TRUE((status==game::DrawingStatus::drawn || status==game::DrawingStatus::skipped)) << "canvas glyph submission";
            hash=2166136261u;
            for (auto pixel:pixels) {
                const auto value=static_cast<std::uint16_t>(pixel);
                hash=(hash^(value&255u))*16777619u; hash=(hash^(value>>8))*16777619u;
            }
            if (canvas_advance!=advance || hash!=expected)
                throw std::runtime_error("canvas glyph/original mismatch at sample "+std::to_string(draws));
            ++draws;
        } else throw std::runtime_error("unknown reference record");
        EXPECT_TRUE((bool(reference))) << "complete reference record";
    }
    EXPECT_TRUE((measurements==230 && draws==100)) << "original corpus sample counts";
}
}

TEST(Bitfont, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();


            Temp temp;
            lock_state_contracts();
            EXPECT_TRUE((BitFont::Instance==nullptr)) << "standalone singleton starts unbound";
            font_contracts(temp.write("dense.fnt",fixture(false)));
            font_contracts(temp.write("ranges.fnt",fixture(true)));
            failure_contracts(temp); high_index(temp);
            const auto reference_path = argc == 3 && std::string(argv[1]) == "--reference"
                ? std::filesystem::path(argv[2]) : std::filesystem::path(RA2_TEST_FIXTURE_DIR)/"bitfont_reference.txt";
            original_reference(temp.write("reference.fnt",fixture(false)), reference_path);
            if (argc==2) real_font(argv[1]);
}
