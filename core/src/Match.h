#pragma once

#include "Country.h"
#include "Logging.h"
#include "Map.h"
#include "Attack.h"
#include <map>
#include <vector>
#include <chrono>
#include "GameState.h"
#include "typedefs.h"

// CE stands for constexpr
constexpr std::chrono::milliseconds attack_update_intervalCE { 50 };
constexpr std::chrono::milliseconds population_update_intervalCE { 10'000 };
constexpr std::chrono::milliseconds ai_update_intervalCE { 500 };
constexpr std::chrono::milliseconds economy_update_intervalCE { 1000 };

class Match {
    GameState game_state = GameState::SelectingStartingPoint;
    std::map<CountryId, Country> countries;
    std::map<CountryId, std::set<TileIndex>> tiles_owned_by_country;
    Map map;
    std::map<CountryId, std::vector<CountryId>> alliances;
    std::map<CountryId, std::map<CountryId, Attack>> on_going_attacks;

    std::chrono::time_point<std::chrono::high_resolution_clock> last_population_update;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_attack_update;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_ai_update;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_economy_update;

    void update_populations();
    std::vector<std::pair<TileCoor, TileCoor>> update_attacks();
    void update_ai_decisions();
    void update_economies();
public:
    Match(unsigned width, unsigned height): countries {}, map {width, height} {
        countries[0] = { 0, "Neutral", false, {0, 0, 0, 0} };
        tiles_owned_by_country[0] = {};
    }

    const Country &get_country(CountryId id) const { return countries.at(id); }
    const Country &new_country(std::string name, bool is_player, Color color) {
        CountryId id = countries.size();
        tiles_owned_by_country[id] = {};
        return countries.insert({ id, Country { id, name, is_player, color } }).first->second;
    }
    void spawn_country(CountryId id, TileCoor x, TileCoor y);

    void set_game_started() { game_state = GameState::InGame; }
    GameState get_game_state() const { return game_state; }
    
    void new_alliance(CountryId id1, CountryId id2) {
        alliances[id1].push_back(id2);
        alliances[id2].push_back(id1);
    }

    void attack(CountryId attacker, CountryId defender_id, unsigned troops_to_attack, TileCoor tile_x, TileCoor tile_y);

    const Map &get_map() const { return map; }
    void set_map_tile(TileCoor x, TileCoor y, CountryId owner);
    void set_map_tile(std::pair<TileCoor, TileCoor> pos, CountryId owner) { set_map_tile(pos.first, pos.second, owner); }

    const std::map<CountryId, Country> &get_countries() const { return countries; }

    // updates the state of the game, should be called every frame or as often as possible
    // returns a list of tiles that have changed
    std::vector<std::pair<TileCoor, TileCoor>> tick();
};
