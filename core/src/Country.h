#ifndef COUNTRY_H
#define COUNTRY_H

#include "Map.h"
#include "RandomGenerator.h"
#include "color.h"
#include "PopulationPyramid.h"
#include "typedefs.h"
#include <cstdint>
#include <string>
#include <optional>

constexpr unsigned short ai_check_attack_interval_minCE = 5'000;
constexpr unsigned short ai_check_attack_interval_maxCE = 20'000;

constexpr uint8_t ai_mobilization_level_minCE = 5;
constexpr uint8_t ai_mobilization_level_maxCE = 20;

constexpr uint8_t ai_reserve_troops_minCE = 30;
constexpr uint8_t ai_reserve_troops_maxCE = 60;

struct AIPlayerBehavior {
    // how often the AI will check if it can attack a neighbor
    // in milliseconds
    unsigned short check_decision_interval;
    // millitary recruitment target percentage
    uint8_t target_mobilization_level;
    // how troops the AI will ALWAYS keep in reserve as a percentage
    uint8_t reserve_troops;

    CQIntervalTimePoint last_descision_check;

    AIPlayerBehavior(RandomGenerator &random);
    void update_last_attack_check();
};

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
    unsigned millitary_level = 1;
    PopulationPyramid pyramid;
    unsigned money = 0;
    unsigned last_economy = 0;
    unsigned last_density = 0;

    // if .has_value() returns true then it is an AI player
    std::optional<AIPlayerBehavior> ai_behavior;
public:
    // if the random generator is passed in then the country will be an AI
    // otherwise it will be a player
    Country(CountryId id, std::string name, Color color, RandomGenerator *random = nullptr);

    bool can_attack(CountryId other_id, std::pair<unsigned, unsigned> pos, const Map &map) const;

    unsigned get_troops() const;
    // uses the target_mobilization_level to calculate troops
    void calculate_troops();
    unsigned long get_military_score() const;

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

    unsigned calculate_millitary_score() const;

    unsigned get_urbanization_level() const;
    unsigned upgrade_urbanization_level();
    unsigned get_millitary_level() const;
    unsigned upgrade_millitary_level();

    uint8_t get_target_mobilization_level() const;
    void set_target_mobilization_level(uint8_t level);

    unsigned get_money() const;
    void add_money(unsigned amount);
    void remove_money(unsigned amount);
};

#endif
