#ifndef COUNTRY_H
#define COUNTRY_H

#include "color.h"
#include <cstdint>
#include <string>

// CountryId is an alias for uint8_t.
// If it is 0, it means no player/ai
typedef uint8_t CountryId;

class Country {
    CountryId id;
    std::string name;

    // false if it is an AI
    bool is_human;
    Color color;

    unsigned troops = 0;
public:
    Country(CountryId id, std::string name, bool is_human, Color color) : id {id}, name {name}, is_human {is_human}, color {color} {}
};

#endif
