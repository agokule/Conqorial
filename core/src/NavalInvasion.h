#pragma once

#include "typedefs.h"
#include "Country.h"
#include <set>
#include <vector>

struct NavalInvasion {
    TileCoor curr_x;
    TileCoor curr_y;

    TileCoor dest_x;
    TileCoor dest_y;

    unsigned remaining_troops;
    CountryId attacker;

    // the path to the destination backwards
    std::vector<std::pair<TileCoor, TileCoor>> path_to_destination;
    std::vector<std::pair<TileCoor, TileCoor>>::iterator curr_path_it;

    NavalInvasion(TileIndex destination, CountryId attacker, unsigned troops, const Map &map);

    std::set<std::pair<TileCoor, TileCoor>> advance(Map &map, std::map<CountryId, std::set<TileIndex>> &tiles_owned_by_country, std::map<CountryId, Country> &countries);
    bool is_done() const;
};

