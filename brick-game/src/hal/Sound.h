#pragma once
#include "../core/Sfx.h"

// Stub until M4 (ES8311 codec over I2C + I2S, amp enable IO1 active low).
// For now it keeps the amplifier OFF and only logs the effect over serial.
class Sound {
public:
    static constexpr int kAmpEnablePin = 1;   // LOW = amplifier on
    void begin();
    void play(Sfx s);
};
