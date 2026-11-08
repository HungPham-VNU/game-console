#include "LcdRenderer.h"
#include <string.h>

namespace {
constexpr int kLabelH = 16;   // Font2
constexpr int kValueH = 26;   // Font4
} // namespace

void LcdRenderer::begin(const Layout& layout) {
    L_ = layout;
    bg_ = g_.color565(0x9E, 0xAD, 0x86);
    lit_ = g_.color565(0x10, 0x10, 0x10);
    ghost_ = g_.color565(0x8A, 0x98, 0x77);
    full_ = true;
}

// One LCD cell: 1 px outer ring, 1 px gap, filled centre.
void LcdRenderer::drawCell(int px, int py, bool on) {
    const uint16_t c = on ? lit_ : ghost_;
    const int s = L_.cell;
    g_.fillRect(px + 1, py + 1, s - 2, s - 2, c);
    g_.fillRect(px + 2, py + 2, s - 4, s - 4, bg_);
    g_.fillRect(px + 3, py + 3, s - 6, s - 6, c);
}

void LcdRenderer::drawStatic() {
    const int fw = Field::kWidth * L_.cell, fh = Field::kHeight * L_.cell;
    g_.fillScreen(bg_);
    g_.drawRect(L_.fieldX - 2, L_.fieldY - 2, fw + 4, fh + 4, lit_);

    g_.setFont(&fonts::Font2);
    g_.setTextColor(lit_, bg_);
    g_.setTextDatum(textdatum_t::top_left);
    g_.drawString("HI-SCORE", L_.panelX, L_.hiY);
    g_.drawString("SCORE", L_.panelX, L_.scoreY);
    g_.drawString("NEXT", L_.panelX, L_.nextLabelY);
    g_.drawString("SPEED", L_.speedX, L_.speedY);
    g_.drawString("LEVEL", L_.levelX, L_.levelY);
}

void LcdRenderer::drawNumber(int x, int right, int labelY, uint32_t value) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)value);
    const int y = labelY + kLabelH;
    g_.fillRect(x, y, right - x + 1, kValueH, bg_);
    g_.setFont(&fonts::Font4);
    g_.setTextColor(lit_, bg_);
    g_.setTextDatum(textdatum_t::top_right);
    g_.drawString(buf, right, y);
}

void LcdRenderer::drawSoundIcon(bool on) {
    // Small speaker with two sound waves.
    const int x = L_.soundX, y = L_.soundY;
    const uint16_t c = on ? lit_ : ghost_;
    g_.fillRect(x - 2, y - 2, 30, 20, bg_);
    g_.fillRect(x, y + 5, 5, 6, c);
    g_.fillTriangle(x + 4, y + 8, x + 11, y + 1, x + 11, y + 15, c);
    g_.drawArc(x + 11, y + 8, 7, 6, -50, 50, c);
    g_.drawArc(x + 11, y + 8, 12, 11, -50, 50, c);
}

void LcdRenderer::drawPauseIcon(bool on) {
    const int x = L_.pauseX, y = L_.pauseY;
    const uint16_t c = on ? lit_ : ghost_;
    g_.fillRect(x, y, 5, 16, c);
    g_.fillRect(x + 10, y, 5, 16, c);
}

void LcdRenderer::drawStatus(const char* s) {
    g_.fillRect(L_.statusX, L_.statusY, L_.statusW, kLabelH, bg_);
    g_.setFont(&fonts::Font2);
    g_.setTextColor(lit_, bg_);
    g_.setTextDatum(textdatum_t::top_center);
    g_.drawString(s, L_.statusCx, L_.statusY);
}

void LcdRenderer::drawTouchButton(const TouchButton& tb, bool pressed) {
    const uint16_t fg = pressed ? bg_ : lit_;   // glyph colour
    const uint16_t fill = pressed ? lit_ : bg_;
    const int x = tb.x, y = tb.y, w = tb.w, h = tb.h;
    const int cx = x + w / 2, cy = y + h / 2;

    switch (tb.shape) {
    case TouchButton::Arrow: {
        g_.fillRoundRect(x, y, w, h, 4, fill);
        g_.drawRoundRect(x, y, w, h, 4, lit_);
        const int a = 7;   // half-size of the arrow
        switch (tb.button) {
        case Button::Up:    g_.fillTriangle(cx, cy - a, cx - a, cy + a / 2, cx + a, cy + a / 2, fg); break;
        case Button::Down:  g_.fillTriangle(cx, cy + a, cx - a, cy - a / 2, cx + a, cy - a / 2, fg); break;
        case Button::Left:  g_.fillTriangle(cx - a, cy, cx + a / 2, cy - a, cx + a / 2, cy + a, fg); break;
        case Button::Right: g_.fillTriangle(cx + a, cy, cx - a / 2, cy - a, cx - a / 2, cy + a, fg); break;
        default: break;
        }
        break;
    }
    case TouchButton::Round: {
        const int r = w / 2;
        g_.fillCircle(cx, cy, r, fill);
        g_.drawCircle(cx, cy, r, lit_);
        g_.drawCircle(cx, cy, r - 1, lit_);
        g_.setFont(&fonts::Font4);
        g_.setTextColor(fg, fill);
        g_.setTextDatum(textdatum_t::middle_center);
        g_.drawString(tb.label, cx, cy + 1);
        break;
    }
    case TouchButton::Pill: {
        g_.fillRoundRect(x, y, w, h, h / 2, fill);
        g_.drawRoundRect(x, y, w, h, h / 2, lit_);
        g_.setFont(&fonts::Font0);
        g_.setTextColor(fg, fill);
        g_.setTextDatum(textdatum_t::middle_center);
        g_.drawString(tb.label, cx, cy);
        break;
    }
    }
}

void LcdRenderer::present(const Frame& f, uint8_t touchHeld) {
    const char* status = f.status ? f.status : "";
    g_.startWrite();

    const bool full = full_;
    if (full) drawStatic();

    const int s = L_.cell;
    for (int y = 0; y < Field::kHeight; ++y) {
        const uint16_t diff = full ? Field::kFullRow : uint16_t(f.field.rows[y] ^ prev_.field.rows[y]);
        if (!diff) continue;
        for (int x = 0; x < Field::kWidth; ++x)
            if (diff & (1u << x)) drawCell(L_.fieldX + x * s, L_.fieldY + y * s, f.field.get(x, y));
    }

    for (int y = 0; y < Preview::kHeight; ++y) {
        const uint16_t diff = full ? Preview::kFullRow : uint16_t(f.next.rows[y] ^ prev_.next.rows[y]);
        for (int x = 0; x < Preview::kWidth; ++x)
            if (diff & (1u << x)) drawCell(L_.nextGridX + x * s, L_.nextGridY + y * s, f.next.get(x, y));
    }

    if (full || f.hud.hiScore != prev_.hud.hiScore) drawNumber(L_.panelX, L_.panelRight, L_.hiY, f.hud.hiScore);
    if (full || f.hud.score != prev_.hud.score) drawNumber(L_.panelX, L_.panelRight, L_.scoreY, f.hud.score);
    if (full || f.hud.speed != prev_.hud.speed) drawNumber(L_.speedX, L_.speedRight, L_.speedY, f.hud.speed);
    if (full || f.hud.level != prev_.hud.level) drawNumber(L_.levelX, L_.levelRight, L_.levelY, f.hud.level);
    if (full || f.soundOn != prev_.soundOn) drawSoundIcon(f.soundOn);
    if (full || f.paused != prev_.paused) drawPauseIcon(f.paused);
    if (full || strncmp(status, prevStatus_, sizeof(prevStatus_)) != 0) {
        drawStatus(status);
        strncpy(prevStatus_, status, sizeof(prevStatus_) - 1);
    }

    if (L_.touchButtons) {
        const uint8_t changed = full ? 0xFF : uint8_t(touchHeld ^ prevTouch_);
        for (const auto& tb : kTouchButtons)
            if (changed & buttonBit(tb.button)) drawTouchButton(tb, touchHeld & buttonBit(tb.button));
        prevTouch_ = touchHeld;
    }

    g_.endWrite();
    prev_ = f;
    prev_.status = prevStatus_;
    full_ = false;
}
