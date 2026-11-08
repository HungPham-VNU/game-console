#pragma once
#include <stdint.h>
#include <string.h>

// A W×H bit grid, one uint16_t per row (bit x = column x). (0,0) is the top-left cell.
// Games write only into Grids; the renderer is the only code that touches the display.
template <int W, int H>
struct Grid {
    static_assert(W > 0 && W <= 16, "rows are uint16_t");
    static constexpr int kWidth = W;
    static constexpr int kHeight = H;
    static constexpr uint16_t kFullRow = uint16_t((1u << W) - 1u);

    uint16_t rows[H] = {};

    static constexpr bool inside(int x, int y) { return x >= 0 && x < W && y >= 0 && y < H; }

    bool get(int x, int y) const { return inside(x, y) && ((rows[y] >> x) & 1u); }

    void set(int x, int y, bool on = true) {
        if (!inside(x, y)) return;
        if (on) rows[y] |= uint16_t(1u << x);
        else    rows[y] &= uint16_t(~(1u << x));
    }

    void clear() { memset(rows, 0, sizeof(rows)); }
    void fill() { for (auto& r : rows) r = kFullRow; }
    bool rowFull(int y) const { return rows[y] == kFullRow; }

    bool operator==(const Grid& o) const { return memcmp(rows, o.rows, sizeof(rows)) == 0; }
    bool operator!=(const Grid& o) const { return !(*this == o); }
};

using Field = Grid<10, 20>;   // the playfield
using Preview = Grid<4, 4>;   // the NEXT box

// 4×4 shape mask: bit 15 = row 0 col 0, bit 0 = row 3 col 3 (same layout as Tetris pieces).
inline bool maskCell(uint16_t mask, int r, int c) { return (mask & (0x8000u >> (r * 4 + c))) != 0; }

inline void maskToPreview(uint16_t mask, Preview& p) {
    p.clear();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (maskCell(mask, r, c)) p.set(c, r);
}
