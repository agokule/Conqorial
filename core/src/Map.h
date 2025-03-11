#ifndef MAP_H
#define MAP_H

#include <vector>
#include "FastNoiseLite/FastNoiseLite.h"
#include "MapTile.h"
#include "MapTileTypes.h"
#include "typedefs.h"

class Map {
    unsigned width;
    unsigned height;

    std::vector<MapTile> tiles;
    FastNoiseLite noise;
public:
    Map(unsigned width, unsigned height);

    void set_tile(unsigned x, unsigned y, MapTileType type, PlayerId owner);
    MapTile get_tile(unsigned x, unsigned y) const;
};

#endif // MAP_H
