#pragma once
#include <LovyanGFX.hpp>
#include "../core/Frame.h"
#include "Layout.h"

// Draws a Frame with the classic segment-LCD look (see Layout.h for the two layouts).
// Unlit cells are drawn in the "ghost" colour, like a real LCD. After the first full draw
// only changed cells, numbers, icons and on-screen buttons are redrawn.
class LcdRenderer {
public:
    explicit LcdRenderer(lgfx::LovyanGFX& gfx) : g_(gfx) {}

    void begin(const Layout& layout);   // sets colours + layout, full redraw on next present()
    // touchHeld: bit per Button currently pressed on screen (highlights the on-screen buttons).
    void present(const Frame& f, uint8_t touchHeld = 0);
    void invalidate() { full_ = true; }
    const Layout& layout() const { return L_; }

private:
    void drawStatic();
    void drawCell(int px, int py, bool on);
    void drawNumber(int x, int right, int labelY, uint32_t value);
    void drawSoundIcon(bool on);
    void drawPauseIcon(bool on);
    void drawStatus(const char* s);
    void drawTouchButton(const TouchButton& tb, bool pressed);

    lgfx::LovyanGFX& g_;
    Layout L_ = kClassicLayout;
    uint16_t bg_ = 0, lit_ = 0, ghost_ = 0;
    bool full_ = true;
    Frame prev_;
    uint8_t prevTouch_ = 0;
    char prevStatus_[24] = {};
};
