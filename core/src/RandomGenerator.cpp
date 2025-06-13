#include "RandomGenerator.h"

RandomGenerator::RandomGenerator(unsigned int s) : seed(s), count(0) {
    generator.seed(seed);
}

RandomGenerator::RandomGenerator() : count(0) {
    std::random_device rd;
    seed = rd();
    generator.seed(seed);
}

int RandomGenerator::randint(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    count++;
    return dist(generator);
}

double RandomGenerator::rand_double(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    count++;
    return dist(generator);
}

double RandomGenerator::rand_double() {
    return rand_double(0.0, 1.0);
}

unsigned int RandomGenerator::get_seed() const {
    return seed;
}

int RandomGenerator::get_count() const {
    return count;
}

void RandomGenerator::reset(unsigned int newSeed) {
    seed = newSeed;
    generator.seed(seed);
    count = 0;
}

void RandomGenerator::reset_count() {
    count = 0;
}

