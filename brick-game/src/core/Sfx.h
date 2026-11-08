#pragma once
#include <stdint.h>

// Sound effects, ordered by priority: if a game raises several in one update, the highest wins.
enum class Sfx : uint8_t { None, Move, Rotate, Drop, Line, Tetris, LevelUp, Start, GameOver };
