#include "noise_wrapper.h"

constexpr double sum_of_apmlitudes() {
    double sum {};
    for (const auto &amp : amplitutes) {
        sum += amp;
    }
    return sum;
}

double get_raw_noise(const FastNoiseLite &noise, double x, double y) {
    return (noise.GetNoise(x, y) + 1) / 2;
}

double get_elevation(const FastNoiseLite& noise, double x, double y) {
    double raw_noise {};
    for (const auto &amp : amplitutes) {
        double inverse_amp = 1 / amp;
        raw_noise += get_raw_noise(noise, x * inverse_amp, y * inverse_amp) * amp;
    }
    raw_noise /= sum_of_apmlitudes();
    return raw_noise;
}

