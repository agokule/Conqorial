#include "Country.h"
#include "MapTile.h"
#include "Logging.h"

bool Country::can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const {
    if (!(other_id != this->id && map.get_tile(pos.first, pos.second).type != MapTileType::Water))
        return false;
    if (map.get_tile(pos.first, pos.second).owner != other_id) {
        CQ_LOG_RELEASE_ERROR << "Huh what? The other id does not equal the tile owner" << '\n';
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

unsigned Country::get_troops() const {
    return troops;
}

std::string Country::get_name() const {
    return name;
}

bool Country::get_is_human() const {
    return is_human;
};

Color Country::get_color() const {
    return color;
}

CountryId Country::get_id() const {
    return id;
}

const PopulationPyramid &Country::get_pyramid() const {
    return pyramid;
}

unsigned Country::get_economy() const {
    return last_economy;
}

unsigned Country::get_density() const {
    return last_density;
}

void Country::set_economy(unsigned economy) {
    last_economy = economy;
}

void Country::set_density(unsigned density) {
    last_density = density;
}

unsigned Country::get_urbanization_level() const {
    return urbanization_level;
}

unsigned Country::upgrade_urbanization_level() {
    return ++urbanization_level;
}

unsigned Country::get_money() const {
    return money;
}

void Country::add_money(unsigned amount) {
    money += amount;
}

void Country::remove_money(unsigned amount) {
    CONQORIAL_ASSERT_ALL(amount <= money, "Tried to remove more money than owned", return;);
    money -= amount;
}
