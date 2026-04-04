#include "furious/ui/profiler_window.hpp"
#include "imgui.h"
#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <numeric>
#include <string>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#elif defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

namespace furious {

std::atomic<uint64_t> AllocationStats::total_allocations{0};
std::atomic<uint64_t> AllocationStats::total_deallocations{0};
std::atomic<uint64_t> AllocationStats::total_bytes_allocated{0};
std::atomic<uint64_t> AllocationStats::frame_allocations{0};
std::atomic<uint64_t> AllocationStats::frame_bytes{0};

ProfilerWindow::ProfilerWindow() {
    last_update_ = std::chrono::steady_clock::now();
    last_frame_ = std::chrono::steady_clock::now();
    cpu_history_.fill(0.0f);
    memory_history_.fill(0.0f);
    frame_time_history_.fill(0.0f);
    alloc_history_.fill(0.0f);
}

void ProfilerWindow::update() {
    auto now = std::chrono::steady_clock::now();

    auto frame_duration = std::chrono::duration<float, std::milli>(now - last_frame_);
    current_frame_time_ms_ = frame_duration.count();
    last_frame_ = now;

    last_frame_allocs_ = AllocationStats::frame_allocations.load();
    AllocationStats::reset_frame();

    auto elapsed = std::chrono::duration<double>(now - last_update_).count();
    if (elapsed >= update_interval_) {
        sample_metrics();
        last_update_ = now;
    }
}

void ProfilerWindow::sample_metrics() {
    current_memory_mb_ = get_process_memory_mb();
    current_cpu_ = get_cpu_usage();

    if (current_memory_mb_ > peak_memory_mb_) {
        peak_memory_mb_ = current_memory_mb_;
    }

    cpu_history_[history_index_] = current_cpu_;
    memory_history_[history_index_] = current_memory_mb_;
    frame_time_history_[history_index_] = current_frame_time_ms_;
    alloc_history_[history_index_] = static_cast<float>(last_frame_allocs_);
    history_index_ = (history_index_ + 1) % HISTORY_SIZE;

    float sum = 0.0f;
    for (float ft : frame_time_history_) {
        sum += ft;
    }
    avg_frame_time_ms_ = sum / static_cast<float>(HISTORY_SIZE);
}

float ProfilerWindow::get_process_memory_mb() {
#ifdef __linux__
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        long pages = 0;
        statm >> pages;
        long resident = 0;
        statm >> resident;
        long page_size = sysconf(_SC_PAGESIZE);
        return static_cast<float>(resident * page_size) / (1024.0f * 1024.0f);
    }
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
    }
#endif
    return 0.0f;
}

float ProfilerWindow::get_cpu_usage() {
#ifdef __linux__
    std::ifstream stat("/proc/stat");
    if (stat.is_open()) {
        std::string cpu;
        uint64_t user, nice, system, idle, iowait, irq, softirq;
        stat >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

        uint64_t total = user + nice + system + idle + iowait + irq + softirq;
        uint64_t idle_total = idle + iowait;

        if (last_cpu_total_ > 0) {
            uint64_t total_diff = total - last_cpu_total_;
            uint64_t idle_diff = idle_total - last_cpu_idle_;

            if (total_diff > 0) {
                float usage = 100.0f * (1.0f - static_cast<float>(idle_diff) / static_cast<float>(total_diff));
                last_cpu_total_ = total;
                last_cpu_idle_ = idle_total;
                return std::clamp(usage, 0.0f, 100.0f);
            }
        }

        last_cpu_total_ = total;
        last_cpu_idle_ = idle_total;
    }
#elif defined(_WIN32)
    FILETIME idle_time, kernel_time, user_time;
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        auto to_uint64 = [](const FILETIME& ft) -> uint64_t {
            return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        };
        uint64_t idle_val = to_uint64(idle_time);
        uint64_t kernel_val = to_uint64(kernel_time);
        uint64_t user_val = to_uint64(user_time);
        uint64_t total = kernel_val + user_val;

        if (last_cpu_total_ > 0) {
            uint64_t total_diff = total - last_cpu_total_;
            uint64_t idle_diff = idle_val - last_cpu_idle_;
            if (total_diff > 0) {
                float usage = 100.0f * (1.0f - static_cast<float>(idle_diff) / static_cast<float>(total_diff));
                last_cpu_total_ = total;
                last_cpu_idle_ = idle_val;
                return std::clamp(usage, 0.0f, 100.0f);
            }
        }

        last_cpu_total_ = total;
        last_cpu_idle_ = idle_val;
    }
#endif
    return 0.0f;
}

void ProfilerWindow::render() {
    if (!visible_) return;

    ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
    ImGui::Begin("Profiler", &visible_, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Video Decoder: %s", video_decoder_info_.c_str());
    ImGui::Separator();

    ImGui::Text("Frame Time: %.2f ms (%.1f FPS)",
                current_frame_time_ms_,
                current_frame_time_ms_ > 0 ? 1000.0f / current_frame_time_ms_ : 0.0f);
    ImGui::Text("Avg Frame Time: %.2f ms (%.1f FPS)",
                avg_frame_time_ms_,
                avg_frame_time_ms_ > 0 ? 1000.0f / avg_frame_time_ms_ : 0.0f);

    {
        std::array<float, HISTORY_SIZE> ordered;
        for (size_t i = 0; i < HISTORY_SIZE; ++i) {
            ordered[i] = frame_time_history_[(history_index_ + i) % HISTORY_SIZE];
        }
        float max_ft = *std::max_element(ordered.begin(), ordered.end());
        max_ft = std::max(max_ft, 16.67f);

        ImGui::PlotLines("##frame_time", ordered.data(), static_cast<int>(HISTORY_SIZE),
                         0, "Frame Time (ms)", 0.0f, max_ft * 1.2f,
                         ImVec2(ImGui::GetContentRegionAvail().x, 30));
    }

    ImGui::Separator();

    ImGui::Text("Allocations: %" PRIu64 "/frame (%" PRIu64 " bytes)",
                last_frame_allocs_,
                AllocationStats::frame_bytes.load());
    ImGui::Text("Total: %" PRIu64 " allocs, %" PRIu64 " deallocs",
                AllocationStats::total_allocations.load(),
                AllocationStats::total_deallocations.load());

    {
        std::array<float, HISTORY_SIZE> ordered;
        for (size_t i = 0; i < HISTORY_SIZE; ++i) {
            ordered[i] = alloc_history_[(history_index_ + i) % HISTORY_SIZE];
        }
        float max_alloc = *std::max_element(ordered.begin(), ordered.end());
        max_alloc = std::max(max_alloc, 10.0f);

        ImGui::PlotLines("##allocs", ordered.data(), static_cast<int>(HISTORY_SIZE),
                         0, "Allocs/frame", 0.0f, max_alloc * 1.2f,
                         ImVec2(ImGui::GetContentRegionAvail().x, 30));
    }

    ImGui::Separator();

    ImGui::Text("CPU Usage: %.1f%%", current_cpu_);
    {
        std::array<float, HISTORY_SIZE> ordered;
        for (size_t i = 0; i < HISTORY_SIZE; ++i) {
            ordered[i] = cpu_history_[(history_index_ + i) % HISTORY_SIZE];
        }
        ImGui::PlotLines("##cpu", ordered.data(), static_cast<int>(HISTORY_SIZE),
                         0, "CPU %", 0.0f, 100.0f,
                         ImVec2(ImGui::GetContentRegionAvail().x, 30));
    }

    ImGui::Separator();

    ImGui::Text("Memory: %.1f MB (Peak: %.1f MB)", current_memory_mb_, peak_memory_mb_);
    {
        std::array<float, HISTORY_SIZE> ordered;
        for (size_t i = 0; i < HISTORY_SIZE; ++i) {
            ordered[i] = memory_history_[(history_index_ + i) % HISTORY_SIZE];
        }
        float max_mem = peak_memory_mb_ * 1.2f;
        max_mem = std::max(max_mem, 100.0f);

        ImGui::PlotLines("##memory", ordered.data(), static_cast<int>(HISTORY_SIZE),
                         0, "Memory (MB)", 0.0f, max_mem,
                         ImVec2(ImGui::GetContentRegionAvail().x, 30));
    }

    ImGui::End();
}

} // namespace furious
