#include "noise_wrapper.h"

double stretch_sigmoid(double x, double factor) {
    return x / (1.0 - factor * (1.0 - std::abs(x)));
}

double get_raw_noise(const FastNoiseLite &noise, double x, double y) {
    return (stretch_sigmoid(noise.GetNoise(x, y), 0.6) + 1) / 2;
}

double get_elevation(const FastNoiseLite& noise, double x, double y) {
    return get_raw_noise(noise, x, y);
}

