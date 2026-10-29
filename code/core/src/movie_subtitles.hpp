#pragma once
#include "yrpp/OwnerDraw.h"
#include <memory>
#include <vector>
namespace game {
// Internal implementation of the existing movie subtitle responsibility.
bool movie_subtitle_time(const wchar_t*, unsigned&) noexcept;
bool parse_movie_subtitle(wchar_t*, SubTitle&) noexcept;
void release_movie_subtitle(SubTitle&) noexcept;
void YRPP_THISCALL sidebar_subtitle_message(SubTitle*) noexcept;
class MovieSubtitles {
public:
    ~MovieSubtitles();
    bool load(const char*) noexcept;
    void advance(unsigned, MovieSubtitleCallback) noexcept;
    void rewind() noexcept {
        next_ = 0;
        current_ = nullptr;
    }

private:
    std::vector<std::unique_ptr<SubTitle>> entries_;
    std::size_t next_ = 0;
    SubTitle* current_ = nullptr;
};
}
