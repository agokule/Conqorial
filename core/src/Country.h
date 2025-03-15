#ifndef COUNTRY_H
#define COUNTRY_H

#include "Map.h"
#include "color.h"
#include <optional>
#include <string>

class Country {
    friend struct Attack;

    CountryId id;
    std::string name;

    // false if it is an AI
    bool is_human;
    Color color;

    unsigned troops = 0;
public:
    Country(CountryId id, std::string name, bool is_human, Color color) : id {id}, name {name}, is_human {is_human}, color {color} {}

    bool can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const;

    unsigned get_troops() const { return troops; }
    CountryId get_id() const { return id; }
    std::string get_name() const { return name; }
    bool get_is_human() const { return is_human; }
    Color get_color() const { return color; }
};

#endif
