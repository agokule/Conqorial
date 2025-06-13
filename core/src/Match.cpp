#include "Match.h"
#include "optional"
#include "Logging.h"
#include "typedefs.h"

bool check_time_to_update(std::chrono::time_point<std::chrono::high_resolution_clock> last_update, std::chrono::milliseconds interval) {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update) > interval;
}


Match::Match(unsigned width, unsigned height): countries {}, map {width, height}, random {} {
    countries.emplace(0, Country { 0, "Neutral", {0, 0, 0} });
    tiles_owned_by_country[0] = {};

    spawn_and_create_ai_countries();
}

const Country &Match::get_country(CountryId id) const {
    return countries.at(id);
}

const Country &Match::new_country(std::string name, bool is_player, Color color) {
    CountryId id = countries.size();
    tiles_owned_by_country[id] = {};
    RandomGenerator *random_arg = is_player ? &random : nullptr;
    return countries.insert({ id, Country { id, name, color, random_arg } }).first->second;
}

void Match::spawn_country(CountryId id, TileCoor x, TileCoor y) {
    std::array<std::pair<TileCoor, TileCoor>, 25> coords {
        std::pair {x, y},
        {x + 1, y}, {x, y + 1},
        {x - 1, y}, {x, y - 1},
        {x - 1, y - 1}, {x + 1, y - 1},
        {x - 1, y + 1}, {x + 1, y + 1},
        {x + 2, y}, {x - 2, y},
        {x, y + 2}, {x, y - 2},
        {x + 2, y + 2}, {x + 2, y - 2},
        {x - 2, y + 2}, {x - 2, y - 2},
        {x + 1, y + 2}, {x + 1, y - 2},
        {x - 1, y + 2}, {x - 1, y - 2},
        {x + 2, y + 1}, {x + 2, y - 1},
        {x - 2, y + 1}, {x - 2, y - 1},
    };
    for (auto &coord : coords) {
        if (map.get_tile(coord).owner != 0 ||
            map.get_tile(coord).type == MapTileType::Water)
            continue;
        set_map_tile(coord, id);
    }
}

void Match::set_game_started() {
    game_state = GameState::InGame;
}

GameState Match::get_game_state() const {
    return game_state;
}

void Match::new_alliance(CountryId id1, CountryId id2) {
    alliances[id1].push_back(id2);
    alliances[id2].push_back(id1);
}

std::vector<std::pair<TileCoor, TileCoor>> Match::tick() {
    if (game_state != GameState::InGame)
        return {};


    std::vector<std::pair<TileCoor, TileCoor>> result;
    auto now = std::chrono::high_resolution_clock::now();
    if (check_time_to_update(last_attack_update, attack_update_intervalCE)) {
        last_attack_update = now;
        auto tiles_changed = update_attacks();
        result.insert(result.end(), tiles_changed.begin(), tiles_changed.end());
    }
    if (check_time_to_update(last_population_update, population_update_intervalCE)) {
        last_population_update = now;
        update_populations();
    }
    if (check_time_to_update(last_ai_update, ai_update_intervalCE)) {
        last_ai_update = now;
        update_ai_decisions();
    }

    return result;
}

std::vector<std::pair<TileCoor, TileCoor>> Match::update_attacks() {
    std::vector<std::pair<TileCoor, TileCoor>> tiles_changed;
    for (auto &[attacker, attacks] : on_going_attacks) {
        for (auto it = attacks.begin(); it != attacks.end(); ++it) {
            auto tiles_changed_this_attack = it->second.advance(map, countries, tiles_owned_by_country);
            if (tiles_changed_this_attack.empty()) {
                it = attacks.erase(it);
                if (attacks.empty())
                    break;
            }
            tiles_changed.insert(tiles_changed.end(), tiles_changed_this_attack.begin(), tiles_changed_this_attack.end());
        }
    }
    return tiles_changed;
}

void Match::spawn_and_create_ai_countries() {
    constexpr unsigned num_ai_countries = 15;
    std::array<CountryId, num_ai_countries> ai_countries;

    for (unsigned i = 0; i < num_ai_countries; i++) {
        ai_countries[i] = new_country("Bot " + std::to_string(i), false, {
                static_cast<uint8_t>(random.randint(0, 255)),
                static_cast<uint8_t>(random.randint(0, 255)),
                static_cast<uint8_t>(random.randint(0, 255))
        }).id;
    }

    auto current_ai_country {ai_countries.begin()};
    for (unsigned tile = 0; tile < map.get_width() * map.get_height(); ++tile) {
        if (map.get_tile(tile).owner != 0)
            continue;
        if (map.get_tile(tile).type == MapTileType::Water)
            continue;

        spawn_country(*current_ai_country, random.randint(0, map.get_width() - 1), random.randint(0, map.get_height() - 1));
        tile += 20;
        current_ai_country++;
        if (current_ai_country == ai_countries.end())
            break;
    }
}

void Match::update_populations() {
    for (auto &[id, country] : countries) {
        auto number_tiles = tiles_owned_by_country[id].size();
        if (number_tiles == 0)
            continue;
        auto current_population = country.pyramid.get_total_population();
        auto economy = PyramidUtils::get_economy_score(country.pyramid, country.id, country.get_target_mobilization_level());

        country.set_economy(economy.score);
        country.set_density(current_population / number_tiles);
        country.add_money(economy.money_made);
        country.pyramid.tick(economy.score, current_population / number_tiles, country.urbanization_level);
        country.calculate_troops();
    }
}

void Match::update_ai_decisions() { }

void Match::attack(CountryId attacker, CountryId defender_id, unsigned troops_to_attack, TileCoor tile_x, TileCoor tile_y) {
    bool able_to_attack = get_country(attacker).can_attack(defender_id, {tile_x, tile_y}, map);

    CQ_LOG_DEBUG << "Can attack: " << able_to_attack << "\n";
    if (!able_to_attack)
        return;

    std::optional<Country> defender {};
    if (defender_id != 0)
        defender = get_country(defender_id);

    auto &ongoing_attacks_for_player = on_going_attacks[attacker];
    bool attack_exists = ongoing_attacks_for_player.find(defender_id) != ongoing_attacks_for_player.end();

    // If the player is already attacking the defender, simply add troops to the attack.
    if (attack_exists) {
        ongoing_attacks_for_player.at(defender_id).troops_to_attack += troops_to_attack;
        return;
    }

    ongoing_attacks_for_player.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(defender_id),
        std::forward_as_tuple(attacker, defender_id, troops_to_attack)
    );
}


const Map &Match::get_map() const {
    return map;
}

void Match::set_map_tile(TileCoor x, TileCoor y, CountryId owner) {
    TileIndex index = map.get_tile_index(x, y);
    tiles_owned_by_country[owner].insert(index);
    auto removed = tiles_owned_by_country[map.get_tile(x, y).owner].erase(index);
    if (map.get_tile(x, y).owner != 0) {
        CONQORIAL_ASSERT_ALL(removed != 0, "The country which owns the tile does not have it in their tiles_owned_by_country set",
                std::cerr << "Country: " << (short)map.get_tile(x, y).owner << "\n";);
    }
    map.set_tile(x, y, owner);
}

void Match::set_map_tile(std::pair<TileCoor, TileCoor> pos, CountryId owner) {
    set_map_tile(pos.first, pos.second, owner);
}

void Match::set_country_target_mobilization_level(CountryId id, uint8_t level) {
    countries.at(id).set_target_mobilization_level(level);
}

const std::map<CountryId, Country> &Match::get_countries() const {
    return countries;
}
