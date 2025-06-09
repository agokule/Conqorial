#ifndef MAP_H
#define MAP_H

#include <vector>
#include "FastNoiseLite/FastNoiseLite.h"
#include "MapTile.h"
#include "typedefs.h"

class Map {
    unsigned width;
    unsigned height;

    std::vector<MapTile> tiles;
    FastNoiseLite noise;
public:
    Map(unsigned width, unsigned height);

    // This should not be used outside of the Match and Attack class!
    void set_tile(unsigned x, unsigned y, CountryId owner);
    // This should not be used outside of the Match and Attack class!
    void set_tile(std::pair<unsigned, unsigned> pos, CountryId owner);
    MapTile get_tile(unsigned x, unsigned y) const;
    MapTile get_tile(std::pair<unsigned, unsigned> pos) const;

    unsigned get_width() const { return width; }
    unsigned get_height() const { return height; }

    TileIndex get_tile_index(TileCoor x, TileCoor y) const { return y * width + x; }
    TileIndex get_tile_index(std::pair<TileCoor, TileCoor> pos) const { return get_tile_index(pos.first, pos.second); }
    std::pair<TileCoor, TileCoor> get_tile_coors(TileIndex index) const { return { index % width, index / width }; }
};

#endif // MAP_H
