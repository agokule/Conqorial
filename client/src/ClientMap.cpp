#include "ClientMap.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include <cstdint>
#include <iostream>

SDL_Color get_tile_color(MapTileType type) {
    switch (type) {
        case MapTileType::Water:
            return {0, 0, 255, 255};
        case MapTileType::Beach:
            return {255, 255, 0, 255};
        case MapTileType::Grass:
            return {0, 255, 0, 255};
        case MapTileType::Hill:
            return {255, 255, 100, 255};
        case MapTileType::Mountain:
            return {100, 100, 100, 255};
    }
}

SDL_Texture *init_map_texture(const Map &map, SDL_Renderer *renderer, unsigned width, unsigned height) {
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
            pixels[y * pitch + x * format->bytes_per_pixel + 3] = color.a;
            if (x == width - 1)
                std::cout << "Drawing map tile: " << (short)tile.elevation << "(" << (short)color.r << " " << (short)color.g << " " << (short)color.b << " " << (short)color.a << ")\n";
        }
    }

    SDL_UnlockTexture(texture);

    return texture;
}

void draw_map_texture(SDL_Texture *texture, SDL_Renderer *renderer) {
    SDL_FRect dst_rect;

    /* center this one and make it grow and shrink. */
    dst_rect.w = (float) texture->w;
    dst_rect.h = (float) texture->h;
    dst_rect.x = 0.0f;
    dst_rect.y = 0.0f;
    if (!SDL_RenderTexture(renderer, texture, NULL, NULL)) {
        std::cerr << "Failed to render texture: " << SDL_GetError() << std::endl;
    }
}


