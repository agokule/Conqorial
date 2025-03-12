#include "noise_wrapper.h"

double get_raw_noise(const FastNoiseLite &noise, double x, double y) {
    return (noise.GetNoise(x, y) + 1) / 2;
}

double get_elevation(const FastNoiseLite& noise, double x, double y) {
    return get_raw_noise(noise, x, y);
}

