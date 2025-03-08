#pragma once
#include <SDL3/SDL.h>
#include "imgui.h"

struct AppState {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    ImVec4 color = { 0, 0, 0, 255 };
};

