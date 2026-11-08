#pragma once
#include <stdint.h>

// FT6336G capacitive touch (ES3C28P only) @ 0x38 on the shared I2C bus (SDA 16 / SCL 15),
// RST IO18, INT IO17 (unused: we poll). Read directly over Wire, because LovyanGFX's own
// I2C driver would fight Arduino Wire for the same bus.
//
// Up to two fingers are tracked, so e.g. LEFT and A can be held together.
// poll() returns a bit per Button for every on-screen button under a finger (Layout.h).
//
// Touch → screen mapping. Measured on the real ES3C28P (2026-11-08): with the display at
// LCD_ROTATION 2 the FT6336G's raw coordinates already match screen coordinates, so no
// flip is needed. If you change LCD_ROTATION, flip X and Y (check the [touch] log).
#ifndef TOUCH_SWAP_XY
#define TOUCH_SWAP_XY 0
#endif
#ifndef TOUCH_FLIP_X
#define TOUCH_FLIP_X 0
#endif
#ifndef TOUCH_FLIP_Y
#define TOUCH_FLIP_Y 0
#endif
#ifndef TOUCH_LOG
#define TOUCH_LOG 1   // print each new touch (raw + screen coords) to serial
#endif

class Touch {
public:
    static constexpr uint8_t kAddr = 0x38;
    static constexpr int kRstPin = 18;
    static constexpr uint32_t kPollMs = 10;
    static constexpr int kWidth = 240, kHeight = 320;

    bool begin();                 // true if the controller answered
    bool present() const { return present_; }
    uint8_t poll(uint32_t nowMs); // returns held mask (also cached in held())
    uint8_t held() const { return held_; }
    uint32_t readErrors() const { return errors_; }

private:
    static uint8_t hitTest(int x, int y);
    void mapPoint(int rx, int ry, int& sx, int& sy) const;

    bool present_ = false;
    uint8_t held_ = 0;
    uint8_t prevCount_ = 0;
    uint32_t lastPoll_ = 0;
    uint32_t errors_ = 0;
};
