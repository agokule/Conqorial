#include "Profiler.h"
#include <algorithm>
#include "implot.h"
#include <cmath>
#include <numeric>

// Singleton implementation
Profiler& Profiler::instance() {
    static Profiler instance;
    return instance;
}

Profiler::Profiler() 
    : enabled(true)
    , max_history_size(100'000)
    , graph_height(250.0f)
    , history_duration(5.0f) // 5 seconds of history
    , frame_count(0)
{}

Profiler::~Profiler() {}

void Profiler::start_frame(const char* name) {
    if (!enabled) return;
    
    ProfileSection section;
    section.name = name;
    section.start_time = std::chrono::high_resolution_clock::now();
    section.depth = active_sections.size();
    
    active_sections.push(section);
}

void Profiler::end_frame() {
    if (!enabled || active_sections.empty()) return;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    ProfileSection section = active_sections.top();
    active_sections.pop();
    
    auto duration = end_time - section.start_time;
    float duration_ms = std::chrono::duration<float, std::milli>(duration).count();
    
    push_timing(section.name, duration_ms, section.depth);
}

void Profiler::push_timing(const std::string& name, float duration_ms, int depth) {
    // Create section if it doesn't exist
    if (section_data.find(name) == section_data.end()) {
        section_data[name] = {
            std::vector<float>(),
            generate_color(section_data.size()),
            depth,
            true
        };
        section_order.push_back(name);
    }
    
    auto& data = section_data[name];
    data.history.push_back(duration_ms);
    
    // Limit history size
    if (data.history.size() > max_history_size) {
        data.history.erase(data.history.begin());
    }
    
    // Update depth if changed
    data.depth = std::min(data.depth, depth);

    current_frame_sections.insert(name);
}

ImVec4 Profiler::generate_color(int index) const {
    // Generate colors with good contrast
    const float hue_step = 0.618033988749895f; // Golden ratio
    float hue = fmodf(0.4f + hue_step * index, 1.0f);
    
    // Convert HSV to RGB
    ImVec4 color;
    ImGui::ColorConvertHSVtoRGB(hue, 0.65f, 0.9f, color.x, color.y, color.z);
    color.w = 1.0f;
    return color;
}

void Profiler::render_ui() {
    if (!enabled || section_data.empty()) return;

    std::set<std::string> sections {section_order.begin(), section_order.end()};
    for (const auto& section : current_frame_sections)
        sections.erase(section);
    for (const auto& section : sections)
        push_timing(section, 0.0f, 0);
    
    current_frame_sections.clear();
    
    if (ImGui::Begin("Profiler", nullptr)) {
        ImGui::Text("Performance Profiler");
        ImGui::Separator();
        
        // Controls
        if (ImGui::Button("Reset")) {
            reset();
        }
        ImGui::SameLine();
        
        bool is_enabled = enabled;
        if (ImGui::Checkbox("Enabled", &is_enabled)) {
            enable(is_enabled);
        }
        
        ImGui::SliderFloat("History (seconds)", &history_duration, 1.0f, 30.0f);
        ImGui::SliderFloat("Graph Height", &graph_height, 100.0f, 400.0f);
        
        // Prepare data for plotting
        const int visible_frames = (int)std::min(max_history_size, 
            (size_t)(history_duration * 120.0f)); // Assuming ~120 FPS
            
        // Find sections with data
        std::vector<std::string> active_sections;
        for (const auto& name : section_order) {
            const auto& data = section_data[name];
            if (data.history.size() > 0 && data.visible) {
                active_sections.push_back(name);
            }
        }
        
        // Sort by depth (deeper sections first for stacked area)
        std::sort(active_sections.begin(), active_sections.end(), 
            [this](const std::string& a, const std::string& b) {
                return section_data[a].depth > section_data[b].depth;
            });
        
        // Checkboxes to toggle sections
        if (ImGui::CollapsingHeader("Sections", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& name : section_order) {
                auto& data = section_data[name];
                ImGui::ColorEdit4(name.c_str(), &data.color.x, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                ImGui::SameLine();
                ImGui::Checkbox(("##" + name).c_str(), &data.visible);
                ImGui::SameLine();
                
                // Show indentation for nested sections
                std::string indent;
                for (int i = 0; i < data.depth; i++) {
                    indent += "  ";
                }
                
                // Calculate average
                float avg = 0.0f;
                if (!data.history.empty()) {
                    avg = std::accumulate(data.history.begin(), data.history.end(), 0.0f) 
                        / data.history.size();
                }
                
                ImGui::Text("%s%s (%.2f ms)", indent.c_str(), name.c_str(), avg);
            }
        }
        
        // Plot the data
        if (ImPlot::BeginPlot("Frame Times", ImVec2(-1, graph_height))) {
            ImPlot::SetupAxes("Frame", "Time (ms)", 
                ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            
            // X-axis with frame numbers (or time)
            std::vector<float> x_data(visible_frames);
            for (int i = 0; i < visible_frames; i++) {
                x_data[i] = i;
            }
            
            // Collect data for all sections
            std::vector<const float*> y_data_ptrs;
            std::vector<std::vector<float>> y_data;
            std::vector<const char*> labels;
            std::vector<ImVec4> colors;
            
            for (const auto& name : active_sections) {
                const auto& data = section_data[name];
                
                // If we have enough history data
                if (data.history.size() > 0) {
                    // Create a vector of the right size for plotting
                    std::vector<float> plot_data(visible_frames, 0.0f);
                    
                    // Copy available history
                    size_t to_copy = std::min(visible_frames, (int)data.history.size());
                    std::copy(
                        data.history.end() - to_copy,
                        data.history.end(),
                        plot_data.end() - to_copy
                    );
                    
                    y_data.push_back(std::move(plot_data));
                    y_data_ptrs.push_back(y_data.back().data());
                    labels.push_back(name.c_str());
                    colors.push_back(data.color);
                }
            }
            
            // Plot stacked area
            if (!y_data.empty()) {
                // Instead of PlotStackedAreaSeries, use multiple PlotShaded calls for stacking
                float* total_y = new float[visible_frames]();
                
                for (size_t i = 0; i < y_data_ptrs.size(); i++) {
                    ImPlot::SetNextFillStyle(colors[i], 0.5f);
                    ImPlot::SetNextLineStyle(colors[i]);
                    
                    // Create a cumulative sum array
                    std::vector<float> stack_data(visible_frames);
                    const float* current_data = y_data_ptrs[i];
                    
                    for (int j = 0; j < visible_frames; j++) {
                        total_y[j] += current_data[j];
                        stack_data[j] = total_y[j];
                    }
                    
                    // Plot the shaded area between previous total and new total
                    if (i == 0) {
                        // For the first dataset, shade from 0 to its value
                        ImPlot::PlotShaded(labels[i], x_data.data(), stack_data.data(), visible_frames, 0.0);
                    } else {
                        // For subsequent datasets, shade between previous total and new total
                        std::vector<float> prev_total(visible_frames);
                        for (int j = 0; j < visible_frames; j++) {
                            prev_total[j] = total_y[j] - current_data[j];
                        }
                        ImPlot::PlotShaded(labels[i], x_data.data(), stack_data.data(), 
                                           prev_total.data(), visible_frames);
                    }
                    
                    // Draw the line at the top of this section
                    ImPlot::PlotLine(labels[i], x_data.data(), stack_data.data(), visible_frames);
                }
                
                delete[] total_y;
            }
            
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

void Profiler::reset() {
    for (auto& [name, data] : section_data) {
        data.history.clear();
    }
    frame_count = 0;
}

void Profiler::enable(bool enabled_state) {
    enabled = enabled_state;
}

bool Profiler::is_enabled() const {
    return enabled;
}
