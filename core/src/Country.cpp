#include "Country.h"
#include "MapTile.h"
#include <iostream>
#include <optional>
#include <set>
#include <tuple>

bool Country::can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const {
    if (!(other_id != this->id && map.get_tile(pos.first, pos.second).type != MapTileType::Water))
        return false;
    if (map.get_tile(pos.first, pos.second).owner != other_id) {
        std::cerr << "Huh what? The other id does not equal the tile owner" << std::endl;
        return false;
    }

    // check if there is a border between the 2 countries
    int directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    for (unsigned y = 0; y < map.get_height(); y++) {
        for (unsigned x = 0; x < map.get_width(); x++) {
            if (map.get_tile(x, y).owner == other_id && map.get_tile(x, y).type != MapTileType::Water) {
                for (auto &dir : directions) {
                    int nx = x + dir[0];
                    int ny = y + dir[1];
                    if (nx >= 0 && nx < map.get_width() && ny >= 0 && ny < map.get_height()) {
                        MapTile neighbor = map.get_tile(nx, ny);
                        if (neighbor.owner == this->id) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}


std::tuple<bool, unsigned, std::set<std::pair<unsigned, unsigned>>> Country::attack(std::optional<Country> other, std::pair<unsigned, unsigned> pos, Map &map, unsigned troops) {
    unsigned other_id {};
    if (other.has_value())
        other_id = other->id;
    else
        other_id = 0;
    if (!can_attack(other_id, pos, map))
        return {false, troops, {}};

    double troop_cost_per_pixel = 1.0;
    
    if (other.has_value())
        troop_cost_per_pixel += (double)other->troops / this->troops - 1;

    unsigned pixels_to_capture {static_cast<unsigned int>(troops / troop_cost_per_pixel)};

    // get border between this and other
    std::set<std::pair<unsigned, unsigned>> border;
    int directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    for (unsigned y = 0; y < map.get_height(); y++) {
        for (unsigned x = 0; x < map.get_width(); x++) {
            if (map.get_tile(x, y).owner == this->id && map.get_tile(x, y).type != MapTileType::Water) {
                for (auto &dir : directions) {
                    int nx = x + dir[0];
                    int ny = y + dir[1];
                    if (nx >= 0 && nx < map.get_width() && ny >= 0 && ny < map.get_height()) {
                        MapTile neighbor = map.get_tile(nx, ny);
                        if (neighbor.owner == other_id && neighbor.type != MapTileType::Water)
                            border.insert({nx, ny});
                    }
                }
            }
        }
    }

    if (border.empty()) {
        std::cerr << "I thought there was a border??" << std::endl;
        return {false, troops, {}};
    }

    if (border.size() > pixels_to_capture) {
        std::cerr << "Not enought troops" << std::endl;
        return {false, troops, {}};
    }

    for (const auto &tile : border) {
        map.set_tile(tile, this->id);
        this->troops -= troop_cost_per_pixel;
        troops -= troop_cost_per_pixel;
    }

    return {true, troops, border};
}

