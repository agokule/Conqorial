#pragma once

#include "typedefs.h"
#include <array>
#include <cmath>
#include <cstdint>

struct PyramidPiece {
    // age range is from age to age + 5
    // unless it is 100+, then it is 100+
    uint8_t age;
    unsigned long male_count;
    unsigned long female_count;

    // returns a percentage of the population that will die
    // in one month
    double get_death_rate(double life_expectancy) const {
        return (std::pow(tanh(age / life_expectancy) / 3, 2.5) + 0.001) / 3.5;
    }
};

class PopulationPyramid {
    std::array<PyramidPiece, 20> pieces;
    unsigned total_population;
    unsigned months_passed;

public:
    PopulationPyramid() : pieces {}, total_population {}, months_passed {} {
        unsigned pop = 1'000'000;
        unsigned age = 0;
        for (auto &piece : pieces) {
            piece.age = age;
            piece.male_count = pop;
            piece.female_count = pop;

            total_population += piece.male_count + piece.female_count;
            age += 5;
            pop /= 1.2;
        }
    }

    unsigned get_total_population() const { return total_population; }

    // moves the pyramid forward one month
    void tick(unsigned economy, unsigned density, unsigned urbanization);

    double calculate_birth_rate(double density, unsigned max_density, unsigned economic_score) const;

    // Add to PopulationPyramid.h public section:
    const std::array<PyramidPiece, 20> &getPieces() const { return pieces; }

    void update_total_population() {
        total_population = 0;
        for (const auto &piece : pieces) {
            total_population += piece.male_count + piece.female_count;
        }
    }
};

namespace PyramidUtils {

struct EconomyResult {
    unsigned score;
    int money_made;
};

EconomyResult get_economy_score(const PopulationPyramid &pyramid, CountryId country);

}

