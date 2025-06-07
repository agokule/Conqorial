#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include <vector>
#include "NameRendering.h"
#include "Country.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "Map.h"
#include "typedefs.h"
#include "utils.h"
#include "Match.h"
#include "PopulationPyramidRenderer.h"

struct AppState {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *map_texture = nullptr;

    ImVec4 color = { 0, 0, 0, 255 };
    uint64_t last_frame_time;
    ScrollingBuffer frame_rates;

    RegionCache region_cache;
    bool region_cache_needs_update = true;

    SDL_FRect dst_map_to_display;

    CountryId player_country_id;

    std::vector<std::function<bool()>> callback_functions;
    Match match;

    PopulationPyramidRenderer pyramid_renderer;
    bool profiler_enabled;

    AppState(const Map &map)
        : window(nullptr), renderer(nullptr), map_texture(nullptr),
          color({ 0, 0, 0, 255 }), last_frame_time {SDL_GetTicks()},
          region_cache {},
          dst_map_to_display({ 0, 0, (float)map.get_width(), (float)map.get_height()}),
          match {map.get_width(), map.get_height()}, pyramid_renderer {}, profiler_enabled(true)
          {
        player_country_id = match.new_country("Player", true, {0,0,0,0}).get_id();
    };
};

