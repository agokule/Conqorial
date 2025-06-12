#pragma once

#include "typedefs.h"
#include <array>
#include <cmath>
#include <cstdint>

const unsigned money_producing_age_min {20};
const unsigned money_producing_age_max {60};
const unsigned reproductive_age_min {20};
const unsigned reproductive_age_max {55};

struct PyramidPiece {
    // age range is from age to age + 5
    // unless it is 100+, then it is 100+
    uint8_t age;
    unsigned long male_count;
    unsigned long female_count;

    // returns a percentage of the population that will die
    // in one month
    double get_death_rate(double life_expectancy) const;
};

class PopulationPyramid {
    std::array<PyramidPiece, 20> pieces;
    unsigned total_population;
    unsigned months_passed;

public:
    PopulationPyramid();
    unsigned get_total_population() const;

    // moves the pyramid forward one month
    void tick(unsigned economy, unsigned density, unsigned urbanization);

    double calculate_birth_rate(double density, unsigned max_density, unsigned economic_score) const;

    const std::array<PyramidPiece, 20> &get_pieces() const;

    void update_total_population();
};

namespace PyramidUtils {

struct EconomyResult {
    unsigned score;
    int money_made;
};

EconomyResult get_economy_score(const PopulationPyramid &pyramid, CountryId country, uint8_t target_mobilization_level);

}

