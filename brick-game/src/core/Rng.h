#pragma once
#include <stdint.h>

// xorshift32: small, fast, deterministic for a given seed (useful in host tests).
class Rng {
public:
    explicit Rng(uint32_t seed = 1) { reseed(seed); }
    void reseed(uint32_t s) { state_ = s ? s : 0x9E3779B9u; }
    uint32_t next() {
        uint32_t x = state_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state_ = x;
    }
    // Uniform-ish in [0, n). n must be > 0.
    uint32_t below(uint32_t n) { return next() % n; }

private:
    uint32_t state_;
};
