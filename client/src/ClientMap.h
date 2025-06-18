#pragma once

#include "Match.h"
#include "AppState.h"
#include "SDL3/SDL.h"
#include "Match.h"

SDL_Color get_tile_color(MapTileType type);

SDL_Color get_tile_display_color(const MapTile &tile, const Match &match);

SDL_Texture *init_map_texture(SDL_Renderer *renderer, const Match &match);

void sync_map_texture(SDL_Texture *texture, const Match &match, const std::vector<std::pair<TileCoor, TileCoor>> &tiles_to_update);

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer, SDL_FRect src_rect);

void zoom_map(float zoom_factor, float center_x, float center_y, AppState &state);

std::optional<std::pair<TileCoor, TileCoor>> convert_screen_to_map_coors(float x, float y, const AppState &state);
std::pair<float, float> convert_map_to_screen_coors(TileCoor x, TileCoor y, const AppState &state);

