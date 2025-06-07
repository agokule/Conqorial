#ifndef COUNTRY_H
#define COUNTRY_H

#include "Map.h"
#include "color.h"
#include "PopulationPyramid.h"
#include "Logging.h"
#include <iostream>
#include <string>

class Country {
    friend struct Attack;
    friend class PopulationPyramidRenderer;
    friend class Match;

    CountryId id;
    std::string name;

    // false if it is an AI
    bool is_human;
    Color color;

    unsigned troops = 0;
    unsigned urbanization_level = 1;
    PopulationPyramid pyramid;
    unsigned money = 0;
public:
    Country(CountryId id, std::string name, bool is_human, Color color) : id {id}, name {name}, is_human {is_human}, color {color} {}
    Country() : Country(0, "", false, {0, 0, 0, 0}) { std::cerr << "Warning: Created default country\n"; }

    bool can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const;

    unsigned get_troops() const { return troops; }
    CountryId get_id() const { return id; }
    std::string get_name() const { return name; }
    bool get_is_human() const { return is_human; }
    Color get_color() const { return color; }
    const PopulationPyramid &get_pyramid() const { return pyramid; }

    unsigned get_urbanization_level() const { return urbanization_level; }
    unsigned upgrade_urbanization_level() { return ++urbanization_level; }

    unsigned get_money() const { return money; }
    void add_money(unsigned amount) { money += amount; }
    void remove_money(unsigned amount) {
        CONQORIAL_ASSERT_ALL(amount <= money, "Tried to remove more money than owned", return;);
        money -= amount;
    }
};

#endif
