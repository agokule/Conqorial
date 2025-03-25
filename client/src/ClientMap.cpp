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

SDL_Color get_tile_display_color(const MapTile &tile, const std::map<CountryId, Country> &countries) {
    SDL_Color color = get_tile_color(tile.type);
    // If the tile has been conquered (owner != 0), tint the color.
    if (tile.owner != 0) {
        color.r = (color.r + countries.at(tile.owner).get_color().r) / 2;
        color.g = (color.g + countries.at(tile.owner).get_color().g) / 2;
        color.b = (color.b + countries.at(tile.owner).get_color().b) / 2;
    }
    if (tile.type != MapTileType::Water)
        color.a = 255 - (std::pow((double)tile.elevation / 100.0, 2)) * 100;
    else
        color.a = 255 - tile.elevation * 2.5;
    return color;
}

SDL_Texture *init_map_texture(const Map &map, SDL_Renderer *renderer, const std::map<CountryId, Country> &countries) {
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
            auto color = get_tile_display_color(tile, countries);
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

