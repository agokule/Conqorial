#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include <map>
#include <vector>
#include "Attack.h"
#include "NameRendering.h"
#include "GameState.h"
#include "Country.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "Map.h"
#include "typedefs.h"
#include "utils.h"

struct AppState {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *map_texture = nullptr;

    ImVec4 color = { 0, 0, 0, 255 };
    uint64_t last_frame_time;
    ScrollingBuffer frame_rates;

    RegionCache region_cache;
    bool region_cache_needs_update = true;

    Map map;

    SDL_FRect dst_map_to_display;

    // New fields for game state and player country.
    GameState game_state = GameState::SelectingStartingPoint;
    Country player_country;

    std::vector<std::function<bool()>> callback_functions;
    std::map<CountryId, Country> countries;

    // on_going_attacks[attacker-][defender-id]
    std::map<CountryId, std::map<CountryId, Attack>> on_going_attacks;

    AppState(const Map &map)
        : window(nullptr), renderer(nullptr), map_texture(nullptr),
          color({ 0, 0, 0, 255 }), last_frame_time {SDL_GetTicks()},
          region_cache {}, map {map},
          dst_map_to_display({ 0, 0, (float)map.get_width(), (float)map.get_height()}),
          player_country { 1, "Player", true, {0, 0, 0, 0} },
          countries {},
          on_going_attacks {}
          {
        countries[0] = { 0, "Neutral", false, {0, 0, 0, 0} };
        countries[1] = player_country;
    };
};

