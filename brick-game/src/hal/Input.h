#pragma once
#include <stdint.h>
#include "../core/Buttons.h"

// 8 buttons on a PCF8574 @ 0x20 (P0..P7 = Button enum order, active low).
// Wire must already be started on SDA 16 / SCL 15 (the bus is shared with the ES8311 and touch).
// Fallback: with no PCF8574 on the bus, the BOOT button (IO0) acts as A so the bare board
// can still be tested (A also starts a game from the menu). Touch buttons come in via poll().
class Input {
public:
    static constexpr uint8_t kAddr = 0x20;
    static constexpr uint32_t kPollMs = 5;          // debounce = 2 equal polls ≈ 5–10 ms
    static constexpr uint32_t kRepeatDelayMs = 170;
    static constexpr uint32_t kRepeatRateMs = 50;
    static constexpr uint8_t kRepeatMask =
        buttonBit(Button::Left) | buttonBit(Button::Right) | buttonBit(Button::Down);

    struct Event {
        Button button;
        ButtonEvent type;
    };

    bool begin();   // true if the PCF8574 answered
    bool hasExpander() const { return expander_; }

    // Returns the number of events written to out (at most max).
    // extra: buttons held by another source (on-screen touch buttons), OR-ed in before
    // debouncing so every source gets the same debounce and auto-repeat.
    uint8_t poll(uint32_t nowMs, uint8_t extra, Event* out, uint8_t max);

    uint8_t held() const { return stable_; }   // bit per Button
    uint32_t readErrors() const { return errors_; }

private:
    uint8_t readRaw();

    bool expander_ = false;
    uint8_t lastRaw_ = 0;
    uint8_t stable_ = 0;
    uint32_t lastPoll_ = 0;
    uint32_t errors_ = 0;
    uint32_t nextRepeat_[kButtonCount] = {};
};
