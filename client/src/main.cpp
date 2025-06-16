#include "ClientMap.h"
#include "GameState.h"
#include "SDL3/SDL_init.h"
#include <sys/stat.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Logging.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "implot.h"

#include "AppState.h"
#include "Profiler.h"
#include "Map.h"
#include "cq_ui.h"

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    CQ_LOG_DEBUG << "Hello World!\n";
    AppState *state = new AppState({ 600, 600 });
    *appstate = state;

    CQ_LOG_RELEASE << "Initializing SDL\n";
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, SDL_WINDOW_FULLSCREEN, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    CQ_LOG_RELEASE << "Initializing ImGui\n";
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

    ImPlot::CreateContext();

    CQ_LOG_RELEASE << "Initializing Map texture\n";
    state->map_texture = init_map_texture(state->renderer, state->match);

    SDL_SetTextureScaleMode(state->map_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    PROFILE_SECTION("SDL_AppEvent");

    ImGui_ImplSDL3_ProcessEvent(event);
    if (ImGui::GetIO().WantCaptureMouse) return SDL_APP_CONTINUE;
    if (ImGui::GetIO().WantCaptureKeyboard) return SDL_APP_CONTINUE;

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
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // Convert mouse coordinates (from the event) to map tile coordinates.
        float relX = event->button.x - state.dst_map_to_display.x;
        float relY = event->button.y - state.dst_map_to_display.y;
        // Scale based on how the map texture is rendered.
        int tileX = static_cast<int>(relX * state.match.get_map().get_width() / state.dst_map_to_display.w);
        int tileY = static_cast<int>(relY * state.match.get_map().get_height() / state.dst_map_to_display.h);

        // Ensure the click is within the map bounds.
        if (tileX < 0 || tileX >= state.match.get_map().get_width() || tileY < 0 || tileY >= state.match.get_map().get_height()) {
            return SDL_APP_CONTINUE;
        }

        MapTile tile = state.match.get_map().get_tile(tileX, tileY);

        if (state.match.get_game_state() == GameState::SelectingStartingPoint) {
            // For example, only allow non-water tiles as starting positions.
            if (tile.type != MapTileType::Water) {
                // Set the player's starting tile.
                state.match.spawn_country(state.player_country_id, tileX, tileY);
                // Re-create the texture so that the new ownership shows.
                SDL_DestroyTexture(state.map_texture);
                state.map_texture = init_map_texture(state.renderer, state.match);
                SDL_SetTextureScaleMode(state.map_texture, SDL_SCALEMODE_NEAREST);
                // Transition to the in-game state.
                state.match.set_game_started();
            }
        } else if (state.match.get_game_state() == GameState::InGame) {
            // If the tile is not already owned by the player, check for an adjacent tile owned by the player.
            if (tile.owner != state.player_country_id)
                state.match.attack(state.player_country_id, tile.owner, state.troops_selected);
        }
        state.region_cache_needs_update = true;
    }
    return SDL_APP_CONTINUE;
}


/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    PROFILE_SECTION("SDL_AppIterate");

    AppState &state = *static_cast<AppState *>(appstate);
    SDL_Renderer *renderer = state.renderer;

    auto frame_time = SDL_GetTicks() - state.last_frame_time;
    state.last_frame_time = SDL_GetTicks();

    Profiler::instance().start_frame("Run callbacks");

    // run callbacks.
    for (auto it = state.callback_functions.begin(); it != state.callback_functions.end(); it++) {
        CQ_LOG_DEBUG << "Running callback...\n";
        if (!it->operator()()) {
            it = state.callback_functions.erase(it);
            --it;
        }
    }

    Profiler::instance().end_frame();

    {
        PROFILE_SECTION("Match tick");
        auto tiles_changed = state.match.tick();
        if (!tiles_changed.empty()) {
            sync_map_texture(state.map_texture, state.match, tiles_changed);
            state.region_cache_needs_update = true;
        }
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    draw_main_ui(state, frame_time);

    Profiler::instance().start_frame("Draw Map");
    draw_map_texture(state.map_texture, renderer, state.dst_map_to_display);
    Profiler::instance().end_frame();

    Profiler::instance().render_ui();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    CQ_LOG_RELEASE << "Exiting right now...\n";
    AppState *state = static_cast<AppState *>(appstate);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_DestroyTexture(state->map_texture);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    delete state;
}

