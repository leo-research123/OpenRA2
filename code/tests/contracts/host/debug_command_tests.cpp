#include <gtest/gtest.h>
#include "api/debug_commands.hpp"
#include "map_debug_commands.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

using game::DebugCommandStatus;

static_assert(std::is_standard_layout_v<game::DebugCommandDescriptor>);
static_assert(std::is_trivially_copyable_v<game::DebugCommandDescriptor>);
static_assert(sizeof(game::DebugCommandDescriptor) == 344);
static_assert(offsetof(game::DebugCommandDescriptor, name) == 16);
static_assert(offsetof(game::DebugCommandDescriptor, success_message) == 216);
static_assert(sizeof(game::DebugCommandId) == 4);
static_assert(sizeof(game::DebugCommandStatus) == 4);
static_assert(sizeof(game::DebugTargetFailure) == 4);
#if defined(_MSC_VER) || INTPTR_MAX == INT64_MAX
static_assert(sizeof(game::DebugCommandResult) == 64);
static_assert(offsetof(game::DebugCommandResult, first_failed_target) == 48);
#endif
static_assert(std::is_standard_layout_v<game::DebugCommandResult>);
static_assert(std::is_trivially_copyable_v<game::DebugCommandResult>);
static_assert(std::is_standard_layout_v<game::DebugPerformanceSnapshot>);
static_assert(std::is_trivially_copyable_v<game::DebugPerformanceSnapshot>);
static_assert(offsetof(game::DebugCommandResult, command) == 16);
static_assert(offsetof(game::DebugCommandResult, first_failure) == 40);
using DebugEntry = DebugCommandStatus (*)(game::MapViewHandle*, std::uint64_t, const char*, std::uint32_t,
                                          game::DebugCommandResult&) noexcept;
static_assert(std::is_same_v<decltype(&game::execute_debug_command), DebugEntry>);

// Test-only commands exercise extension without changing the production catalog.
class ProbeDebugCommand final : public game::DebugConsoleCommand {
public:
    ProbeDebugCommand(const char* name, game::DebugCommandId id) {
        metadata_.id = id;
        std::snprintf(metadata_.name, sizeof(metadata_.name), "%s", name);
        std::snprintf(metadata_.usage, sizeof(metadata_.usage), "%s <number> <text> <enabled>", name);
    }
    const game::DebugCommandDescriptor& descriptor() const noexcept override { return metadata_; }
    DebugCommandStatus parse(game::DebugArgumentReader& reader,
                             game::DebugCommandArguments& arguments) const noexcept override {
        auto status = reader.read_integer(arguments, 0, 100);
        if (status != DebugCommandStatus::success) return status;
        status = reader.read_text(arguments);
        return status == DebugCommandStatus::success ? reader.read_boolean(arguments) : status;
    }
    DebugCommandStatus execute(game::DebugCommandContext& context,
                               const game::DebugCommandArguments& arguments) const override {
        std::int32_t number = 0;
        const char* text = nullptr;
        std::uint32_t length = 0;
        bool enabled = false;
        if (!arguments.get_integer(0, number) || !arguments.get_text(1, text, length) ||
            !arguments.get_boolean(2, enabled))
            return DebugCommandStatus::invalid_argument;
        // A successful read-only command does not require applied targets.
        context.result.value = number + static_cast<std::int32_t>(length) + (enabled ? 1 : 0);
        return DebugCommandStatus::success;
    }

private:
    game::DebugCommandDescriptor metadata_{};
};

TEST(DebugCommands, RuntimeRegistrationAndConflictIsolation) {
    const auto initial_count = game::debug_command_count();
    const auto probe_id = static_cast<game::DebugCommandId>(0x00010001u);
    ProbeDebugCommand probe("probe", probe_id);
    {
        game::DebugCommandRegistration registration(probe);
        EXPECT_EQ(game::debug_command_count(), initial_count + 1);
        EXPECT_EQ(game::find_debug_command("PROBE", 5), &probe);
        const auto rejects = [&](const char* name, game::DebugCommandId id) {
            ProbeDebugCommand conflict(name, id);
            {
                game::DebugCommandRegistration duplicate(conflict);
                EXPECT_FALSE(game::debug_command_registry_valid());
                game::DebugCommandDescriptor unchanged;
                unchanged.minimum = 999;
                EXPECT_FALSE(game::get_debug_command_descriptor(0, unchanged));
                EXPECT_EQ(unchanged.minimum, 999);
                game::ParsedDebugCommand parsed;
                EXPECT_EQ(game::parse_debug_command("power", 5, parsed), DebugCommandStatus::execution_failure);
            }
            EXPECT_TRUE(game::debug_command_registry_valid());
            EXPECT_EQ(game::find_debug_command("probe", 5), &probe);
        };
        rejects("other", probe_id);
        rejects("probe", static_cast<game::DebugCommandId>(0x00010002u));
        rejects("help", static_cast<game::DebugCommandId>(0x00010002u));
        rejects("clear", static_cast<game::DebugCommandId>(0x00010002u));
        rejects("BadName", static_cast<game::DebugCommandId>(0x00010002u));
        rejects("zero", game::DebugCommandId::none);
    }
    EXPECT_EQ(game::debug_command_count(), initial_count);
    EXPECT_EQ(game::find_debug_command("probe", 5), nullptr);
}

TEST(DebugCommands, MetadataAndStrictParser) {
    for (std::uint32_t i = 0; i < game::debug_command_count(); ++i) {
        game::DebugCommandDescriptor descriptor;
        ASSERT_TRUE(game::get_debug_command_descriptor(i, descriptor));
        EXPECT_NE(descriptor.id, game::DebugCommandId::none);
        EXPECT_NE(descriptor.name[0], 0);
        EXPECT_NE(descriptor.usage[0], 0);
        EXPECT_NE(descriptor.description[0], 0);
    }
    game::DebugCommandDescriptor unchanged;
    unchanged.minimum = 999;
    EXPECT_FALSE(game::get_debug_command_descriptor(game::debug_command_count(), unchanged));
    EXPECT_EQ(unchanged.minimum, 999);
    struct Case {
        const char* text;
        DebugCommandStatus status;
        int value;
    };
    const Case cases[] = {
        {"power", DebugCommandStatus::success, 0},
        {" \tLeVeL\t+2\r\n", DebugCommandStatus::success, 2},
        {"hp 1000000", DebugCommandStatus::success, 1000000},
        {"hp -0", DebugCommandStatus::success, 0},
        {"", DebugCommandStatus::invalid_command, 0},
        {"wat", DebugCommandStatus::invalid_command, 0},
        {"power 1", DebugCommandStatus::invalid_argument, 0},
        {"level", DebugCommandStatus::invalid_argument, 0},
        {"level 3", DebugCommandStatus::invalid_argument, 0},
        {"hp -1", DebugCommandStatus::invalid_argument, 0},
        {"hp 1000001", DebugCommandStatus::invalid_argument, 0},
        {"level 1 2", DebugCommandStatus::invalid_argument, 0},
        {"level 2x", DebugCommandStatus::invalid_argument, 0},
        {"hp 2147483648", DebugCommandStatus::invalid_argument, 0},
        {"hp +-1", DebugCommandStatus::invalid_argument, 0},
        {"hp ++1", DebugCommandStatus::invalid_argument, 0},
    };
    for (const auto& sample : cases) {
        game::ParsedDebugCommand parsed;
        EXPECT_EQ(game::parse_debug_command(sample.text, std::strlen(sample.text), parsed), sample.status)
            << sample.text;
        EXPECT_EQ(parsed.value, sample.value) << sample.text;
    }
    game::ParsedDebugCommand parsed;
    const char embedded[] = {'l', 'i', 'f', 'e', ' ', '1', 0, '2'};
    EXPECT_EQ(game::parse_debug_command(embedded, sizeof(embedded), parsed), DebugCommandStatus::invalid_argument);
    const std::string oversized(513, ' ');
    EXPECT_EQ(game::parse_debug_command(oversized.data(), oversized.size(), parsed),
              DebugCommandStatus::invalid_argument);
    EXPECT_EQ(game::parse_debug_command(nullptr, 0, parsed), DebugCommandStatus::invalid_argument);
}

TEST(DebugCommands, ExtendedArgumentsRejectMalformedAndNonFiniteValues) {
    const char* valid[] = {"fog",         "fog 0",     "fog 1",        "tp",          "tp 1",  "perf",   "perf 0",
                           "perf 1",      "money",     "money +10000", "gamespeed 9", "xp .5", "xp 1.5", "xp +2e0",
                           "atk 1000000", "atk reset", "select 3",     "control 2",   "perf",  "god",
                           "radar", "radar 0", "RADAR 1"};
    game::ParsedDebugCommand parsed;
    for (auto* text : valid)
        EXPECT_EQ(game::parse_debug_command(text, std::strlen(text), parsed), DebugCommandStatus::success) << text;
    const char* invalid[] = {
        "fog 2",       "tp -1",        "tp 1 0",   "perf 2", "perf 1 0", "money 0", "money -1",    "money 2147483648",
        "gamespeed 0", "gamespeed 10", "xp",       "xp nan", "xp inf",   "xp 0x1",  "xp 1e9999",   "xp 1e-9999",
        "xp +-1",      "xp --1",       "xp 1.2.3", "xp 1 2", "xp -1",    "atk 0",   "atk reset 1", "select 0",
        "control 4",   "god 1", "radar -1", "radar 2", "radar 1 0", "radar on"};
    for (auto* text : invalid)
        EXPECT_EQ(game::parse_debug_command(text, std::strlen(text), parsed), DebugCommandStatus::invalid_argument)
            << text;
    EXPECT_EQ(game::parse_debug_command("life 10", 7, parsed), DebugCommandStatus::invalid_command);
}
