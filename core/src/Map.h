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

    void set_tile(unsigned x, unsigned y, MapTileType type, CountryId owner);
    void set_tile(std::pair<unsigned, unsigned> pos, MapTileType type, CountryId owner);
    MapTile get_tile(unsigned x, unsigned y) const;
    MapTile get_tile(std::pair<unsigned, unsigned> pos) const;

    unsigned get_width() const { return width; }
    unsigned get_height() const { return height; }
};

#endif // MAP_H
