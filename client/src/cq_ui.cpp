#include "cq_ui.h"
#include "ClientMap.h"
#include "GameState.h"
#include "Logging.h"
#include "MapTile.h"
#include "MapTileTypes.h"
#include "imgui.h"
#include "implot.h"
#include "Profiler.h"
#include <algorithm>

// Country id is the index of the country in the match
// to show the pyramid for
void show_population_pyramid_renderer(AppState &state, CountryId country_id) {
    const Country &country = state.match.get_country(country_id);
    if (&country.get_pyramid() != &state.pyramid_renderer.get_pyramid())
        state.pyramid_renderer.set_pyramid(country);

    state.pyramid_renderer.render(country.get_urbanization_level(), false);
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

    int player_target_mobilization = state.player_target_mobilization;
    ImGui::SliderInt("Mobilization Percent", &player_target_mobilization, 1, 100);
    state.player_target_mobilization = player_target_mobilization;
    state.match.set_country_target_mobilization_level(state.player_country_id, state.player_target_mobilization);


    auto troops_max = state.match.get_country(state.player_country_id).get_troops() / 2;
    int troops_selected {static_cast<int>(std::clamp(state.troops_selected, 0u, troops_max))};

    float x, y;
    SDL_GetMouseState(&x, &y);
    auto coors = convert_screen_to_map_coors(x, y, state);
    if (coors.has_value()) {
        auto [mx, my] = coors.value();
        ImGui::Text("Mouse: %d, %d", mx, my);
    }

    ImGui::SliderInt("Troops", &troops_selected, 0, troops_max);
    state.troops_selected = troops_selected;

    if (ImGui::Button("Upgrade Millitary Level")) {
        state.match.upgrade_country_millitary(state.player_country_id);
    }

    if (state.match.get_game_state() == GameState::SelectingStartingPoint && ImGui::Button("Start Game"))
        state.match.set_game_started();

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

void click_on_map(AppState &state, TileCoor x, TileCoor y) {
    state.selected_tile = state.match.get_map().get_tile_index(x, y);
}

void right_click_on_map(AppState &state, TileCoor x, TileCoor y) {
    MapTile tile = state.match.get_map_tile(x, y);
    click_on_map(state, x, y);
    state.country_being_selected = tile.owner;
}

void display_country_info(AppState &state, CountryId country_id) {
    if (country_id == 0)
        return;

    const Country &country = state.match.get_country(country_id);
    ImGui::Begin("Country Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Name: %s", country.get_name().c_str());
    ImGui::Text("Millitary Level: %d", country.get_millitary_level());
    ImGui::Text("Money: %d", country.get_money());
    ImGui::Text("Number of Troops: %d", country.get_troops());
    ImGui::Text("Population: %d", country.get_pyramid().get_total_population());
    ImGui::Text("Economy: %d", country.get_economy());

    if (country_id != state.player_country_id) {
        if (ImGui::Button("Attack with selected troops?")) {
            state.match.attack(state.player_country_id, country_id, state.troops_selected);
            state.region_cache_needs_update = true;
        }
    }

    static bool population_pyramid = false;
    ImGui::Checkbox("Show Population Pyramid", &population_pyramid);

    if (population_pyramid) {
        ImGui::Separator();
        show_population_pyramid_renderer(state, country_id);
        ImGui::Separator();
    }

    ImGui::End();
}

void display_tile_dialogs(AppState &state) {
    CONQORIAL_ASSERT_ALL(state.selected_tile.has_value(), "No tile selected", return;);
    TileIndex tile_index = *state.selected_tile;
    auto [mx, my] = state.match.get_map().get_tile_coors(tile_index);
    MapTile tile = state.match.get_map_tile(mx, my);
    auto [sx, sy] = convert_map_to_screen_coors(mx, my, state);

    if (tile.type == MapTileType::Water)
        return;

    ImGui::SetNextWindowPos({sx, sy});
    ImGui::Begin("Tile Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    if (state.match.get_game_state() == GameState::SelectingStartingPoint) {
        if (ImGui::Button("Set Starting Point")) {
            state.match.spawn_country(state.player_country_id, mx, my);
            state.match.set_game_started();
            state.selected_tile = std::nullopt;
        }
        ImGui::End();
        return;
    }

    if (ImGui::Button("Attack")) {
        state.match.attack(state.player_country_id, tile.owner, state.troops_selected);
        state.region_cache_needs_update = true;
        state.selected_tile = std::nullopt;
    }

    if (tile.elevation >= MapTileType::Beach && tile.elevation < MapTileType::Grass &&
            ImGui::Button("Naval Invade")) {
        // TODO: Implement this
        CQ_LOG_DEBUG << "Naval invade\n";
        state.selected_tile = std::nullopt;
    }

    ImGui::End();
}

