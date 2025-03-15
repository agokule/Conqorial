#ifndef ATTACK_H
#define ATTACK_H

#include "Country.h"
#include "typedefs.h"
#include <optional>
#include <utility>
#include <set>

struct Attack {
    Country attacker;
    std::optional<Country> defender;
    std::pair<unsigned, unsigned> initial_pos;
    unsigned troops_to_attack;
    CountryId defender_id;

    Attack(Country &attacker, std::optional<Country> &defender, std::pair<unsigned, unsigned> initial_pos, unsigned troops_to_attack) :
        attacker {attacker},
        defender {defender},
        initial_pos {initial_pos},
        troops_to_attack {troops_to_attack},
        defender_id {static_cast<CountryId>(defender.has_value() ? defender->get_id() : 0)}
    {}

    std::set<std::pair<unsigned, unsigned>> advance(Map &map);
};

#endif
