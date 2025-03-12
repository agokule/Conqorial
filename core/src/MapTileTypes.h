#ifndef MAPTILETYPES_H
#define MAPTILETYPES_H

#include <cstdint>

// the values of each enum is
// the minimum elevation of the tile multiplied by 100
enum class MapTileType : uint8_t {
    Water = 0,
    Beach = 50,
    Grass = 55,
    Hill = 70,
    Mountain = 80,
};

template<typename T>
inline bool operator<(T lhs, MapTileType type) {
    return lhs < static_cast<uint8_t>(type);
}

template<typename T>
inline bool operator>=(T lhs, MapTileType type) {
    return lhs >= static_cast<uint8_t>(type);
}

#endif // MAPTILETYPES_H
