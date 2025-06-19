#pragma once

#include "AppState.h"

void show_population_pyramid_renderer(AppState &state, CountryId country_id);
void draw_main_ui(AppState &state, unsigned long long frame_time);

void click_on_map(AppState &state, TileCoor x, TileCoor y);
void right_click_on_map(AppState &state, TileCoor x, TileCoor y);

void display_country_info(AppState &state, CountryId country_id);
void display_tile_dialogs(AppState &state);

