#pragma once

#include "Map.h"
#include "AppState.h"
#include <map>
#include "SDL3/SDL.h"

SDL_Color get_tile_color(MapTileType type);

SDL_Color get_tile_display_color(const MapTile &tile, const std::map<CountryId, Country> &countries);

SDL_Texture *init_map_texture(const Map &map, SDL_Renderer *renderer, const std::map<CountryId, Country> &countries);

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer, SDL_FRect src_rect);

void zoom_map(float zoom_factor, float center_x, float center_y, AppState &state);

