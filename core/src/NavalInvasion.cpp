#include "NavalInvasion.h"
#include "Logging.h"
#include "typedefs.h"
// TODO: Implement this

NavalInvasion::NavalInvasion(TileIndex destination, CountryId attacker, unsigned troops, const Map &map)
    : remaining_troops {troops}, attacker {attacker} {
}

std::set<std::pair<TileCoor, TileCoor>> NavalInvasion::advance(Map &map, std::map<CountryId, std::set<TileIndex>> &tiles_owned_by_country, std::map<CountryId, Country> &countries) {
}

bool NavalInvasion::is_done() const {
    return this->curr_path_it == this->path_to_destination.end() || path_to_destination.empty();
}
