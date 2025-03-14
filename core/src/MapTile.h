#ifndef MAP_TILE_H
#define MAP_TILE_H

#include "MapTileTypes.h"
#include "typedefs.h"
#include <cstdint>

struct MapTile {
    uint8_t elevation;

    MapTileType type;
    CountryId owner;
};
constexpr size_t MAP_TILE_SIZE = sizeof(MapTile);

#endif // MAP_TILE_H

