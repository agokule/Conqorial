#ifndef MAP_H
#define MAP_H

#include <vector>
#include "FastNoiseLite/FastNoiseLite.h"
#include "MapTile.h"
#include "typedefs.h"
#include <set>
#include <optional>
#include <map>

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
    // This should not be used outside of the Match and Attack class!
    void set_tile(TileIndex pos, CountryId owner);
    MapTile get_tile(unsigned x, unsigned y) const;
    MapTile get_tile(std::pair<unsigned, unsigned> pos) const;
    MapTile get_tile(TileIndex pos) const;

    unsigned get_width() const;
    unsigned get_height() const;

    TileIndex get_tile_index(TileCoor x, TileCoor y) const;
    TileIndex get_tile_index(std::pair<TileCoor, TileCoor> pos) const;
    std::pair<TileCoor, TileCoor> get_tile_coors(TileIndex index) const;

    struct BorderResult {
        std::set<TileIndex> border;
        std::set<CountryId> neighbors;
    };
    BorderResult get_border(CountryId from, const std::map<CountryId, std::set<TileIndex>> &tiles_owned_by_country, std::optional<CountryId> to = std::nullopt) const;

};

#endif // MAP_H
