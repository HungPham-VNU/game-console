#include "Touch.h"
#include <Arduino.h>
#include <Wire.h>
#include "../render/Layout.h"

namespace {
constexpr uint8_t kRegDevMode = 0x00;
constexpr uint8_t kRegTdStatus = 0x02;   // then P1: XH XL YH YL WEIGHT MISC, P2: XH XL YH YL
constexpr uint8_t kReadLen = 11;         // 0x02..0x0C
} // namespace

bool Touch::begin() {
    pinMode(kRstPin, OUTPUT);
    digitalWrite(kRstPin, LOW);
    delay(5);
    digitalWrite(kRstPin, HIGH);
    delay(300);   // FT6x36 needs ~300 ms after reset before it answers

    Wire.beginTransmission(kAddr);
    present_ = Wire.endTransmission() == 0;
    if (present_) {
        Wire.beginTransmission(kAddr);
        Wire.write(kRegDevMode);
        Wire.write(0x00);   // normal operating mode
        Wire.endTransmission();
    }
    return present_;
}

void Touch::mapPoint(int rx, int ry, int& sx, int& sy) const {
    int x = rx, y = ry;
#if TOUCH_SWAP_XY
    const int t = x; x = y; y = t;
#endif
#if TOUCH_FLIP_X
    x = kWidth - 1 - x;
#endif
#if TOUCH_FLIP_Y
    y = kHeight - 1 - y;
#endif
    sx = x;
    sy = y;
}

uint8_t Touch::hitTest(int x, int y) { return touchHit(x, y); }

uint8_t Touch::poll(uint32_t nowMs) {
    if (!present_ || nowMs - lastPoll_ < kPollMs) return held_;
    lastPoll_ = nowMs;

    uint8_t d[kReadLen];
    Wire.beginTransmission(kAddr);
    Wire.write(kRegTdStatus);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(kAddr, kReadLen) != kReadLen) {
        ++errors_;
#ifdef TOUCH_DEBUG
        if (errors_ % 50 == 1) Serial.printf("[tdbg] read error #%lu\n", (unsigned long)errors_);
#endif
        while (Wire.available()) Wire.read();
        return held_;
    }
    for (uint8_t i = 0; i < kReadLen; ++i) d[i] = uint8_t(Wire.read());

#ifdef TOUCH_DEBUG
    static uint32_t lastDbg = 0;
    if (nowMs - lastDbg >= 500) {
        lastDbg = nowMs;
        Serial.printf("[tdbg] err=%lu int=%d regs:", (unsigned long)errors_, digitalRead(17));
        for (uint8_t i = 0; i < kReadLen; ++i) Serial.printf(" %02X", d[i]);
        Serial.println();
    }
#endif

    uint8_t count = d[0] & 0x0F;
    if (count > 2) count = 0;   // 0x0F etc. = invalid / no touch

    uint8_t mask = 0;
    for (uint8_t i = 0; i < count; ++i) {
        const uint8_t* p = &d[1 + i * 6];
        const uint8_t event = p[0] >> 6;   // 0 down, 1 up, 2 contact, 3 none
        if (event == 1 || event == 3) continue;
        const int rx = ((p[0] & 0x0F) << 8) | p[1];
        const int ry = ((p[2] & 0x0F) << 8) | p[3];
        int sx, sy;
        mapPoint(rx, ry, sx, sy);
        const uint8_t hit = hitTest(sx, sy);
        mask |= hit;
#if TOUCH_LOG
        if (count > prevCount_)
            Serial.printf("[touch] #%u raw=(%d,%d) screen=(%d,%d) button=0x%02X\n", i, rx, ry, sx, sy, hit);
#endif
    }
    prevCount_ = count;
    held_ = mask;
    return held_;
}
