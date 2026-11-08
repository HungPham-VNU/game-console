#pragma once
#include <stdint.h>

// base × factor^steps (factor as a percentage), clamped to minMs. Integer only.
inline uint32_t decayInterval(uint32_t baseMs, uint8_t steps, uint32_t percent, uint32_t minMs) {
    uint32_t ms = baseMs;
    for (uint8_t i = 0; i < steps && ms > minMs; ++i) ms = ms * percent / 100;
    return ms < minMs ? minMs : ms;
}
