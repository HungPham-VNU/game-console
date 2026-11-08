#pragma once
#include <stdint.h>
#include "Grid.h"

// Everything the renderer needs for one screen. Built by App each loop; the renderer
// diffs it against the previous Frame and redraws only what changed.
struct Hud {
    uint32_t hiScore = 0;
    uint32_t score = 0;
    uint8_t speed = 0;
    uint8_t level = 0;
};

struct Frame {
    Field field;
    Preview next;
    Hud hud;
    bool soundOn = true;
    bool paused = false;
    const char* status = "";   // short line under the field ("PAUSE", "GAME OVER", ...)
};
