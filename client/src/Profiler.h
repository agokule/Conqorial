#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <stack>
#include "imgui.h"

class Profiler {
public:
    // Singleton pattern
    static Profiler& instance();
    
    // Main API methods
    void start_frame(const char* name);
    void end_frame();
    
    // Visualization
    void render_ui();
    
    // Helper methods
    void reset();
    void enable(bool enabled = true);
    bool is_enabled() const;
    
private:
    Profiler();
    ~Profiler();
    
    // Prevent copying
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    
    // Internal structures
    struct ProfileSection {
        std::string name;
        std::chrono::high_resolution_clock::time_point start_time;
        float duration_ms;
        int depth;
    };
    
    struct SectionData {
        std::vector<float> history;
        ImVec4 color;
        int depth;
        bool visible;
    };
    
    // State
    bool enabled;
    std::stack<ProfileSection> active_sections;
    std::unordered_map<std::string, SectionData> section_data;
    std::vector<std::string> section_order;
    
    // Configuration
    size_t max_history_size;
    float graph_height;
    float history_duration; // in seconds
    int frame_count;
    
    // Internal helpers
    ImVec4 generate_color(int index) const;
    void push_timing(const std::string& name, float duration_ms, int depth);
};

// Helper macros for easier usage
#define PROFILE_SECTION(name) Profiler::instance().start_frame(name); \
                              auto PROFILE_LINE_HELPER_##__LINE__ = [&](const char* n) { \
                                  return ProfilerGuard(n); \
                              }(name);

// RAII guard to automatically end a profile section
class ProfilerGuard {
public:
    ProfilerGuard(const char* name) : name(name) {}
    ~ProfilerGuard() { Profiler::instance().end_frame(); }
private:
    const char* name;
};
