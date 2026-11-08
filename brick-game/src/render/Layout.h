#pragma once
#include <stdint.h>
#include "../core/Buttons.h"

// Screen layouts (portrait 240×320, after rotation).
//
//  Classic (physical buttons)          Touch (on-screen buttons)
//  ┌────────┐ HI-SCORE                 ┌──────┐ HI-SCORE
//  │ 10×20  │ SCORE                    │10×20 │ SCORE
//  │ 14 px  │ NEXT                     │10 px │ NEXT  ♪ ▮▮
//  │        │ SPEED                    │      │ SPEED LEVEL
//  │        │ LEVEL                    └──────┘
//  └────────┘ ♪ ▮▮                       status
//   status                              ▲     SEL     (A)
//                                     ◄   ►  START (B)
//                                       ▼
struct Layout {
    int cell;
    int fieldX, fieldY;
    int panelX, panelRight;
    int hiY, scoreY;                 // label y; the value is drawn under it, right-aligned
    int nextLabelY, nextGridX, nextGridY;
    int speedX, speedY, speedRight;
    int levelX, levelY, levelRight;
    int soundX, soundY, pauseX, pauseY;
    int statusX, statusY, statusW, statusCx;
    bool touchButtons;
};

constexpr Layout kClassicLayout = {
    14,
    5, 14,
    152, 237,
    14, 62,
    110, 167, 128,
    152, 192, 237,
    152, 236, 237,
    156, 280, 219, 280,
    0, 302, 148, 75,
    false,
};

constexpr Layout kTouchLayout = {
    10,
    6, 6,
    114, 237,
    4, 48,
    94, 114, 110,
    114, 156, 172,
    180, 156, 237,
    168, 122, 220, 122,
    0, 210, 240, 120,
    true,
};

// On-screen buttons for the touch layout. Shared by the renderer (drawing) and
// hal/Touch (hit testing), so what you see is what you press.
struct TouchButton {
    Button button;
    int16_t x, y, w, h;
    enum Shape : uint8_t { Arrow, Round, Pill } shape;
    const char* label;
};

constexpr TouchButton kTouchButtons[] = {
    {Button::Up, 36, 228, 30, 28, TouchButton::Arrow, "U"},
    {Button::Left, 4, 258, 30, 30, TouchButton::Arrow, "L"},
    {Button::Right, 68, 258, 30, 30, TouchButton::Arrow, "R"},
    {Button::Down, 36, 290, 30, 28, TouchButton::Arrow, "D"},
    {Button::Select, 104, 238, 48, 22, TouchButton::Pill, "SELECT"},
    {Button::Start, 104, 272, 48, 22, TouchButton::Pill, "START"},
    {Button::B, 157, 270, 38, 38, TouchButton::Round, "B"},
    {Button::A, 198, 230, 38, 38, TouchButton::Round, "A"},
};
constexpr int kTouchButtonCount = sizeof(kTouchButtons) / sizeof(kTouchButtons[0]);
constexpr int kTouchHitPad = 5;   // fingers are fat: accept touches a little outside the outline

// The D-pad is one zone: the direction is whichever axis the finger is furthest along from
// the centre, so there are no dead gaps between the arrows (taps on the real board landed
// between DOWN and RIGHT, and in the empty centre). The ring within kDpadDead of the centre is neutral.
struct DpadZone {
    int x0, y0, x1, y1;   // zone, x1/y1 exclusive
    int cx, cy;           // centre
    int dead;
};
constexpr DpadZone kDpad = {0, 226, 99, 320, 51, 273, 5};

// Which button (bit mask) is under screen point (x, y)? 0 if none.
inline uint8_t touchHit(int x, int y) {
    if (x >= kDpad.x0 && x < kDpad.x1 && y >= kDpad.y0 && y < kDpad.y1) {
        const int dx = x - kDpad.cx, dy = y - kDpad.cy;
        const int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
        if (ax <= kDpad.dead && ay <= kDpad.dead) return 0;
        if (ax > ay) return buttonBit(dx < 0 ? Button::Left : Button::Right);
        return buttonBit(dy < 0 ? Button::Up : Button::Down);
    }
    for (const auto& tb : kTouchButtons) {
        if (tb.shape == TouchButton::Arrow) continue;   // handled by the zone above
        if (x >= tb.x - kTouchHitPad && x < tb.x + tb.w + kTouchHitPad &&
            y >= tb.y - kTouchHitPad && y < tb.y + tb.h + kTouchHitPad)
            return buttonBit(tb.button);
    }
    return 0;
}
