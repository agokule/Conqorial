#include "PopulationPyramidRenderer.h"
#include <vector>
#include "PopulationPyramid.h"
#include "implot.h"

PopulationPyramidRenderer::PopulationPyramidRenderer(): pyramid {nullptr} {
    // Initialize history with current state
    record_current_state();
}

void PopulationPyramidRenderer::record_current_state() {
    // Get current population data
    std::vector<unsigned> males(20), females(20);
    unsigned total_males = 0, total_females = 0;
    
    // Access pyramid data (you'll need to add a getter method to PopulationPyramid)
    // For now, we'll simulate this - you should add a getter method
    const auto& pieces = pyramid->getPieces();
    for (int i = 0; i < 20; ++i) {
        males[i] = pieces[i].male_count;
        females[i] = pieces[i].female_count;
        total_males += males[i];
        total_females += females[i];
    }
    
    // Store in history
    if (male_history.size() >= MAX_HISTORY) {
        male_history.erase(male_history.begin());
        female_history.erase(female_history.begin());
        total_population_history.erase(total_population_history.begin());
    }
    
    male_history.push_back(males);
    female_history.push_back(females);
    total_population_history.push_back(total_males + total_females);

}

void PopulationPyramidRenderer::render_pyramid_chart() {
    // Custom colormap for population pyramid (blue for males, pink for females)
    static ImPlotColormap PopulationColors = -1;
    if (PopulationColors == -1) {
        static const ImU32 PopulationData[2] = { 
            IM_COL32(51, 153, 255, 180),   // Blue for males
            IM_COL32(255, 102, 153, 180)   // Pink for females
        };
        PopulationColors = ImPlot::AddColormap("Population", PopulationData, 2);
    }
    
    if (ImPlot::BeginPlot("Population Pyramid", ImVec2(-1, 400), ImPlotFlags_NoMouseText)) {
        // Age group labels
        static const char* age_labels[20] = {
            "0-4", "5-9", "10-14", "15-19", "20-24", "25-29", "30-34", "35-39",
            "40-44", "45-49", "50-54", "55-59", "60-64", "65-69", "70-74", 
            "75-79", "80-84", "85-89", "90-94", "95+"
        };
        
        // Prepare data arrays for stacked bars
        // We need: [male_negative, female_positive] for each age group
        static int population_data[40]; // 2 categories * 20 age groups
        
        // Get current data (replace with actual pyramid data when available)
        for (int i = 0; i < 20; ++i) {
            const auto& pieces = pyramid->getPieces();
            population_data[i] = -static_cast<int>(pieces[i].male_count);      // Males (negative)
            population_data[i + 20] = static_cast<int>(pieces[i].female_count); // Females (positive)
        }
        
        static const char* categories[] = {"Males", "Females"};
        
        ImPlot::PushColormap(PopulationColors);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
        ImPlot::SetupAxes("Population", "Age Groups", 
                        ImPlotAxisFlags_AutoFit, 
                        ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_Invert);
        ImPlot::SetupAxisTicks(ImAxis_Y1, 0, 19, 20, age_labels, false);
        
        // Plot stacked horizontal bars
        ImPlot::PlotBarGroups(categories, population_data, 2, 20, 0.8, 0, 
                            ImPlotBarGroupsFlags_Stacked | ImPlotBarGroupsFlags_Horizontal);
        
        ImPlot::PopColormap();
        ImPlot::EndPlot();
    }
}

void PopulationPyramidRenderer::render_population_trend() {
    if (total_population_history.size() < 2) return;
    
    if (ImPlot::BeginPlot("Population Over Time", ImVec2(-1, 250))) {
        std::vector<double> months;
        std::vector<double> population;
        
        for (size_t i = 0; i < total_population_history.size(); ++i) {
            months.push_back(static_cast<double>(i));
            population.push_back(static_cast<double>(total_population_history[i]));
        }
        ImPlot::SetupAxis(ImAxis_X1, "Month", ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxis(ImAxis_Y1, "Population", ImPlotAxisFlags_AutoFit);
        
        ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);
        ImPlot::PlotLine("Total Population", months.data(), population.data(), months.size());
        
        ImPlot::EndPlot();
    }
}

void PopulationPyramidRenderer::render_controls(int urbanization_param) {
    ImGui::Begin("Population Simulation Controls");
    
    ImGui::Text("Month: %d", current_month);

    economy_param = economy.score;
    
    ImGui::SliderInt("Economy", &economy_param, 0, 200, "%d", ImGuiSliderFlags_NoInput);
    ImGui::SliderInt("Density", &density_param, 0, 100'000);
    ImGui::SliderInt("Urbanization", &urbanization_param, 1, 10);

    ImGui::Text("Money made: %d", economy.money_made);
    
    // Display current statistics
    ImGui::Separator();
    ImGui::Text("Current Statistics:");
    ImGui::Text("Total Population: %u", pyramid->get_total_population());
    
    ImGui::End();
}

void PopulationPyramidRenderer::render(int urbanization_param) {
    render_controls(urbanization_param);
    
    ImGui::Begin("Population Pyramid Visualization");
    render_pyramid_chart();
    render_population_trend();
    ImGui::End();
}

void PopulationPyramidRenderer::set_pyramid(const Country &country) {
    this->pyramid = &country.pyramid;
    current_month = 0;
    male_history.clear();
    female_history.clear();
    total_population_history.clear();
    birth_history.clear();
    birth_rate_history.clear();
    life_expectancy_history.clear();
}

