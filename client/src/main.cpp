#include "ClientMap.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <iostream>
#include <sys/stat.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "AppState.h"

#include "Map.h"

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    std::cout << "Hello World!\n";
    AppState *state = new AppState({ 600, 600 });
    *appstate = state;

    std::cout << "Initializing SDL\n";
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, SDL_WINDOW_FULLSCREEN, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    std::cout << "Initializing ImGui\n";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(state->window, state->renderer);
    ImGui_ImplSDLRenderer3_Init(state->renderer);

    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;

    std::cout << "Initializing Map texture\n";
    state->map_texture = init_map_texture(state->map, state->renderer);

    SDL_SetTextureScaleMode(state->map_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    ImGui_ImplSDL3_ProcessEvent(event);
    AppState &state = *static_cast<AppState *>(appstate);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        if (event->motion.state & SDL_BUTTON_LMASK) {
            auto x_rel = event->motion.xrel;
            auto y_rel = event->motion.yrel;
            state.dst_map_to_display.x += x_rel;
            state.dst_map_to_display.y += y_rel;
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        // Choose a zoom factor. For example, a wheel notch scales by 10%
        float zoom_factor = 1.0f + event->wheel.y * 0.1f;
        zoom_map(zoom_factor, event->wheel.mouse_x, event->wheel.mouse_y, state);
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *const state = static_cast<AppState *>(appstate);
    SDL_Renderer *renderer = state->renderer;

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    ImGui::Begin("Hello there");
    if (ImGui::Button("Reset view")) {
        state->dst_map_to_display = { 0, 0, (float)state->map.get_width(), (float)state->map.get_height()};
    }
    ImGui::End();

    draw_map_texture(state->map_texture, renderer, state->dst_map_to_display);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    std::cout << "Exiting right now...\n";
    AppState *state = static_cast<AppState *>(appstate);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyTexture(state->map_texture);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    delete state;
}
