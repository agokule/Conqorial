#ifndef PLAYERID_H
#define PLAYERID_H

#include <cstdint>
#include <chrono>

// should be between 0 and 1
typedef double Elevation;

// CountryId is an alias for uint8_t.
// If it is 0, it means no player/ai
typedef uint8_t CountryId;

typedef uint16_t TileCoor;
typedef uint32_t TileIndex;

typedef std::chrono::steady_clock::time_point CQIntervalTimePoint;
typedef std::chrono::system_clock::time_point CQSystemTimePoint;

#endif // PLAYERID_H
