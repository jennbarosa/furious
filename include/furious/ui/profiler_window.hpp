#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>

namespace furious {

struct AllocationStats {
    static std::atomic<uint64_t> total_allocations;
    static std::atomic<uint64_t> total_deallocations;
    static std::atomic<uint64_t> total_bytes_allocated;
    static std::atomic<uint64_t> frame_allocations;
    static std::atomic<uint64_t> frame_bytes;

    static void reset_frame() {
        frame_allocations = 0;
        frame_bytes = 0;
    }
};

class ProfilerWindow {
public:
    ProfilerWindow();

    void render();
    void update();

    void set_visible(bool visible) { visible_ = visible; }
    [[nodiscard]] bool visible() const { return visible_; }
    void toggle_visible() { visible_ = !visible_; }

    void set_video_decoder_info(const std::string& info) { video_decoder_info_ = info; }

private:
    bool visible_ = false;

    static constexpr size_t HISTORY_SIZE = 120;

    std::array<float, HISTORY_SIZE> cpu_history_{};
    std::array<float, HISTORY_SIZE> memory_history_{};
    std::array<float, HISTORY_SIZE> frame_time_history_{};
    std::array<float, HISTORY_SIZE> alloc_history_{};
    size_t history_index_ = 0;

    std::chrono::steady_clock::time_point last_update_;
    std::chrono::steady_clock::time_point last_frame_;
    double update_interval_ = 0.1;

    float current_cpu_ = 0.0f;
    float current_memory_mb_ = 0.0f;
    float current_frame_time_ms_ = 0.0f;
    float peak_memory_mb_ = 0.0f;
    float avg_frame_time_ms_ = 0.0f;
    uint64_t last_frame_allocs_ = 0;

    unsigned long last_cpu_total_ = 0;
    unsigned long last_cpu_idle_ = 0;

    std::string video_decoder_info_ = "None";

    void sample_metrics();
    float get_process_memory_mb();
    float get_cpu_usage();
};

} // namespace furious
