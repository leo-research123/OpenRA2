// Installed movie-package portion of original startup 0x00530460.
#include "yrpp/MixFileClass.h"
#include "yrpp/ArrayClasses.h"
#include "filesystem/resource_environment.hpp"
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

bool MixFileClass::LoadMovieMixes() noexcept {
    const auto* context = game::try_current_context();
    if (!context || !context->files) return false;
    if (Generics.MOVIES02D || Movies.Count) return true;
    auto& files = *context->files;
    std::vector<MixFileClass*> created;
    try {
        std::vector<std::string> md, base;
        for (const auto& entry : std::filesystem::directory_iterator(files.directory())) {
            if (!entry.is_regular_file()) continue;
            auto name = game::path_to_utf8(entry.path().filename());
            auto upper = name;
            for (auto& c : upper)
                if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
            if (!upper.ends_with(".MIX")) continue;
            if (!upper.starts_with("MOVMD") && !upper.starts_with("MOVIES")) continue;
            // Launcher placeholders cannot contain even the six-byte MIX header.
            if (entry.file_size() < 6) continue;
            if (upper.starts_with("MOVMD"))
                md.push_back(name);
            else if (upper.starts_with("MOVIES"))
                base.push_back(name);
        }
        auto& names = md.empty() ? base : md;
        std::sort(names.begin(), names.end());
        created.reserve(names.size());
        for (const auto& name : names) {
            // A launcher placeholder such as "CLASS" is not a MIX package.
            // It must not hide movies already supplied by expansion archives.
            try {
                auto* mix = &files.mount(name);
                created.push_back(mix);
                if (!Generics.MOVIES02D)
                    Generics.MOVIES02D = mix;
                else if (!Movies.AddItem(mix))
                    throw std::bad_alloc();
            } catch (const std::bad_alloc&) {
                throw;
            } catch (const std::exception&) {
                continue;
            }
        }
        return true;
    } catch (...) {
        for (auto* mix : created)
            files.unmount(*mix);
        return false;
    }
}
