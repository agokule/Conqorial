#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <cstdint>

enum class GameState: uint8_t {
    // Before the game starts, the player selects a starting point.
    // And we wait for other players to join as well.
    SelectingStartingPoint,
    // The game is in progress. Countries can attack each other.
    InGame,
    GameOver
};

#endif

