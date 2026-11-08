#pragma once
#include <stdint.h>
#include "Sfx.h"

// What App needs from the outside world. ESP32 implementation lives in main.cpp;
// host tests provide a fake.
class Platform {
public:
    virtual ~Platform() = default;
    virtual uint32_t loadHiScore(const char* key) = 0;
    virtual void saveHiScore(const char* key, uint32_t value) = 0;
    virtual void play(Sfx s) = 0;
    virtual uint32_t randomSeed() = 0;
};
