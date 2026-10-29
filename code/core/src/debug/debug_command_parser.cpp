#include "debug/debug_command.hpp"
#include "map_debug_commands.hpp"
#include <charconv>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <locale.h>
#if defined(__APPLE__)
#include <xlocale.h>
#endif

namespace game {
static bool debug_space(char value) noexcept { return value == ' ' || value == '\t' || value == '\r' || value == '\n'; }

bool DebugCommandArguments::get_integer(std::uint32_t index, std::int32_t& output) const noexcept {
    if (index >= count || values[index].kind != DebugValueKind::integer) return false;
    output = values[index].integer;
    return true;
}
bool DebugCommandArguments::get_boolean(std::uint32_t index, bool& output) const noexcept {
    if (index >= count || values[index].kind != DebugValueKind::boolean) return false;
    output = values[index].integer != 0;
    return true;
}
bool DebugCommandArguments::get_real(std::uint32_t index, double& output) const noexcept {
    if (index >= count || values[index].kind != DebugValueKind::real) return false;
    output = values[index].real;
    return true;
}

DebugCommandStatus DebugArgumentReader::read_real(DebugCommandArguments& arguments, double minimum,
                                                  double maximum) noexcept {
    char buffer[debug_command_max_bytes + 1];
    std::uint32_t length = 0;
    bool quoted = false;
    if (arguments.count == DebugCommandArguments::capacity || !token(buffer, length, quoted) || quoted || !length)
        return DebugCommandStatus::invalid_argument;
    const char* begin = buffer;
    if (*begin == '+') {
        ++begin;
        if (begin != buffer + length && (*begin == '+' || *begin == '-')) return DebugCommandStatus::invalid_argument;
    }
    if (begin == buffer + length || *begin == '+') return DebugCommandStatus::invalid_argument;
    // Floating from_chars is unavailable on the supported macOS deployment
    // target. A private C numeric locale avoids changing the process locale.
#if defined(_WIN32)
    static const auto locale = _create_locale(LC_NUMERIC, "C");
#else
    static const auto locale = newlocale(LC_NUMERIC_MASK, "C", nullptr);
#endif
    if (!locale) return DebugCommandStatus::resource_failure;
    for (const char* c = begin; c != buffer + length; ++c)
        if (!(*c >= '0' && *c <= '9') && *c != '.' && *c != 'e' && *c != 'E' && *c != '+' && *c != '-')
            return DebugCommandStatus::invalid_argument;
    char* end = nullptr;
    errno = 0;
#if defined(_WIN32)
    const double value = _strtod_l(begin, &end, locale);
#else
    const double value = strtod_l(begin, &end, locale);
#endif
    if (errno == ERANGE || end == begin || end != buffer + length || !std::isfinite(value) || value < minimum ||
        value > maximum)
        return DebugCommandStatus::invalid_argument;
    auto& argument = arguments.values[arguments.count++];
    argument.kind = DebugValueKind::real;
    argument.real = value;
    return DebugCommandStatus::success;
}
bool DebugCommandArguments::get_text(std::uint32_t index, const char*& output, std::uint32_t& length) const noexcept {
    if (index >= count || values[index].kind != DebugValueKind::text) return false;
    output = text + values[index].offset;
    length = values[index].length;
    return true;
}

DebugArgumentReader::DebugArgumentReader(const char* text, std::uint32_t length) noexcept
    : text_(text), length_(length) {}

bool DebugArgumentReader::token(char* output, std::uint32_t& length, bool& quoted) noexcept {
    length = 0;
    quoted = false;
    if (!text_ || length_ > debug_command_max_bytes) return false;
    while (position_ < length_ && debug_space(text_[position_]))
        ++position_;
    if (position_ == length_) return false;
    quoted = text_[position_] == '"';
    if (quoted) ++position_;
    while (position_ < length_) {
        char value = text_[position_++];
        if (!value) return false;
        if (quoted && value == '"') {
            if (position_ < length_ && !debug_space(text_[position_])) return false;
            output[length] = 0;
            return true;
        }
        if (!quoted && debug_space(value)) {
            output[length] = 0;
            return true;
        }
        if (!quoted && (value == '"' || value == '\\')) return false;
        if (quoted && value == '\\') {
            if (position_ == length_) return false;
            value = text_[position_++];
            if (value != '"' && value != '\\') return false;
        }
        output[length++] = value;
    }
    output[length] = 0;
    return !quoted;
}

DebugCommandStatus DebugArgumentReader::read_integer(DebugCommandArguments& arguments, std::int32_t minimum,
                                                     std::int32_t maximum) noexcept {
    char buffer[debug_command_max_bytes + 1];
    std::uint32_t length = 0;
    bool quoted = false;
    if (arguments.count == DebugCommandArguments::capacity || !token(buffer, length, quoted) || quoted || !length ||
        minimum > maximum)
        return DebugCommandStatus::invalid_argument;
    const char* begin = buffer;
    if (*begin == '+') {
        ++begin;
        if (length == 1 || *begin == '+' || *begin == '-') return DebugCommandStatus::invalid_argument;
    }
    std::int32_t value = 0;
    const auto parsed = std::from_chars(begin, buffer + length, value);
    if (parsed.ec != std::errc{} || parsed.ptr != buffer + length || value < minimum || value > maximum)
        return DebugCommandStatus::invalid_argument;
    arguments.values[arguments.count++] = {DebugValueKind::integer, value};
    return DebugCommandStatus::success;
}

DebugCommandStatus DebugArgumentReader::read_boolean(DebugCommandArguments& arguments) noexcept {
    char buffer[debug_command_max_bytes + 1];
    std::uint32_t length = 0;
    bool quoted = false;
    if (arguments.count == DebugCommandArguments::capacity || !token(buffer, length, quoted) || quoted)
        return DebugCommandStatus::invalid_argument;
    const bool yes = !std::strcmp(buffer, "true") || !std::strcmp(buffer, "1");
    if (!yes && std::strcmp(buffer, "false") && std::strcmp(buffer, "0")) return DebugCommandStatus::invalid_argument;
    arguments.values[arguments.count++] = {DebugValueKind::boolean, yes ? 1 : 0};
    return DebugCommandStatus::success;
}

DebugCommandStatus DebugArgumentReader::read_text(DebugCommandArguments& arguments) noexcept {
    char buffer[debug_command_max_bytes + 1];
    std::uint32_t length = 0;
    bool quoted = false;
    if (arguments.count == DebugCommandArguments::capacity || !token(buffer, length, quoted) ||
        arguments.text_length + length + 1 > sizeof(arguments.text))
        return DebugCommandStatus::invalid_argument;
    arguments.values[arguments.count++] = {DebugValueKind::text, 0, arguments.text_length, length};
    std::memcpy(arguments.text + arguments.text_length, buffer, length + 1);
    arguments.text_length += length + 1;
    return DebugCommandStatus::success;
}

DebugCommandStatus DebugArgumentReader::finish() const noexcept {
    if (!text_ || length_ > debug_command_max_bytes) return DebugCommandStatus::invalid_argument;
    for (auto index = position_; index < length_; ++index)
        if (!debug_space(text_[index])) return DebugCommandStatus::invalid_argument;
    return DebugCommandStatus::success;
}

DebugCommandStatus parse_debug_command(const char* text, std::uint32_t length, ParsedDebugCommand& output) noexcept {
    output = {};
    if (!text || length > debug_command_max_bytes || std::memchr(text, 0, length))
        return DebugCommandStatus::invalid_argument;
    if (!debug_command_registry_valid()) return DebugCommandStatus::execution_failure;
    std::uint32_t start = 0, end = 0;
    while (start < length && debug_space(text[start]))
        ++start;
    end = start;
    while (end < length && !debug_space(text[end]))
        ++end;
    output.command = find_debug_command(text + start, end - start);
    if (!output.command) return DebugCommandStatus::invalid_command;
    DebugArgumentReader reader(text + end, length - end);
    const auto status = output.command->parse(reader, output.arguments);
    if (status != DebugCommandStatus::success) return status;
    if (reader.finish() != DebugCommandStatus::success) return DebugCommandStatus::invalid_argument;
    output.arguments.get_integer(0, output.value);
    return DebugCommandStatus::success;
}
}
