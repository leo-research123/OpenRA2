#pragma once
// Internal INI parsing shared by the original object readers. No STL values
// cross their public status/diagnostic interfaces.
#include "yrpp/CCINIClass.h"
#include "yrpp/HouseClass.h"
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace game {
inline std::string_view scenario_trim(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
    s.remove_prefix(1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
    s.remove_suffix(1);
  return s;
}
inline std::vector<std::string> scenario_fields(const char *value) {
  std::vector<std::string> out;
  std::string_view rest = value ? value : "";
  for (;;) {
    const auto comma = rest.find(',');
    out.emplace_back(scenario_trim(rest.substr(0, comma)));
    if (comma == rest.npos)
      return out;
    rest.remove_prefix(comma + 1);
    if (out.size() > 64)
      throw std::runtime_error("Map record has too many fields");
  }
}
inline bool scenario_number(std::string_view text, int &out) {
  text = scenario_trim(text);
  if (text.empty())
    return false;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), out);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
inline int scenario_integer(const std::vector<std::string> &row,
                            std::size_t index, int fallback) {
  int value;
  return index < row.size() && scenario_number(row[index], value) ? value
                                                                  : fallback;
}
inline HouseClass *scenario_house(const char *name, int first) {
  for (int i = first; i < HouseClass::Array.Count; ++i)
    if (!_strcmpi(HouseClass::Array[i]->PlainName, name))
      return HouseClass::Array[i];
  return nullptr;
}
} // namespace game
