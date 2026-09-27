#pragma once
#include <string>
#include <vector>
namespace game {
// FileSearchPath avoids the Win32 SearchPath macro regardless of include order.
// Same linked search records as the original CD service, outside file objects.
struct FileSearchPath { FileSearchPath* next; const char* path; };
FileSearchPath*& file_search_paths() noexcept;
void set_file_search_paths(const std::vector<std::string>& paths);
void clear_file_search_paths() noexcept;
}
