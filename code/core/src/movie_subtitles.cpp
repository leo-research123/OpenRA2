// Subtitle parsing and consumption: 0x006CA310 / 0x006CA5A0 / 0x006C9C60.
#include "movie_subtitles.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/MessageListClass.h"
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>

namespace game {
namespace {
wchar_t* trim(wchar_t* text) noexcept {
    auto* begin = text;
    while (*begin && *begin <= 0x20)
        ++begin;
    const auto length = std::wcslen(begin);
    if (begin != text) std::memmove(text, begin, (length + 1) * sizeof(wchar_t));
    auto end = length;
    while (end && text[end - 1] <= 0x20)
        --end;
    text[end] = 0;
    return text;
}
bool equal(const wchar_t* a, const wchar_t* b) noexcept {
    for (; *a && *b; ++a, ++b) {
        const auto fold = [](wchar_t c) { return c >= L'A' && c <= L'Z' ? c + 0x20 : c; };
        if (fold(*a) != fold(*b)) return false;
    }
    return *a == *b;
}
}
bool movie_subtitle_time(const wchar_t* source, unsigned& output) noexcept {
    if (!source) return false;
    wchar_t copy[12]{};
    std::wcsncpy(copy, source, 11);
    wchar_t* parts[4] = {copy};
    for (int i = 1; i < 4; ++i) {
        auto* colon = std::wcschr(parts[i - 1], L':');
        if (!colon) return false;
        *colon = 0;
        parts[i] = colon + 1;
    }
    const auto number = [](const wchar_t* p) { return unsigned(std::wcstol(p, nullptr, 10)); };
    output =
        60u * number(parts[3]) / 100u + 60u * (number(parts[2]) + 60u * (number(parts[1]) + 60u * number(parts[0])));
    return true;
}
void release_movie_subtitle(SubTitle& entry) noexcept {
    std::free(entry.Text);
    entry.Text = nullptr;
}
bool parse_movie_subtitle(wchar_t* source, SubTitle& entry) noexcept {
    if (!source) return false;
    auto* cursor = source;
    for (;;) {
        auto* split = std::wcschr(cursor, L'=');
        if (!split) return false;
        *split = 0;
        trim(cursor);
        auto* name = cursor;
        auto* value = split + 1;
        trim(value);
        if (*value == L'"') {
            ++value;
            auto* quote = std::wcschr(value, L'"');
            if (!quote) return false;
            *quote = 0;
            cursor = trim(quote + 1);
            if (*cursor == L',') ++cursor;
        } else {
            auto* end = std::wcspbrk(value, L", ");
            if (end) {
                *end = 0;
                cursor = end + 1;
            } else
                cursor = value + std::wcslen(value);
        }
        if (!*name) return false;
        unsigned time = 0;
        if (equal(name, L"Time")) {
            if (!movie_subtitle_time(value, time)) return false;
            entry.Time = time;
        } else if (equal(name, L"Duration")) {
            if (!movie_subtitle_time(value, time)) return false;
            if (time) entry.Duration = time;
        } else if (equal(name, L"Position")) {
            auto* colon = std::wcschr(value, L':');
            if (colon) {
                *colon = 0;
                entry.Line = int(std::wcstol(value, nullptr, 0));
                value = colon + 1;
            }
            entry.Alignment = equal(value, L"Left") ? 0 : equal(value, L"Right") ? 1 : 2;
        } else if (equal(name, L"Color")) {
            auto* g = std::wcschr(value, L':');
            if (!g) return false;
            *g++ = 0;
            auto* b = std::wcschr(g, L':');
            if (!b) return false;
            *b++ = 0;
            entry.Color = ((unsigned(std::wcstol(value, nullptr, 10)) & 0xFF) << 16) |
                          ((unsigned(std::wcstol(g, nullptr, 10)) & 0xFF) << 8) |
                          (unsigned(std::wcstol(b, nullptr, 10)) & 0xFF);
        } else if (equal(name, L"Text")) {
            const auto count = std::wcslen(value) + 1;
            auto* copy = static_cast<wchar_t*>(std::malloc(count * sizeof(wchar_t)));
            if (!copy) return false;
            std::memcpy(copy, value, count * sizeof(wchar_t));
            release_movie_subtitle(entry);
            entry.Text = copy;
        }
        trim(cursor);
        if (!*cursor) return true;
    }
}
void YRPP_THISCALL sidebar_subtitle_message(SubTitle* entry) noexcept {
    // 0x005BF360: the sidebar uses messages, not the cinematic RGB/position.
    if (entry)
        MessageListClass::Instance.AddMessage(nullptr, 0, entry->Text, 0, static_cast<TextPrintType>(0x4046),
                                              int(entry->Duration), false);
}
MovieSubtitles::~MovieSubtitles() {
    for (auto& entry : entries_)
        release_movie_subtitle(*entry);
}
bool MovieSubtitles::load(const char* movie) noexcept {
    try {
        CCFileClass file("SUBTITLEMD.TXT");
        if (!file.Exists()) file.SetFileName("SUBTITLE.TXT");
        if (!file.Open(FileAccessMode::Read)) return false;
        const auto size = file.GetFileSize();
        if (size < 2 || size > 16 * 1024 * 1024 || size % 2) return false;
        std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
        if (file.ReadBytes(bytes.data(), size) != size || bytes[0] != 0xFF || bytes[1] != 0xFE) return false;
        std::wstring text;
        text.reserve(bytes.size() / 2);
        for (std::size_t i = 2; i < bytes.size(); i += 2) {
            unsigned unit = bytes[i] | (unsigned(bytes[i + 1]) << 8);
            if constexpr (sizeof(wchar_t) > 2) {
                if (unit >= 0xD800 && unit <= 0xDBFF && i + 3 < bytes.size()) {
                    const unsigned low = bytes[i + 2] | (unsigned(bytes[i + 3]) << 8);
                    if (low >= 0xDC00 && low <= 0xDFFF) {
                        unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
                        i += 2;
                    }
                }
            }
            text.push_back(wchar_t(unit));
        }
        std::wstring stem;
        for (const char* p = movie; *p && *p != '.'; ++p)
            stem.push_back(wchar_t(static_cast<unsigned char>(*p)));
        bool selected = false;
        std::size_t start = 0;
        while (start < text.size()) {
            const auto end = text.find(L'\n', start);
            auto line = text.substr(start, end == std::wstring::npos ? text.size() - start : end - start);
            start = end == std::wstring::npos ? text.size() : end + 1;
            trim(line.data());
            line.resize(std::wcslen(line.c_str()));
            if (line.empty() || line[0] == L';') continue;
            if (!selected) {
                if (line.size() > 10) {
                    auto prefix = line.substr(0, 10);
                    if (equal(prefix.c_str(), L"BeginMovie")) selected = equal(trim(line.data() + 10), stem.c_str());
                }
                continue;
            }
            if (line.size() >= 2 && equal(line.substr(0, 2).c_str(), L"En")) break;
            if (line.size() < 2 || line.front() != L'<' || line.back() != L'>') continue;
            line.back() = 0;
            auto entry = std::make_unique<SubTitle>();
            if (parse_movie_subtitle(trim(line.data() + 1), *entry)) {
                try {
                    entries_.push_back(std::move(entry));
                } catch (...) {
                    if (entry) release_movie_subtitle(*entry);
                    throw;
                }
            } else
                release_movie_subtitle(*entry);
        }
        return !entries_.empty();
    } catch (...) {
        return false;
    }
}
void MovieSubtitles::advance(unsigned time, MovieSubtitleCallback callback) noexcept {
    // Keep file order. Fast-forward consumes every due entry exactly once.
    while (next_ < entries_.size() && entries_[next_]->Time <= time) {
        current_ = entries_[next_++].get();
        if (callback) callback(current_);
    }
    if (current_ && time >= current_->Time + current_->Duration) current_ = nullptr;
}
}
