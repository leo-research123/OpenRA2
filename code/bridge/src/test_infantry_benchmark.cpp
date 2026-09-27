// Benchmark/reporting methods of RA2TestInfantry.
#include "bridge/test_infantry.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <numeric>
#include <sstream>


void RA2TestInfantry::observe_benchmark(double frame_ms) {
    // Fixed protocol requested for both sprite and ball benchmarks: 5s + 5s.
    if (!benchmark_recording_) {
        if (std::chrono::duration<double>(last_frame_ - benchmark_origin_).count() < 5.0) return;
        benchmark_recording_ = true;
        benchmark_viewport_ = viewport_size_;
        godot::UtilityFunctions::print("test1 benchmark: warmup complete; recording 5 seconds at cap 60.");
        return; // First recorded interval starts after warmup, with no partial interval.
    }
    benchmark_resized_ |= viewport_size_ != benchmark_viewport_;
    benchmark_samples_.push_back({frame_ms, update_ms_, submit_ms_, submitted_, get_window()->has_focus(), paused_});
    benchmark_measured_seconds_ += frame_ms / 1000;
    if (benchmark_measured_seconds_ < 5.0) return;
    set_process(false);
    save_benchmark(); // No disk I/O or percentile calculation inside the timed window.
}

void RA2TestInfantry::save_benchmark() {
    const auto distribution = [](std::vector<double> values) {
        godot::Dictionary result;
        std::sort(values.begin(), values.end());
        const auto percentile = [&](double p) {
            return values[size_t(std::ceil(p * values.size())) - 1];
        };
        result["mean"] = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        result["p50"] = percentile(0.5); result["p95"] = percentile(0.95);
        result["p99"] = percentile(0.99); result["max"] = values.back();
        return result;
    };
    std::vector<double> intervals, updates, submits;
    int over_budget = 0, over_20 = 0, over_33 = 0, unfocused = 0, paused = 0;
    int min_submitted = instance_count_, max_submitted = 0;
    std::ostringstream csv;
    csv.imbue(std::locale::classic()); csv << std::fixed << std::setprecision(6);
    csv << "frame,frame_interval_ms,update_ms,submit_ms,submitted,focused,paused\n";
    for (size_t i = 0; i < benchmark_samples_.size(); ++i) {
        const auto& sample = benchmark_samples_[i];
        intervals.push_back(sample.frame_ms); updates.push_back(sample.update_ms); submits.push_back(sample.submit_ms);
        over_budget += sample.frame_ms > 1000.0 / 60;
        over_20 += sample.frame_ms > 20; over_33 += sample.frame_ms > 1000.0 / 30;
        unfocused += !sample.focused; paused += sample.paused;
        min_submitted = std::min(min_submitted, sample.submitted);
        max_submitted = std::max(max_submitted, sample.submitted);
        csv << i << ',' << sample.frame_ms << ',' << sample.update_ms << ',' << sample.submit_ms
            << ',' << sample.submitted << ',' << sample.focused << ',' << sample.paused << '\n';
    }
    auto* os = godot::OS::get_singleton();
    auto* engine = godot::Engine::get_singleton();
    auto* server = godot::RenderingServer::get_singleton();
    godot::Dictionary report;
    report["native_build"] = "Release"; report["godot_release_export"] = os->has_feature("release");
    report["godot"] = engine->get_version_info()["string"];
    report["os"] = os->get_name(); report["executable"] = os->get_executable_path();
    report["gpu"] = server->get_video_adapter_name();
    report["renderer"] = server->get_current_rendering_method();
    report["driver"] = server->get_current_rendering_driver_name();
    report["fps_limit"] = engine->get_max_fps();
    report["vsync_mode"] = int(godot::DisplayServer::get_singleton()->window_get_vsync_mode(get_window()->get_window_id()));
    report["viewport_width"] = int(benchmark_viewport_.x); report["viewport_height"] = int(benchmark_viewport_.y);
    report["viewport_changed"] = benchmark_resized_; report["instance_count"] = instance_count_;
    report["scale"] = 1; report["warmup_seconds"] = 5; report["target_measure_seconds"] = 5;
    report["measured_seconds"] = benchmark_measured_seconds_;
    report["frames"] = int64_t(benchmark_samples_.size());
    report["average_fps"] = benchmark_samples_.size() / benchmark_measured_seconds_;
    report["frame_interval_ms"] = distribution(intervals);
    report["update_ms"] = distribution(updates); report["submit_ms"] = distribution(submits);
    report["frames_over_16_667ms"] = over_budget; report["frames_over_20ms"] = over_20;
    report["frames_over_33_333ms"] = over_33;
    report["submitted_min"] = min_submitted; report["submitted_max"] = max_submitted;
    report["unfocused_frames"] = unfocused; report["paused_frames"] = paused;
    report["gpu_ms"] = godot::Variant();
    report["timing_boundary"] = "_process start-to-start; includes frame cap waiting, not GPU presentation timing";
    auto csv_file = godot::FileAccess::open(benchmark_report_.get_basename() + ".csv", godot::FileAccess::WRITE);
    auto json_file = godot::FileAccess::open(benchmark_report_, godot::FileAccess::WRITE);
    if (csv_file.is_null() || json_file.is_null()) {
        ERR_PRINT("Cannot write test1 benchmark report"); get_tree()->quit(1); return;
    }
    csv_file->store_string(godot::String::utf8(csv.str().c_str()));
    json_file->store_string(godot::JSON::stringify(report, "  ", true, true));
    csv_file->flush(); json_file->flush();
    const bool written = csv_file->get_error() == godot::OK && json_file->get_error() == godot::OK;
    godot::UtilityFunctions::print("test1 benchmark finished: ", benchmark_report_);
    get_tree()->quit(written ? 0 : 1);
}
