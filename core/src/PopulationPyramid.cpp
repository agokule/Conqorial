#include "PopulationPyramid.h"
#include "Logging.h"
#include "typedefs.h"
#include <algorithm>
#include <map>
#include "cq_utils.h"

double sigmoid(double x) {
    return 1 / (1 + exp(-x));
}


const unsigned money_producing_age_min {20};
const unsigned money_producing_age_max {60};


double PopulationPyramid::calculate_birth_rate(double density, unsigned max_density, unsigned economic_score) const {
    return std::max((-((double)density * 5 / max_density) + (economic_score * 0.05) + 1), 0.1);
}

void PopulationPyramid::tick(unsigned economy, unsigned density, unsigned urbanization) {
    months_passed++;

    // move part of each age group up one piece
    // in other words, age the population
    unsigned to_move_male {};
    unsigned to_move_female {};
    constexpr double percentage_to_move {((double)1 / 60)};
    for (auto &piece : pieces) {
        piece.male_count += to_move_male;
        piece.female_count += to_move_female;
        to_move_male = piece.male_count * percentage_to_move;
        to_move_female = piece.female_count * percentage_to_move;
        piece.male_count -= to_move_male;
        piece.female_count -= to_move_female;
    }

    // babies born
    unsigned long reproductive_age_women {}, reproductive_age_men {};
    for (const auto &piece : pieces) {
        if (piece.age <= 55 && piece.age >= 20) {
            reproductive_age_women += piece.female_count;
            reproductive_age_men += piece.male_count;
        }
        if (piece.age > 55)
            break;
    }

    unsigned long reproductive_people {std::min(reproductive_age_men, reproductive_age_women)};
    unsigned max_density {10'000}; // TODO: make this depend on the country's tiles
    max_density *= (urbanization * 2);
    auto birth_rate {calculate_birth_rate(density, max_density, economy)};
    double babies_in_lifetime {reproductive_people * birth_rate};
    unsigned babies_this_month {static_cast<unsigned>(babies_in_lifetime / 12 / 75)};
    pieces[0].male_count += babies_this_month + (babies_this_month / 50); // men are more likely to be born
    pieces[0].female_count += babies_this_month;

    CQ_LOG_DEBUG << "Babies this month: " << babies_this_month << '\n';
    CQ_LOG_DEBUG << "Birth rate: " << birth_rate << '\n';

    double life_expectancy {75 - ((double)density / max_density) + (economy * 0.1)};
    CQ_LOG_DEBUG << "Life expectancy: " << life_expectancy << '\n';

    // kill some people
    for (auto &piece : pieces) {
        unsigned long to_kill_male {};
        unsigned long to_kill_female {};
        double percentage_to_kill {piece.get_death_rate(life_expectancy)};
        to_kill_male = piece.male_count * percentage_to_kill;
        to_kill_female = piece.female_count * percentage_to_kill;
        piece.male_count -= to_kill_male + (to_kill_male / 50); // men are more likely to die
        piece.female_count -= to_kill_female;
    }

    update_total_population();
}

namespace PyramidUtils {

EconomyResult get_economy_score(const PopulationPyramid &pyramid, CountryId country) {
    static std::map<CountryId, ScrollingHistory<6>> money_made_history {};

    unsigned long money_producing_people {}, money_unproducing_people {};

    for (const auto &piece : pyramid.getPieces()) {
        if (piece.age >= money_producing_age_min && piece.age < money_producing_age_max) {
            money_producing_people += piece.male_count + piece.female_count;
            continue;
        }
        money_unproducing_people += piece.male_count + piece.female_count;
    }

    double money_made {
        money_producing_people * 0.25 - money_unproducing_people * 0.1
    };

    auto previous_money_made_6_months_ago = money_made_history[country].push(money_made);

    double difference = money_made - previous_money_made_6_months_ago;

    return {
        static_cast<unsigned int>((tanh((difference) / 100.0) + 1) * 100),
        static_cast<int>(money_made)
    };
}

} // namespace PyramidUtils

