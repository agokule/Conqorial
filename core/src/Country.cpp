#include "Country.h"
#include "MapTile.h"
#include "Logging.h"
#include "PopulationPyramid.h"
#include <chrono>
#include <optional>

AIPlayerBehavior::AIPlayerBehavior(RandomGenerator &random) {
    check_decision_interval = random.randint(ai_check_attack_interval_minCE, ai_check_attack_interval_maxCE);
    target_mobilization_level = random.randint(ai_mobilization_level_minCE, ai_mobilization_level_maxCE);
    reserve_troops = random.randint(ai_reserve_troops_minCE, ai_reserve_troops_maxCE);

    update_last_attack_check();
}

void AIPlayerBehavior::update_last_attack_check() {
    last_descision_check = std::chrono::steady_clock::now();
    auto duration = last_descision_check.time_since_epoch();
    CQ_LOG_DEBUG << "Last attack check: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << '\n';
}

Country::Country(CountryId id, std::string name, Color color, RandomGenerator *random)
    : id {id}, name {name}, is_human {random == nullptr}, color {color} {
    if (random != nullptr)
        ai_behavior = AIPlayerBehavior(*random);
    else
        ai_behavior = std::nullopt;
}

bool Country::can_attack(CountryId other_id, const Map &map) const {
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

void Country::calculate_troops() {
    troops = 0;
    double target_mobilization_level_percent {target_mobilization_level / 100.0};
    for (const auto piece : pyramid.get_pieces()) {
        if (piece.age >= reproductive_age_min && piece.age <= reproductive_age_max)
            troops += (piece.male_count + piece.female_count) * target_mobilization_level_percent;
        if (piece.age > reproductive_age_max)
            break;
    }
}

unsigned long Country::get_military_score() const {
    return troops * millitary_level;
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

unsigned Country::calculate_millitary_score() const {
    return millitary_level * troops;
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

unsigned Country::get_millitary_level() const {
    return millitary_level;
}

unsigned Country::upgrade_millitary_level() {
    return ++millitary_level;
}

uint8_t Country::get_target_mobilization_level() const {
    return target_mobilization_level;
}
void Country::set_target_mobilization_level(uint8_t level) {
    target_mobilization_level = level;
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
