#ifndef MAP_TILE_H
#define MAP_TILE_H

#include "MapTileTypes.h"
#include "PlayerId.h"

struct MapTile {
    unsigned x;
    unsigned y;

    MapTileType type;
    PlayerId owner;
};

#endif // MAP_TILE_H

