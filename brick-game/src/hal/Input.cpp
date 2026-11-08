#include "Input.h"
#include <Arduino.h>
#include <Wire.h>

static constexpr int kBootPin = 0;

bool Input::begin() {
    Wire.beginTransmission(kAddr);
    expander_ = Wire.endTransmission() == 0;
    if (expander_) {
        // Writing 1s makes every quasi-bidirectional pin a weak-pull-up input.
        Wire.beginTransmission(kAddr);
        Wire.write(0xFF);
        Wire.endTransmission();
    } else {
        pinMode(kBootPin, INPUT_PULLUP);
    }
    return expander_;
}

uint8_t Input::readRaw() {
    if (!expander_) return digitalRead(kBootPin) == LOW ? buttonBit(Button::A) : 0;

    if (Wire.requestFrom(kAddr, uint8_t(1)) == 1) return uint8_t(~Wire.read());
    ++errors_;
    return stable_;   // keep the last known state on a bus error
}

uint8_t Input::poll(uint32_t nowMs, Event* out, uint8_t max) {
    if (nowMs - lastPoll_ < kPollMs) return 0;
    lastPoll_ = nowMs;

    const uint8_t raw = readRaw();
    const uint8_t next = raw == lastRaw_ ? raw : stable_;   // accept only after 2 equal reads
    lastRaw_ = raw;

    uint8_t n = 0;
    auto emit = [&](uint8_t i, ButtonEvent t) {
        if (n < max) out[n++] = Event{Button(i), t};
    };

    const uint8_t changed = uint8_t(next ^ stable_);
    for (uint8_t i = 0; i < kButtonCount; ++i) {
        const uint8_t bit = uint8_t(1u << i);
        if (changed & bit) {
            if (next & bit) {
                emit(i, ButtonEvent::Press);
                nextRepeat_[i] = nowMs + kRepeatDelayMs;
            } else {
                emit(i, ButtonEvent::Release);
            }
        } else if ((next & bit) && (kRepeatMask & bit) && int32_t(nowMs - nextRepeat_[i]) >= 0) {
            emit(i, ButtonEvent::Repeat);
            nextRepeat_[i] = nowMs + kRepeatRateMs;
        }
    }
    stable_ = next;
    return n;
}
