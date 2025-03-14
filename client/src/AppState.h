#pragma once
#include <SDL3/SDL.h>
#include "GameState.h"
#include "Country.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "Map.h"

struct AppState {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *map_texture = nullptr;

    ImVec4 color = { 0, 0, 0, 255 };
    Map map;

    SDL_FRect dst_map_to_display;

    // New fields for game state and player country.
    GameState game_state = GameState::SelectingStartingPoint;
    Country player_country;

    AppState(const Map &map)
        : window(nullptr), renderer(nullptr), map_texture(nullptr),
          color({ 0, 0, 0, 255 }), map(map),
          dst_map_to_display({ 0, 0, (float)map.get_width(), (float)map.get_height()}),
          player_country { 1, "Player", true, {0, 0, 0, 0} }
          {};
};;

