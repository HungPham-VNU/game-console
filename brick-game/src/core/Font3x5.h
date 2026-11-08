#pragma once
#include <stdint.h>
#include "Grid.h"

// 3×5 digits for drawing numbers inside the playfield (menu screen).
// 15 bits per glyph, row-major, bit 14 = top-left.
namespace font3x5 {

constexpr uint16_t kDigits[10] = {
    0b111'101'101'101'111, // 0
    0b010'110'010'010'111, // 1
    0b111'001'111'100'111, // 2
    0b111'001'111'001'111, // 3
    0b101'101'111'001'001, // 4
    0b111'100'111'001'111, // 5
    0b111'100'111'101'111, // 6
    0b111'001'001'001'001, // 7
    0b111'101'111'101'111, // 8
    0b111'101'111'001'111, // 9
};

// Draws digit d with its top-left at (x0, y0); each glyph pixel becomes scale×scale cells.
inline void drawDigit(Field& f, uint8_t d, int x0, int y0, int scale = 1) {
    const uint16_t g = kDigits[d % 10];
    for (int r = 0; r < 5; ++r)
        for (int c = 0; c < 3; ++c)
            if (g & (1u << (14 - (r * 3 + c))))
                for (int dy = 0; dy < scale; ++dy)
                    for (int dx = 0; dx < scale; ++dx)
                        f.set(x0 + c * scale + dx, y0 + r * scale + dy);
}

} // namespace font3x5
