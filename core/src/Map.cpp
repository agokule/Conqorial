#include "Map.h"

Map::Map(unsigned width, unsigned height) : width(width), height(height), tiles(width * height), noise() {
    noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin);

    unsigned index {};

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {

        }
    }
}

