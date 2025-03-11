#ifndef MAP_TILE_H
#define MAP_TILE_H

#include "MapTileTypes.h"
#include "typedefs.h"

struct MapTile {
    unsigned x;
    unsigned y;

    double elevation;

    MapTileType type;
    PlayerId owner;
};

#endif // MAP_TILE_H

