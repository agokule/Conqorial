#include "cq_ui.h"
#include "imgui.h"
#include "implot.h"
#include "Profiler.h"

// Country id is the index of the country in the match
// to show the pyramid for
void show_population_pyramid_renderer(AppState &state, CountryId country_id) {
    const Country &country = state.match.get_country(country_id);
    if (&country.get_pyramid() != &state.pyramid_renderer.get_pyramid())
        state.pyramid_renderer.set_pyramid(country);

    state.pyramid_renderer.render(country.get_urbanization_level());
}

void draw_main_ui(AppState &state, unsigned long long frame_time) {
    ImGui::Begin("Hello there");
    if (ImGui::Button("Reset view")) {
        state.dst_map_to_display = { 0, 0, (float)state.match.get_map().get_width(), (float)state.match.get_map().get_height()};
    }
    ImGui::Text("Frame time: %llu", frame_time);
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

    ImGui::Checkbox("Profiler Enabled", &state.profiler_enabled);
    Profiler::instance().enable(state.profiler_enabled);

    static bool population_pyramid = false;
    ImGui::Checkbox("Show Population Pyramid", &population_pyramid);

    if (population_pyramid) {
        show_population_pyramid_renderer(state, state.player_country_id);
    }

    state.frame_rates.AddPoint(SDL_GetTicks(), ImGui::GetIO().Framerate);

    if (ImPlot::BeginPlot("##Frame Rate Details", ImVec2(-1, 100), ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxisLimits(ImAxis_X1, (state.frame_rates.Data.begin() + state.frame_rates.Offset)->x,
                                SDL_GetTicks(), ImGuiCond_Always);
        ImPlot::SetupAxis(ImAxis_X1, "FPS", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_Opposite);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 240, ImGuiCond_Always);
        ImPlot::PlotLine("FPS", &state.frame_rates.Data[0].x, &state.frame_rates.Data[0].y,
                         state.frame_rates.Data.size(), 0, state.frame_rates.Offset, 2 * sizeof(float));
        ImPlot::EndPlot();
    }

    Profiler::instance().start_frame("Render Map Names");

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    render_country_labels(state.renderer, draw_list, state.match.get_map(),
                        state.dst_map_to_display, state.match.get_countries(),
                        state.region_cache, state.region_cache_needs_update);
    state.region_cache_needs_update = false; // Reset after update

    Profiler::instance().end_frame();

    ImGui::End();
}

