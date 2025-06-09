#ifndef ATTACK_H
#define ATTACK_H

#include "Country.h"
#include "typedefs.h"
#include <set>
#include <map>

struct Attack {
    CountryId attacker;
    CountryId defender;
    unsigned troops_to_attack;
    std::set<std::pair<TileCoor, TileCoor>> current_boder;

    Attack(CountryId attacker, CountryId defender, unsigned troops_to_attack) :
        attacker {attacker},
        defender {defender},
        troops_to_attack {troops_to_attack},
        current_boder {}
    {}

    std::set<std::pair<TileCoor, TileCoor>> advance(Map &map, std::map<CountryId, Country> &countries, std::map<CountryId, std::set<TileIndex>> &tiles_owned_by_country);
};

#endif
