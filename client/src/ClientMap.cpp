#include "ClientMap.h"
#include "Logging.h"
#include "MapTileTypes.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "typedefs.h"
#include <cmath>
#include <cstdint>

SDL_Color get_tile_color(MapTileType type) {
    switch (type) {
        case MapTileType::Water:
            return {0, 0, 220, 255};
        case MapTileType::Beach:
            return {215, 215, 0, 255};
        case MapTileType::Grass:
            return {13, 159, 0, 255};
        case MapTileType::Hill:
            return {8, 89, 0, 255};
        case MapTileType::Mountain:
            return {100, 100, 100, 255};
    }
    CQ_LOG_DIST_ERROR << "Unknown map tile type: " << static_cast<int>(type);
    return {0, 0, 0, 0};
}

SDL_Color get_tile_display_color(const MapTile &tile, const Match &match) {
    SDL_Color color = get_tile_color(tile.type);
    // If the tile has been conquered (owner != 0), tint the color.
    if (tile.owner != 0) {
        color.r = (color.r + match.get_country(tile.owner).get_color().r) / 2;
        color.g = (color.g + match.get_country(tile.owner).get_color().g) / 2;
        color.b = (color.b + match.get_country(tile.owner).get_color().b) / 2;
    }
    if (tile.type != MapTileType::Water)
        color.a = 255 - (std::pow((double)tile.elevation / 100.0, 2)) * 100;
    else
        color.a = 255 - tile.elevation * 2.5;
    return color;
}

SDL_Texture *init_map_texture(SDL_Renderer *renderer, const Match &match) {
    const Map &map = match.get_map();
    unsigned width = map.get_width(), height = map.get_height();

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);

    CONQORIAL_ASSERT_ALL(texture, SDL_GetError());
    if (!texture)
        return nullptr;

    int pitch = 0;
    auto format = SDL_GetPixelFormatDetails(texture->format);
    uint8_t *pixels = nullptr;

    bool locked = SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
    CONQORIAL_ASSERT_ALL(locked, SDL_GetError());
    if (!locked)
        return nullptr;

    auto size = width * height * format->bytes_per_pixel;
    CQ_LOG_DEBUG << "Allocating " << size << " bytes for map texture\n";
    CQ_LOG_DEBUG << "Height is " << height << " and width is " << width << " and pitch is " << pitch << " and bytes per pixel is " << (short)format->bytes_per_pixel << '\n';

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            MapTile tile = map.get_tile(x, y);
            auto color = get_tile_display_color(tile, match);
            pixels[y * pitch + x * format->bytes_per_pixel] = color.r;
            pixels[y * pitch + x * format->bytes_per_pixel + 1] = color.g;
            pixels[y * pitch + x * format->bytes_per_pixel + 2] = color.b;
            pixels[y * pitch + x * format->bytes_per_pixel + 3] = color.a;
        }
    }

    SDL_UnlockTexture(texture);

    return texture;
}

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer, SDL_FRect dst_rect) {
    CONQORIAL_ASSERT_ALL(SDL_RenderTexture(renderer, texture, NULL, &dst_rect), SDL_GetError());
}

void zoom_map(float zoom_factor, float center_x, float center_y, AppState &state) {
    // Compute the offset of the mouse relative to the current view's top-left corner
    float offsetX = center_x - state.dst_map_to_display.x;
    float offsetY = center_y - state.dst_map_to_display.y;

    // Update the width and height of the view rectangle
    state.dst_map_to_display.w *= zoom_factor;
    state.dst_map_to_display.h *= zoom_factor;

    // Update the top-left coordinates so the mouse position remains fixed in the map
    state.dst_map_to_display.x = center_x - offsetX * zoom_factor;
    state.dst_map_to_display.y = center_y - offsetY * zoom_factor;
}

void sync_map_texture(SDL_Texture *texture, const Match &match, const std::vector<std::pair<TileCoor, TileCoor>> &tiles_to_update) {
    if (!tiles_to_update.empty()) {
        uint8_t *pixels = nullptr;
        int pitch = 0;
        auto format = SDL_GetPixelFormatDetails(texture->format);
        SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
        for (auto [x, y] : tiles_to_update) {
            MapTile tile = match.get_map().get_tile(x, y);
            auto color = get_tile_display_color(tile, match);
            pixels[y * pitch + x * format->bytes_per_pixel] = color.r;
            pixels[y * pitch + x * format->bytes_per_pixel + 1] = color.g;
            pixels[y * pitch + x * format->bytes_per_pixel + 2] = color.b;
            pixels[y * pitch + x * format->bytes_per_pixel + 3] = color.a;
        }
        SDL_UnlockTexture(texture);
    }
}

std::optional<std::pair<TileCoor, TileCoor>> convert_screen_to_map_coors(float x, float y, const AppState &state) {
    float relX = x - state.dst_map_to_display.x;
    float relY = y - state.dst_map_to_display.y;
    // Scale based on how the map texture is rendered.
    long tileX = static_cast<long>(relX * state.match.get_map().get_width() / state.dst_map_to_display.w);
    long tileY = static_cast<long>(relY * state.match.get_map().get_height() / state.dst_map_to_display.h);

    // Ensure the click is within the map bounds.
    if (tileX < 0 || tileX >= state.match.get_map().get_width() || tileY < 0 || tileY >= state.match.get_map().get_height())
        return {};

    return {{tileX, tileY}};
}

std::pair<float, float> convert_map_to_screen_coors(TileCoor x, TileCoor y, const AppState &state) {
    float relX = x * state.dst_map_to_display.w / state.match.get_map().get_width();
    float relY = y * state.dst_map_to_display.h / state.match.get_map().get_height();
    
    // Convert relative coordinates to absolute screen coordinates
    float screenX = relX + state.dst_map_to_display.x;
    float screenY = relY + state.dst_map_to_display.y;
    
    return {screenX, screenY};
}
