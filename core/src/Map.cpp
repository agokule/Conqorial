#include "Map.h"
#include "MapTileTypes.h"
#include "noise_wrapper.h"
#include "typedefs.h"

MapTileType get_tile_type(Elevation elevation) {
    if (elevation >= MapTileType::Mountain)
        return MapTileType::Mountain;
    else if (elevation >= MapTileType::Hill)
        return MapTileType::Hill;
    else if (elevation >= MapTileType::Grass)
        return MapTileType::Grass;
    else if (elevation >= MapTileType::Beach)
        return MapTileType::Beach;
    else
        return MapTileType::Water;
}

Map::Map(unsigned width, unsigned height) : width(width), height(height), tiles(width * height), noise() {
    noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin);

    // for now, just use the same seed
    noise.SetSeed(0);

    unsigned index {};

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            auto elevation = get_elevation(noise, x, y);
            tiles[index].type = get_tile_type(elevation);
            tiles[index].elevation = elevation;
            index++;
        }
    }
}

void Map::set_tile(unsigned x, unsigned y, MapTileType type, PlayerId owner) {
    tiles[y * width + x].type = type;
    tiles[y * width + x].owner = owner;
}

MapTile Map::get_tile(unsigned x, unsigned y) const {
    return tiles[y * width + x];
}
