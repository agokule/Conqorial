#pragma once

#include <random>

// The point of this class is that the multiplayer server can
// just send information on how create a RandomGenerator object
// to every client’s computer so that they can do things that
// require randomness like AI players with the same random numbers
class RandomGenerator {
private:
    std::mt19937 generator;
    unsigned int seed;
    int count;

public:
    // Constructor with seed
    RandomGenerator(unsigned int s);
    
    // Default constructor uses random device for seed
    RandomGenerator();
    
    // Generate random integer in range [min, max]
    int randint(int min, int max);
    
    // Generate random double in range [min, max)
    double rand_double(double min, double max);
    
    // Generate random double in range [0, 1)
    double rand_double();

    // return true or false randomly
    bool rand_bool();
    
    // Get the seed used
    unsigned int get_seed() const;
    
    // Get count of numbers generated
    int get_count() const;
    
    // Reset the generator with new seed
    void reset(unsigned int newSeed);
    
    // Reset count but keep same seed
    void reset_count();
};

