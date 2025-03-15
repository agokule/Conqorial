#ifndef ATTACK_H
#define ATTACK_H

#include "Country.h"
#include "typedefs.h"
#include <map>
#include <optional>
#include <utility>
#include <set>

struct Attack {
    CountryId attacker;
    CountryId defender;
    std::pair<unsigned, unsigned> initial_pos;
    unsigned troops_to_attack;

    Attack(Country &attacker, std::optional<Country> &defender, std::pair<unsigned, unsigned> initial_pos, unsigned troops_to_attack) :
        attacker {attacker.get_id()},
        defender {static_cast<CountryId>(defender.has_value() ? defender->get_id() : 0)},
        initial_pos {initial_pos},
        troops_to_attack {troops_to_attack}
    {}

    std::set<std::pair<unsigned, unsigned>> advance(Map &map, std::map<CountryId, Country> &countries);
};

#endif
