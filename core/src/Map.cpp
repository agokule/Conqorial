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
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(7);
    noise.SetFractalLacunarity(2.0);
    noise.SetFractalGain(0.47);
    noise.SetFractalWeightedStrength(0.08);


    unsigned index {};

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            auto elevation = get_elevation(noise, x, y) * 100;
            tiles[index].type = get_tile_type(elevation);
            tiles[index].elevation = elevation;
            index++;
        }
    }
}

void Map::set_tile(unsigned x, unsigned y, CountryId owner) {
    tiles[y * width + x].owner = owner;
}


void Map::set_tile(std::pair<unsigned, unsigned> pos, CountryId owner) {
    set_tile(pos.first, pos.second, owner);
}

void Map::set_tile(TileIndex pos, CountryId owner) {
    set_tile(get_tile_coors(pos), owner);
}

MapTile Map::get_tile(unsigned x, unsigned y) const {
    return tiles[y * width + x];
}

MapTile Map::get_tile(std::pair<unsigned, unsigned> pos) const {
    return get_tile(pos.first, pos.second);
}

MapTile Map::get_tile(TileIndex pos) const {
    return get_tile(get_tile_coors(pos));
}

unsigned Map::get_width() const {
    return width;
}

unsigned Map::get_height() const {
    return height;
}

TileIndex Map::get_tile_index(TileCoor x, TileCoor y) const {
    return y * width + x;
}

TileIndex Map::get_tile_index(std::pair<TileCoor, TileCoor> pos) const {
    return get_tile_index(pos.first, pos.second);
}

std::pair<TileCoor, TileCoor> Map::get_tile_coors(TileIndex index) const {
    return { index % width, index / width };
}

Map::BorderResult Map::get_border(CountryId from, const std::map<CountryId, std::set<TileIndex>> &tiles_owned_by_country, std::optional<CountryId> to) const {
    std::set<TileIndex> border;
    std::set<CountryId> neighbors;
    constexpr int directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    for (TileIndex tile : tiles_owned_by_country.at(from)) {
        auto [x, y] = get_tile_coors(tile);
        for (auto &dir : directions) {
            int nx = x + dir[0];
            int ny = y + dir[1];
            if (nx >= 0 && nx < (int)get_width() && ny >= 0 && ny < (int)get_height()) {
                MapTile neighbor = get_tile(nx, ny);
                if (to.has_value() && neighbor.owner != to.value())
                    continue;
                if (neighbor.owner != from && neighbor.type != MapTileType::Water) {
                    border.insert(get_tile_index(x, y));
                    neighbors.insert(neighbor.owner);
                }
            }
        }
    }
    return {border, neighbors};
}


