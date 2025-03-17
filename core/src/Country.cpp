#include "Country.h"
#include "MapTile.h"
#include "Logging.h"

bool Country::can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const {
    if (!(other_id != this->id && map.get_tile(pos.first, pos.second).type != MapTileType::Water))
        return false;
    if (map.get_tile(pos.first, pos.second).owner != other_id) {
        LOG_RELEASE_ERROR << "Huh what? The other id does not equal the tile owner" << '\n';
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

