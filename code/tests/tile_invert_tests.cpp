#include "support/test_support.hpp"
#include "experiments/tile_half_invert.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

struct Canvas {
    static constexpr int width=100, height=80;
    std::vector<std::uint16_t> pixels = std::vector<std::uint16_t>(width*height, 0x2468);
    void*& palette;
    bool throw_on_left=false;
    int calls=0;
    explicit Canvas(void*& colors) : palette(colors) {}
    static void draw(void* context, const game::TileDrawArguments& tile) {
        auto& self=*static_cast<Canvas*>(context); ++self.calls;
        EXPECT_TRUE((tile.intensity==1000 && tile.level==3 && tile.use_z && tile.alternate==2)) << "original non-clipping arguments must be preserved";
        if(self.throw_on_left && tile.clip.X < tile.x+30) throw std::runtime_error("draw failure");
        for(int y=0; y<29; ++y) {
            const int length=4*(y<15 ? y+1 : 29-y);
            const int start=(60-length)/2;
            for(int x=start; x<start+length; ++x) {
                const int sx=tile.x+x, sy=tile.y+y;
                if(sx<0 || sy<0 || sx>=width || sy>=height || sx<tile.clip.X || sy<tile.clip.Y
                    || sx>=tile.clip.X+tile.clip.Width || sy>=tile.clip.Y+tile.clip.Height) continue;
                self.pixels[sy*width+sx]=static_cast<std::uint16_t*>(self.palette)[1+(x+y*7)%255];
            }
        }
    }
};
TEST(TileInvert, Contracts) {
    auto scratch=std::make_unique<game::TileInvertScratch>();
    std::array<std::uint16_t,256> colors{};
    for(unsigned i=0;i<256;++i) colors[i]=std::uint16_t(i*197u);
    void* palette=colors.data();
    int cases=0;
    for(std::uint16_t mask : {std::uint16_t(0xffff),std::uint16_t(0x7fff)})
    for(int x : {-50,-20,0,10,70,110})
    for(int y : {-20,0,15,70})
    for(RectangleStruct clip : {RectangleStruct{0,0,100,80}, {22,8,40,25}, {10,10,1,30}}) {
        game::TileDrawArguments args{};
        args.x=x;args.y=y;args.clip=clip;args.level=3;args.intensity=1000;args.use_z=true;args.alternate=2;
        Canvas expected(palette), actual(palette);
        Canvas::draw(&expected,args);
        game::draw_tile_half_inverted(args,palette,2,1,mask,*scratch,Canvas::draw,&actual);
        EXPECT_TRUE((palette==colors.data())) << "palette ownership restored after drawing";
        for(int sy=0;sy<Canvas::height;++sy) for(int sx=0;sx<Canvas::width;++sx) {
            const auto original=expected.pixels[sy*Canvas::width+sx];
            const bool drawn=original!=0x2468;
            const auto want=std::uint16_t(original ^ (drawn && sx<x+30 ? mask : 0));
            EXPECT_TRUE((actual.pixels[sy*Canvas::width+sx]==want)) << "tile-local split must preserve right pixels, clipping and background";
        }
        ++cases;
    }
    game::TileDrawArguments args{};
    args.x=10;args.y=10;args.clip={0,0,100,80};args.level=3;args.intensity=1000;args.use_z=true;args.alternate=2;
    Canvas first(palette), changed(palette), repeated(palette);
    game::draw_tile_half_inverted(args,palette,2,1,0xffff,*scratch,Canvas::draw,&first);
    colors[1+(29+14*7)%255]^=0x3456;
    game::draw_tile_half_inverted(args,palette,2,1,0xffff,*scratch,Canvas::draw,&changed);
    EXPECT_TRUE((first.pixels[24*100+39]!=changed.pixels[24*100+39])) << "same-address palette update invalidates cached inversion";
    repeated.pixels=changed.pixels;
    game::draw_tile_half_inverted(args,palette,2,1,0xffff,*scratch,Canvas::draw,&repeated);
    EXPECT_TRUE((repeated.pixels==changed.pixels)) << "redraw must not toggle already inverted terrain";
    Canvas failing(palette);failing.throw_on_left=true; bool threw=false;
    try { game::draw_tile_half_inverted(args,palette,2,1,0xffff,*scratch,Canvas::draw,&failing); }
    catch(const std::runtime_error&) { threw=true; }
    EXPECT_TRUE((threw && palette==colors.data())) << "exception restores original color table";
    for(int bytes : {1,4}) {
        Canvas fallback(palette), normal(palette);
        Canvas::draw(&normal,args);
        EXPECT_TRUE((!game::draw_tile_half_inverted(args,palette,bytes,1,0xffff,*scratch,Canvas::draw,&fallback)
            && fallback.calls==1 && fallback.pixels==normal.pixels)) << "unsupported pixel format preserves original call";
    }
    std::cout<<"PASS: "<<cases<<" clipped tile/RGB-format cases, palette mutation, redraw, restoration and fallback\n";
}
}

