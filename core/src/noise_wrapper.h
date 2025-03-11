#pragma once

#include "typedefs.h"
#include <FastNoiseLite/FastNoiseLite.h>
#include <array>

constexpr std::array<double, 5> amplitutes {
    1, 0.75, 0.5, 0.25, 0.1
};

constexpr double sum_of_apmlitudes();

// takes the -1.0 to 1.0 noise from FastNoiseLite and returns 0.0 to 1.0
double get_raw_noise(const FastNoiseLite& noise, double x, double y);

// uses the raw_noise function and octaves to make a good looking map
Elevation get_elevation(const FastNoiseLite& noise, double x, double y);

