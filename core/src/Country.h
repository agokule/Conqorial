#ifndef COUNTRY_H
#define COUNTRY_H

#include "Map.h"
#include "color.h"
#include "PopulationPyramid.h"
#include "typedefs.h"
#include <cstdint>
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
    // this is a percentage of the reproductive age
    // group (of the pyramid) the player/ai wants to mobilize
    uint8_t target_mobilization_level = 2;

    unsigned urbanization_level = 1;
    PopulationPyramid pyramid;
    unsigned money = 0;
    unsigned last_economy = 0;
    unsigned last_density = 0;
public:
    Country(CountryId id, std::string name, bool is_human, Color color) : id {id}, name {name}, is_human {is_human}, color {color} {}
    Country() : Country(0, "", false, {0, 0, 0}) { std::cerr << "Warning: Created default country\n"; }

    bool can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const;

    unsigned get_troops() const;
    // uses the target_mobilization_level to calculate troops
    void calculate_troops();

    std::string get_name() const;
    bool get_is_human() const;
    Color get_color() const;
    CountryId get_id() const;
    const PopulationPyramid &get_pyramid() const;
    
    // These are for the visualizer
    unsigned get_economy() const;
    unsigned get_density() const;
    void set_economy(unsigned economy);
    void set_density(unsigned density);

    unsigned get_urbanization_level() const;
    unsigned upgrade_urbanization_level();

    uint8_t get_target_mobilization_level() const;
    void set_target_mobilization_level(uint8_t level);

    unsigned get_money() const;
    void add_money(unsigned amount);
    void remove_money(unsigned amount);
};

#endif
