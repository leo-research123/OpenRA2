#include <gtest/gtest.h>
#include "api/runtime_diagnostics.hpp"
#include "runtime_diagnostics.hpp"
#include "yrpp/TagClass.h"
#include "yrpp/TagTypeClass.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
struct RuntimeLogFixture : testing::Test {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("ra2-runtime-log-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::path path = root / "runtime.log";
    void TearDown() override {
        game::configure_runtime_log(nullptr, nullptr);
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
    static std::string read(const std::filesystem::path& path) {
        std::ifstream stream(path);
        return std::string(std::istreambuf_iterator<char>(stream), {});
    }
};
}

TEST_F(RuntimeLogFixture, FlushesContextAndCopiedTagMetadataBeforeShutdown) {
    static_assert(noexcept(game::configure_runtime_log(nullptr, nullptr)));
    ASSERT_TRUE(game::configure_runtime_log(path.string().c_str(), "resources\n\"test\""));
    game::runtime_log_map("mission.map", 2, 1, "America", 42);
    game::runtime_log_tick(42, 120, false);
    TagTypeClass type("LOG_TAG");
    auto* tag = GameCreate<TagClass>(&type, CellStruct::Empty);
    ASSERT_NE(tag, nullptr);
    const auto id = tag->UniqueID;
    GameDelete(tag);
    // The logger must only use copied metadata after the object's lifetime.
    game::runtime_log_tag("after_delete", tag, 6);
    game::runtime_log_tick(42, 120, true);
    const auto contents = read(path);
    EXPECT_NE(contents.find("runtime_context_v1 run="), std::string::npos);
    EXPECT_NE(contents.find("resources=\"resources__test_\""), std::string::npos);
    EXPECT_NE(contents.find("map=\"mission.map\" mode=2 difficulty=1 player=\"America\""), std::string::npos);
    EXPECT_NE(contents.find("world=42 tick=120 event=tick_begin"), std::string::npos);
    const auto record = contents.find("event=after_delete");
    ASSERT_NE(record, std::string::npos);
    const auto detail = contents.substr(record, contents.find('\n', record) - record);
    EXPECT_NE(detail.find("known=1 alive=0 id=" + std::to_string(id)), std::string::npos);
    EXPECT_NE(detail.find("type=\"LOG_TAG\""), std::string::npos);
    EXPECT_NE(contents.find("event=tick_complete"), std::string::npos);
    ASSERT_TRUE(game::configure_runtime_log(nullptr, nullptr));
    EXPECT_FALSE(game::runtime_log_enabled());
    EXPECT_NE(read(path).find("event=session_end reason=close"), std::string::npos);
    const auto bytes = std::filesystem::file_size(path);
    game::runtime_log_event("disabled", "%s", "ignored");
    EXPECT_EQ(std::filesystem::file_size(path), bytes);
}

TEST_F(RuntimeLogFixture, BoundsRotationAndPreservesContextAndPreviousSession) {
    ASSERT_TRUE(game::configure_runtime_log(path.string().c_str(), "data"));
    game::runtime_log_map("rotation.map", 2, 1, "America", 42);
    const std::string detail(1800, 'x');
    for (int i = 0; i < 6000; ++i)
        game::runtime_log_event("rotation", "%d %s", i, detail.c_str());
    for (int i = 0; i < 4; ++i) {
        const auto segment = i ? std::filesystem::path(path.string() + "." + std::to_string(i)) : path;
        ASSERT_TRUE(std::filesystem::exists(segment));
        EXPECT_LE(std::filesystem::file_size(segment), 2u * 1024u * 1024u);
        const auto contents = read(segment);
        EXPECT_EQ(contents.find("runtime_context_v1"), 0u);
        EXPECT_NE(contents.find("map=\"rotation.map\" mode=2 difficulty=1 player=\"America\" world=42"),
                  std::string::npos);
    }
    EXPECT_FALSE(std::filesystem::exists(path.string() + ".4"));
    ASSERT_TRUE(game::configure_runtime_log(path.string().c_str(), "next-data"));
    EXPECT_NE(read(path.string() + ".1").find("event=session_end reason=reconfigure"), std::string::npos);
    EXPECT_NE(read(path).find("resources=\"next-data\" map=\"\" mode=-1 difficulty=-1"), std::string::npos);
}

TEST_F(RuntimeLogFixture, RejectsInvalidPathsAndRotationDirectoriesWithoutMovingLogs) {
    ASSERT_TRUE(game::configure_runtime_log(path.string().c_str(), nullptr));
    EXPECT_FALSE(game::configure_runtime_log("", nullptr));
    EXPECT_FALSE(game::runtime_log_enabled());
    const auto original = read(path);
    std::filesystem::create_directory(path.string() + ".2");
    EXPECT_FALSE(game::configure_runtime_log(path.string().c_str(), nullptr));
    EXPECT_FALSE(game::runtime_log_enabled());
    EXPECT_EQ(read(path), original);
    EXPECT_FALSE(std::filesystem::exists(path.string() + ".1"));
    EXPECT_TRUE(std::filesystem::is_directory(path.string() + ".2"));
    EXPECT_FALSE(game::configure_runtime_log(root.string().c_str(), nullptr));
    EXPECT_FALSE(game::configure_runtime_log((path / "invalid.log").string().c_str(), nullptr));
}

TEST_F(RuntimeLogFixture, PersistsCopiedRenderFailureAndStagesBeforeTeardown) {
    ASSERT_TRUE(game::configure_runtime_log(path.string().c_str(), "game-data"));
    game::runtime_log_map("ALL07SMD.MAP", 0, 1, "America", 42);
    const auto session = std::filesystem::path(game::runtime_log_session_directory());
    game::RenderFailureContext context;
    game::capture_render_failure_context(context);
    context.world = 42;
    context.generation = 7;
    context.tick = context.frame = 16689;
    context.width = 1920;
    context.height = 1080;
    context.camera_x = -3000;
    context.camera_y = 2611;
    context.core = 2;
    context.end_frame = 1;
    context.core_status = game::DrawingStatus::invalid_argument;
    std::snprintf(context.stage, sizeof(context.stage), "core_draw");
    game::begin_drawing_diagnostics(16689);
    game::record_drawing_diagnostic(game::DrawingStatus::invalid_argument, "indexed", "depth_out_of_range",
                                    "absolute_depth=-1 expected=[0,65535]", "drawing.cpp", 23, 19);
    game::DrawingFailureInfo failure;
    ASSERT_TRUE(game::get_drawing_failure(failure));
    char snapshot[4096];
    ASSERT_TRUE(game::write_render_failure(context, failure, snapshot, sizeof(snapshot)));
    EXPECT_EQ(std::filesystem::path(snapshot).parent_path(), session);
    const auto contents = read(snapshot);
    EXPECT_NE(contents.find("map=\"ALL07SMD.MAP\""), std::string::npos);
    EXPECT_NE(contents.find("frame=16689 world=42 tick=16689 generation=7"), std::string::npos);
    EXPECT_NE(contents.find("canvas=1920,1080 camera=-3000,2611"), std::string::npos);
    EXPECT_NE(
        contents.find(
            "core=2 core_status=invalid_argument end_frame=1 enqueue=0 gpu_prepare=0 gpu_submit=0 gpu_complete=0"),
        std::string::npos);
    EXPECT_NE(contents.find("source=drawing.cpp:23"), std::string::npos);
    EXPECT_NE(contents.find("absolute_depth=-1 expected=[0,65535]"), std::string::npos);
    const auto events = read(session / "events.log");
    EXPECT_NE(events.find("event=render_failure "), std::string::npos);
    EXPECT_NE(events.find("event=render_failure_values"), std::string::npos);
    EXPECT_NE(events.find("snapshot_saved=1"), std::string::npos);
    game::begin_drawing_diagnostics();
    ASSERT_TRUE(game::configure_runtime_log(nullptr, nullptr));
    EXPECT_EQ(read(snapshot), contents);
}

TEST_F(RuntimeLogFixture, SnapshotWriteFailureDoesNotErasePrimaryAndStillWritesEvents) {
    ASSERT_TRUE(game::configure_runtime_log(path.string().c_str(), "data"));
    game::begin_drawing_diagnostics();
    game::record_drawing_diagnostic(game::DrawingStatus::invalid_argument, "raster", "width_out_of_range", "width=-2",
                                    __FILE__, __LINE__);
    game::DrawingFailureInfo failure;
    ASSERT_TRUE(game::get_drawing_failure(failure));
    const auto session = std::filesystem::path(game::runtime_log_session_directory());
    std::filesystem::create_directory(session / ("render-failure-" + std::to_string(failure.id) + ".txt"));
    game::RenderFailureContext context;
    char output[4096] = "not written";
    EXPECT_FALSE(game::write_render_failure(context, failure, output, sizeof(output)));
    EXPECT_EQ(output[0], 0);
    game::DrawingFailureInfo after;
    ASSERT_TRUE(game::get_drawing_failure(after));
    EXPECT_EQ(after.id, failure.id);
    EXPECT_STREQ(after.detail, "width=-2");
    EXPECT_NE(read(session / "events.log").find("snapshot_saved=0"), std::string::npos);
    ASSERT_TRUE(game::configure_runtime_log(nullptr, nullptr));
    EXPECT_FALSE(game::write_render_failure(context, failure, output, sizeof(output)));
    EXPECT_TRUE(game::get_drawing_failure(after));
    game::begin_drawing_diagnostics();
}
