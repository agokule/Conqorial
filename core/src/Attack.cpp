#include "Attack.h"
#include "Logging.h"
#include "typedefs.h"
#include <optional>

std::set<std::pair<TileCoor, TileCoor>> Attack::advance(Map &map, std::map<CountryId, Country> &countries, std::map<CountryId, std::set<TileIndex>> &tiles_owned_by_country) {
    double troop_cost_per_pixel = 1.0;

    std::optional<Country> defender {};
    if (this->defender != 0)
        defender = countries.at(this->defender);

    Country &attacker = countries.at(this->attacker);

    if (defender.has_value()) {
        troop_cost_per_pixel += (double)defender->troops / attacker.troops - 1;
    }

    CQ_LOG_DEBUG << "Troop cost per pixel: " << troop_cost_per_pixel << '\n';
    CQ_LOG_DEBUG << "Troops to attack: " << this->troops_to_attack << '\n';

    const unsigned pixels_to_capture {static_cast<unsigned int>(this->troops_to_attack / troop_cost_per_pixel)};

    // get border between this and other
    std::set<std::pair<TileCoor, TileCoor>> border;
    constexpr int directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    if (this->current_boder.empty()) {
        // no cached border, do a full check over the entire map
        for (unsigned y = 0; y < map.get_height(); y++) {
            for (unsigned x = 0; x < map.get_width(); x++) {
                if (map.get_tile(x, y).owner == defender->id && map.get_tile(x, y).type != MapTileType::Water) {
                    for (auto &dir : directions) {
                        int nx = x + dir[0];
                        int ny = y + dir[1];
                        if (nx >= 0 && nx < (int)map.get_width() && ny >= 0 && ny < (int)map.get_height()) {
                            MapTile neighbor = map.get_tile(nx, ny);
                            if (neighbor.owner == attacker.get_id() && neighbor.type != MapTileType::Water) {
                                border.insert({x, y});
                            }
                        }
                    }
                }
            }
        }
    } else {
        // just check the cached border
        for (auto [x, y] : this->current_boder) {
            for (auto &dir : directions) {
                int nx = x + dir[0];
                int ny = y + dir[1];
                if (nx >= 0 && nx < (int)map.get_width() && ny >= 0 && ny < (int)map.get_height()) {
                    MapTile neighbor = map.get_tile(nx, ny);
                    if (neighbor.owner == defender->id && neighbor.type != MapTileType::Water) {
                        border.insert({nx, ny});
                    }
                }
            }
        }
        CQ_LOG_DEBUG << "Cached border size: " << this->current_boder.size() << '\n';
    }

    CONQORIAL_ASSERT_ALL(!border.empty(), "I thought there was a border but it is empty???", return {};);

    if (border.size() > pixels_to_capture) {
        CQ_LOG_RELEASE_ERROR << "Not enough troops" << '\n';
        return {};
    }

    for (const auto &tile : border) {
        tiles_owned_by_country[this->attacker].insert(map.get_tile_index(tile));
        tiles_owned_by_country[this->defender].erase(map.get_tile_index(tile));
        map.set_tile(tile, attacker.get_id());
        attacker.troops -= troop_cost_per_pixel;
        this->troops_to_attack -= troop_cost_per_pixel;
    }
    this->current_boder = border;
    return border;
}

