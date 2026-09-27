// Original 5307AA..530A1E installed-directory branch and 530B7F..530BE0.
// Physical directory enumeration is provided by the host C++ filesystem;
// original MIXes, Maps and generic slots retain all lookup authority.
#include "yrpp/MixFileClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/ArrayClasses.h"
#include "filesystem/resource_environment.hpp"
#include <filesystem>
#include <string>
#include <vector>

bool MixFileClass::LoadMapMixes() noexcept {
    const auto* context=game::try_current_context();
    if (!context || !context->files) return false;
    auto& files=*context->files;
    std::vector<MixFileClass*> created;
    try {
        if (!Generics.MAPSMD02D && !Generics.MAPS02D) {
            std::vector<std::string> md,base;
            for (const auto& entry : std::filesystem::directory_iterator(files.directory())) {
                if (!entry.is_regular_file()) continue;
                auto name=game::path_to_utf8(entry.path().filename());
                auto upper=name;
                for (auto& c : upper) if (c>='a' && c<='z') c=static_cast<char>(c-'a'+'A');
                if (!upper.ends_with(".MIX")) continue;
                if (upper.starts_with("MAPSMD")) md.push_back(name);
                else if (upper.starts_with("MAPS")) base.push_back(name);
            }
            const auto& names=md.empty() ? base : md;
            created.reserve(names.size()+1);
            for (std::size_t i=0;i<names.size();++i) {
                auto* mix=&files.mount(names[i]); created.push_back(mix);
                if (!i) (md.empty() ? Generics.MAPS02D : Generics.MAPSMD02D)=mix;
                else if (!Maps.AddItem(mix)) throw 1;
            }
        } else created.reserve(1);
        if (!MULTIMD) {
            CCFileClass file("MULTIMD.MIX");
            if (file.Exists()) {
                auto* mix=&files.mount("MULTIMD.MIX"); created.push_back(mix); MULTIMD=mix;
            }
        }
        return true;
    } catch (...) {
        for (auto* mix : created) if (mix) files.unmount(*mix);
        return false;
    }
}
