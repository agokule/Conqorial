#include "Attack.h"
#include "ClientMap.h"
#include "GameState.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <optional>
#include <sys/stat.h>
#include <utility>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Logging.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "implot.h"

#include "AppState.h"

#include "Map.h"

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
    state->map_texture = init_map_texture(state->map, state->renderer, state->countries);

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
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // Convert mouse coordinates (from the event) to map tile coordinates.
        float relX = event->button.x - state.dst_map_to_display.x;
        float relY = event->button.y - state.dst_map_to_display.y;
        // Scale based on how the map texture is rendered.
        int tileX = static_cast<int>(relX * state.map.get_width() / state.dst_map_to_display.w);
        int tileY = static_cast<int>(relY * state.map.get_height() / state.dst_map_to_display.h);

        // Ensure the click is within the map bounds.
        if (tileX < 0 || tileX >= state.map.get_width() || tileY < 0 || tileY >= state.map.get_height()) {
            return SDL_APP_CONTINUE;
        }

        MapTile tile = state.map.get_tile(tileX, tileY);

        if (state.game_state == GameState::SelectingStartingPoint) {
            // For example, only allow non-water tiles as starting positions.
            if (tile.type != MapTileType::Water) {
                // Set the player's starting tile.
                state.map.set_tile(tileX, tileY, state.player_country.get_id());
                // Re-create the texture so that the new ownership shows.
                SDL_DestroyTexture(state.map_texture);
                state.map_texture = init_map_texture(state.map, state.renderer, state.countries);
                SDL_SetTextureScaleMode(state.map_texture, SDL_SCALEMODE_NEAREST);
                // Transition to the in-game state.
                state.game_state = GameState::InGame;
            }
        } else if (state.game_state == GameState::InGame) {
            // If the tile is not already owned by the player, check for an adjacent tile owned by the player.
            if (tile.owner != state.player_country.get_id()) {
                unsigned troops_to_attack = 10000;
                bool able_to_attack = state.player_country.can_attack(tile.owner, {tileX, tileY}, state.map);

                CQ_LOG_DEBUG << "Can attack: " << able_to_attack << "\n";
                if (!able_to_attack)
                    return SDL_APP_CONTINUE;

                std::optional<Country> defender {};
                if (tile.owner != 0)
                    defender = state.countries.at(tile.owner);

                auto &ongoing_attacks_for_player = state.on_going_attacks[state.player_country.get_id()];
                bool attack_exists = ongoing_attacks_for_player.find(tile.owner) != ongoing_attacks_for_player.end();

                // If the player is already attacking the defender, simply add troops to the attack.
                if (attack_exists) {
                    ongoing_attacks_for_player.at(tile.owner).troops_to_attack += troops_to_attack;
                    return SDL_APP_CONTINUE;
                }

                auto attack_info = ongoing_attacks_for_player.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(tile.owner),
                    std::forward_as_tuple(state.player_country.get_id(), tile.owner, troops_to_attack)
                );
                auto &attack = attack_info.first;

                auto callback = [attack, &state]() {
                    CQ_LOG_DEBUG << "Updating attack...\n";
                    auto tiles_to_update = attack->second.advance(state.map, state.countries);
                    if (!tiles_to_update.empty()) {
                        uint8_t *pixels = nullptr;
                        int pitch = 0;
                        auto format = SDL_GetPixelFormatDetails(state.map_texture->format);
                        SDL_LockTexture(state.map_texture, NULL, (void**)&pixels, &pitch);
                        for (auto [x, y] : tiles_to_update) {
                            MapTile tile = state.map.get_tile(x, y);
                            auto color = get_tile_display_color(tile, state.countries);
                            pixels[y * pitch + x * format->bytes_per_pixel] = color.r;
                            pixels[y * pitch + x * format->bytes_per_pixel + 1] = color.g;
                            pixels[y * pitch + x * format->bytes_per_pixel + 2] = color.b;
                            pixels[y * pitch + x * format->bytes_per_pixel + 3] = color.a;
                        }
                        SDL_UnlockTexture(state.map_texture);
                    }
                    if (tiles_to_update.empty())
                        state.on_going_attacks[state.player_country.get_id()].erase(attack);
                    return !tiles_to_update.empty();
                };
                state.callback_functions.emplace_back(callback);
            }
        }
    }
    return SDL_APP_CONTINUE;
}


/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState &state = *static_cast<AppState *>(appstate);
    SDL_Renderer *renderer = state.renderer;

    auto frame_time = SDL_GetTicks() - state.last_frame_time;
    state.last_frame_time = SDL_GetTicks();

    // run callbacks.
    for (auto it = state.callback_functions.begin(); it != state.callback_functions.end(); it++) {
        CQ_LOG_DEBUG << "Running callback...\n";
        CQ_LOG_DEBUG << (it == state.callback_functions.end()) << '\n';
        if (!it->operator()()) {
            it = state.callback_functions.erase(it);
            --it;
        }
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    ImGui::Begin("Hello there");
    if (ImGui::Button("Reset view")) {
        state.dst_map_to_display = { 0, 0, (float)state.map.get_width(), (float)state.map.get_height()};
    }
    ImGui::Text("Frame time: %llu", frame_time);
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

    state.frame_rates.AddPoint(SDL_GetTicks(), ImGui::GetIO().Framerate);

    if (ImPlot::BeginPlot("Frame Rate")) {
        ImPlot::SetupAxisLimits(ImAxis_X1, (state.frame_rates.Data.begin() + state.frame_rates.Offset)->x,
                                SDL_GetTicks(), ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 240, ImGuiCond_Always);
        ImPlot::PlotLine("FPS", &state.frame_rates.Data[0].x, &state.frame_rates.Data[0].y,
                         state.frame_rates.Data.size(), 0, state.frame_rates.Offset, 2 * sizeof(float));
        ImPlot::EndPlot();
    }
    ImGui::End();

    draw_map_texture(state.map_texture, renderer, state.dst_map_to_display);

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
