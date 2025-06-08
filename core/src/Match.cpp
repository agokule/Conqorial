#include "Match.h"
#include "optional"
#include "Logging.h"
#include "typedefs.h"

bool check_time_to_update(std::chrono::time_point<std::chrono::high_resolution_clock> last_update, std::chrono::milliseconds interval) {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update) > interval;
}

void Match::spawn_country(CountryId id, TileCoor x, TileCoor y) {
    std::array<std::pair<TileCoor, TileCoor>, 9> coords = {
        std::pair {x, y},
        {x + 1, y},
        {x, y + 1},
        {x - 1, y},
        {x, y - 1},
        {x - 1, y - 1},
        {x + 1, y - 1},
        {x - 1, y + 1},
        {x + 1, y + 1} 
    };
    for (auto &coord : coords) {
        if (map.get_tile(coord.first, coord.second).owner != 0 ||
            map.get_tile(coord.first, coord.second).type == MapTileType::Water)
            continue;
        map.set_tile(coord.first, coord.second, id);
    }
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
    if (check_time_to_update(last_economy_update, economy_update_intervalCE)) {
        last_economy_update = now;
        update_economies();
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
        for (auto it = attacks.begin(); it != attacks.end(); it++) {
            auto tiles_changed_this_attack = it->second.advance(map, countries);
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

void Match::update_populations() {
    for (auto &[id, country] : countries) {
        auto economy = PyramidUtils::get_economy_score(country.pyramid, country.id);
        country.pyramid.tick(economy.score, 1000'000, country.urbanization_level);
    }
}

void Match::update_economies() { }
void Match::update_ai_decisions() { }

void Match::attack(CountryId attacker, CountryId defender_id, unsigned troops_to_attack, unsigned tile_x, unsigned tile_y) {
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

