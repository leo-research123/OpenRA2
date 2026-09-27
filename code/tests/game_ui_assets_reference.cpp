// Read original resource headers through the production MIX/file chain.
#include "api/filesystem.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/Memory.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

int main(int argc,char** argv) {
    if (argc!=2) return 2;
    game::ResourceHandle* resource=nullptr; std::string error;
    if (!game::create_resources(argv[1],resource,error)) { std::cerr<<error; return 1; }
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> owner(resource,game::destroy_resources);
    if (game::load_resources(*resource,{},error)!=game::ResourceLoadResult::complete) { std::cerr<<error; return 1; }
    const bool ok=game::with_resources(*resource,[](void*) {
        const char* names[]{"SIDEBTTN.SHP","CREDITS.SHP","TOP.SHP","RADAR.SHP","RADARY.SHP",
            "SIDE1.SHP","SIDE2.SHP","SIDE2B.SHP","SIDE3.SHP","ADDON.SHP","LSPACER.SHP","LENDCAP.SHP",
            "BTTNBKGD.SHP","RENDCAP.SHP","SIDEBAR.PAL","BUTTON00.SHP","BUTTON24.SHP"};
        for (int side=0;side<3;++side) {
            if (!MixFileClass::LoadSidebarMixes(side)) throw 1;
            for (const auto* asset:names) {
                CCFileClass file(asset); unsigned short header[4]{};
                if (!file.Exists() || file.ReadBytes(header,8)!=8) { std::cout<<side<<' '<<asset<<" MISSING\n"; continue; }
                std::cout<<side<<' '<<asset<<' '<<header[0]<<' '<<header[1]<<' '<<header[2]<<' '<<header[3]<<'\n';
            }
        }
        MixFileClass::UnloadSidebarMixes();
    },nullptr,error);
    if (!ok) { std::cerr<<error; return 1; }
}
