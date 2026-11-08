#pragma once
#include <LovyanGFX.hpp>
#include "../core/Frame.h"

// Draws a Frame with the classic segment-LCD look, portrait 240×320:
//
//   ┌──────────┐ HI-SCORE
//   │ 10×20    │ SCORE
//   │ field,   │ NEXT [4×4]
//   │ 14 px    │ SPEED
//   │ cells    │ LEVEL
//   └──────────┘ ♪  ▮▮
//     status line
//
// Unlit cells are drawn in the "ghost" colour, like a real LCD. After the first full
// draw only changed cells, numbers and icons are redrawn.
class LcdRenderer {
public:
    static constexpr int kCell = 14;
    static constexpr int kFieldX = 5;
    static constexpr int kFieldY = 14;
    static constexpr int kPanelX = 152;
    static constexpr int kPanelRight = 237;

    explicit LcdRenderer(lgfx::LovyanGFX& gfx) : g_(gfx) {}

    void begin();                  // sets colours, full redraw on the next present()
    void present(const Frame& f);
    void invalidate() { full_ = true; }

private:
    void drawStatic();
    void drawCell(int px, int py, bool on);
    void drawNumber(int y, uint32_t value, int minDigits);
    void drawSoundIcon(bool on);
    void drawPauseIcon(bool on);
    void drawStatus(const char* s);

    lgfx::LovyanGFX& g_;
    uint16_t bg_ = 0, lit_ = 0, ghost_ = 0;
    bool full_ = true;
    Frame prev_;
    char prevStatus_[24] = {};
};
