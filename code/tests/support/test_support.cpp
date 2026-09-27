#include "test_support.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ra2::test {
namespace {
Arguments saved_arguments{};
WideArguments saved_wide_arguments{};
CommandHandler command_handler = nullptr;
thread_local bool stop_on_failure = true;
bool explicit_gtest = false;

class FailureListener final : public testing::EmptyTestEventListener {
    void OnTestPartResult(const testing::TestPartResult& result) override {
        if (result.failed() && std::uncaught_exceptions() == 0 &&
            (stop_on_failure || result.fatally_failed())) {
            // GoogleTest catches AssertionException at the current test boundary;
            // the recorded failure cannot disappear if production catches it first.
            throw testing::AssertionException(result);
        }
    }
};

void install_listener() {
    testing::UnitTest::GetInstance()->listeners().Append(new FailureListener);
}

// Keep old --case IDs without maintaining a second list of registered tests.
bool select_case(std::string_view id) {
    const auto* unit = testing::UnitTest::GetInstance();
    for (int s = 0; s < unit->total_test_suite_count(); ++s) {
        const auto* suite = unit->GetTestSuite(s);
        for (int t = 0; t < suite->total_test_count(); ++t) {
            const auto* info = suite->GetTestInfo(t);
            const std::string_view name(info->name());
            if (name != id && name != "DISABLED_" + std::string(id)) continue;
            GTEST_FLAG_SET(filter, std::string(suite->name()) + "." + info->name());
            if (name.starts_with("DISABLED_")) GTEST_FLAG_SET(also_run_disabled_tests, true);
            return true;
        }
    }
    return false;
}

bool has_suite(std::string_view name) {
    const auto* unit = testing::UnitTest::GetInstance();
    for (int i = 0; i < unit->total_test_suite_count(); ++i)
        if (unit->GetTestSuite(i)->name() == name) return true;
    return false;
}
} // namespace

Arguments arguments() noexcept { return saved_arguments; }
WideArguments wide_arguments() noexcept { return saved_wide_arguments; }
bool gtest_requested() noexcept { return explicit_gtest; }
CommandRegistration::CommandRegistration(CommandHandler handler) {
    if (command_handler) throw std::logic_error("Only one diagnostic handler is allowed per executable");
    command_handler = handler;
}
ContinueAfterFailure::ContinueAfterFailure() noexcept : previous_(stop_on_failure) {
    stop_on_failure = false;
}
ContinueAfterFailure::~ContinueAfterFailure() { stop_on_failure = previous_; }

int run(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        explicit_gtest |= std::string_view(argv[i]).starts_with("--gtest_");
    testing::InitGoogleTest(&argc, argv);
    saved_arguments = {argc, argv};
    install_listener();
    // Discovery must not load game assets, construct legacy probes, emit files,
    // or execute an allocation-failure process. It only enumerates tests.
    if (GTEST_FLAG_GET(list_tests)) return RUN_ALL_TESTS();
    try {
        if (command_handler) {
            if (const auto status = command_handler(argc, argv)) return *status;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--case") {
            if (!select_case(argv[2])) {
                std::cerr << "Unknown case: " << argv[2] << '\n';
                return 2;
            }
        } else if (argc == 2 && std::string_view(argv[1]) == "--known-gaps" && has_suite("MapFidelity")) {
            GTEST_FLAG_SET(filter, "MapFidelity.DISABLED_G*");
            GTEST_FLAG_SET(also_run_disabled_tests, true);
        } else if (argc != 1 && (has_suite("MapFidelity") || has_suite("BuildingVisual"))) {
            std::cerr << "usage: test [--case ID | --known-gaps] [GoogleTest options]\n";
            return 2;
        }
        return RUN_ALL_TESTS();
    } catch (const std::exception& error) {
        // Only diagnostic commands reach this boundary in ordinary operation.
        std::cerr << error.what() << '\n';
        return 1;
    }
}

int run(int argc, wchar_t** argv) {
    testing::InitGoogleTest(&argc, argv);
    saved_wide_arguments = {argc, argv};
    install_listener();
    return RUN_ALL_TESTS();
}
} // namespace ra2::test
