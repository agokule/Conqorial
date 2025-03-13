#include "ClientMap.h"
#include "MapTileTypes.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <cmath>
#include <cstdint>
#include <iostream>

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
}

SDL_Texture *init_map_texture(const Map &map, SDL_Renderer *renderer) {
    unsigned width = map.get_width(), height = map.get_height();

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    int pitch = 0;
    auto format = SDL_GetPixelFormatDetails(texture->format);
    uint8_t *pixels = nullptr;
    if (!SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch)) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    auto size = width * height * format->bytes_per_pixel;
    std::cout << "Allocating " << size << " bytes for map texture\n";
    std::cout << "Height is " << height << " and width is " << width << " and pitch is " << pitch << " and bytes per pixel is " << (short)format->bytes_per_pixel << std::endl;

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            MapTile tile = map.get_tile(x, y);
            auto color = get_tile_color(tile.type);
            pixels[y * pitch + x * format->bytes_per_pixel] = color.r;
            pixels[y * pitch + x * format->bytes_per_pixel + 1] = color.g;
            pixels[y * pitch + x * format->bytes_per_pixel + 2] = color.b;
            if (tile.type != MapTileType::Water)
                pixels[y * pitch + x * format->bytes_per_pixel + 3] = 255 - (std::pow((double)tile.elevation / 100.0, 2)) * 100;
            else
                pixels[y * pitch + x * format->bytes_per_pixel + 3] = 255 - tile.elevation * 2.5;
        }
    }

    SDL_UnlockTexture(texture);

    return texture;
}

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer, SDL_FRect dst_rect) {
    if (!SDL_RenderTexture(renderer, texture, NULL, &dst_rect)) {
        std::cerr << "Failed to render texture: " << SDL_GetError() << std::endl;
    }
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

