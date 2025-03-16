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
    unsigned troops_to_attack;
    std::vector<std::pair<unsigned, unsigned>> current_boder;

    Attack(Country &attacker, std::optional<Country> &defender, unsigned troops_to_attack) :
        attacker {attacker.get_id()},
        defender {static_cast<CountryId>(defender.has_value() ? defender->get_id() : 0)},
        troops_to_attack {troops_to_attack},
        current_boder {}
    {}

    std::set<std::pair<unsigned, unsigned>> advance(Map &map, std::map<CountryId, Country> &countries);
};

#endif
