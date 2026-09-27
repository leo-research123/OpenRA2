#include "search_paths.hpp"
#include <memory>
namespace game {
namespace {
FileSearchPath* first = nullptr;
std::vector<std::string> names;
std::vector<std::unique_ptr<FileSearchPath>> records;
}
FileSearchPath*& file_search_paths() noexcept { return first; }
void clear_file_search_paths() noexcept { first = nullptr; records.clear(); names.clear(); }
void set_file_search_paths(const std::vector<std::string>& paths) {
    clear_file_search_paths();
    names = paths;
    for (auto& name : names) {
        if (!name.empty() && name.back() != '/' && name.back() != '\\') name += '/';
        records.push_back(std::make_unique<FileSearchPath>(FileSearchPath{nullptr, name.c_str()}));
        if (records.size() > 1) records[records.size() - 2]->next = records.back().get();
    }
    first = records.empty() ? nullptr : records.front().get();
}
}
