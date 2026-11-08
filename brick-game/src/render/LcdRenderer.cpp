#include "LcdRenderer.h"
#include <string.h>

namespace {

constexpr int kFieldW = Field::kWidth * LcdRenderer::kCell;    // 140
constexpr int kFieldH = Field::kHeight * LcdRenderer::kCell;   // 280

// Panel rows (y of each label; the value is drawn under it).
constexpr int kHiY = 14;
constexpr int kScoreY = 62;
constexpr int kNextY = 110;
constexpr int kNextGridY = kNextY + 18;
constexpr int kSpeedY = 192;
constexpr int kLevelY = 236;
constexpr int kIconY = 280;
constexpr int kStatusY = 302;

constexpr int kLabelH = 16;   // Font2
constexpr int kValueH = 26;   // Font4
constexpr int kNextX = LcdRenderer::kPanelX + (LcdRenderer::kPanelRight - LcdRenderer::kPanelX + 1 - 4 * LcdRenderer::kCell) / 2;

} // namespace

void LcdRenderer::begin() {
    bg_ = g_.color565(0x9E, 0xAD, 0x86);
    lit_ = g_.color565(0x10, 0x10, 0x10);
    ghost_ = g_.color565(0x8A, 0x98, 0x77);
    full_ = true;
}

// One LCD cell: 1 px outer ring, 1 px gap, 8×8 filled centre.
void LcdRenderer::drawCell(int px, int py, bool on) {
    const uint16_t c = on ? lit_ : ghost_;
    g_.fillRect(px + 1, py + 1, 12, 12, c);
    g_.fillRect(px + 2, py + 2, 10, 10, bg_);
    g_.fillRect(px + 3, py + 3, 8, 8, c);
}

void LcdRenderer::drawStatic() {
    g_.fillScreen(bg_);
    g_.drawRect(kFieldX - 2, kFieldY - 2, kFieldW + 4, kFieldH + 4, lit_);

    g_.setFont(&fonts::Font2);
    g_.setTextColor(lit_, bg_);
    g_.setTextDatum(textdatum_t::top_left);
    g_.drawString("HI-SCORE", kPanelX, kHiY);
    g_.drawString("SCORE", kPanelX, kScoreY);
    g_.drawString("NEXT", kPanelX, kNextY);
    g_.drawString("SPEED", kPanelX, kSpeedY);
    g_.drawString("LEVEL", kPanelX, kLevelY);
}

void LcdRenderer::drawNumber(int labelY, uint32_t value, int minDigits) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%0*lu", minDigits, (unsigned long)value);
    const int y = labelY + kLabelH;
    g_.fillRect(kPanelX, y, kPanelRight - kPanelX + 1, kValueH, bg_);
    g_.setFont(&fonts::Font4);
    g_.setTextColor(lit_, bg_);
    g_.setTextDatum(textdatum_t::top_right);
    g_.drawString(buf, kPanelRight, y);
}

void LcdRenderer::drawSoundIcon(bool on) {
    // Small speaker with two sound waves.
    const int x = kPanelX + 4, y = kIconY;
    const uint16_t c = on ? lit_ : ghost_;
    g_.fillRect(x - 2, y - 2, 30, 20, bg_);
    g_.fillRect(x, y + 5, 5, 6, c);
    g_.fillTriangle(x + 4, y + 8, x + 11, y + 1, x + 11, y + 15, c);
    g_.drawArc(x + 11, y + 8, 7, 6, -50, 50, c);
    g_.drawArc(x + 11, y + 8, 12, 11, -50, 50, c);
}

void LcdRenderer::drawPauseIcon(bool on) {
    const int x = kPanelRight - 18, y = kIconY;
    const uint16_t c = on ? lit_ : ghost_;
    g_.fillRect(x, y, 5, 16, c);
    g_.fillRect(x + 10, y, 5, 16, c);
}

void LcdRenderer::drawStatus(const char* s) {
    g_.fillRect(0, kStatusY, kPanelX - 4, 320 - kStatusY, bg_);
    g_.setFont(&fonts::Font2);
    g_.setTextColor(lit_, bg_);
    g_.setTextDatum(textdatum_t::top_center);
    g_.drawString(s, kFieldX + kFieldW / 2, kStatusY);
}

void LcdRenderer::present(const Frame& f) {
    const char* status = f.status ? f.status : "";
    g_.startWrite();

    const bool full = full_;
    if (full) drawStatic();

    for (int y = 0; y < Field::kHeight; ++y) {
        const uint16_t diff = full ? Field::kFullRow : uint16_t(f.field.rows[y] ^ prev_.field.rows[y]);
        if (!diff) continue;
        for (int x = 0; x < Field::kWidth; ++x)
            if (diff & (1u << x)) drawCell(kFieldX + x * kCell, kFieldY + y * kCell, f.field.get(x, y));
    }

    for (int y = 0; y < Preview::kHeight; ++y) {
        const uint16_t diff = full ? Preview::kFullRow : uint16_t(f.next.rows[y] ^ prev_.next.rows[y]);
        for (int x = 0; x < Preview::kWidth; ++x)
            if (diff & (1u << x)) drawCell(kNextX + x * kCell, kNextGridY + y * kCell, f.next.get(x, y));
    }

    if (full || f.hud.hiScore != prev_.hud.hiScore) drawNumber(kHiY, f.hud.hiScore, 1);
    if (full || f.hud.score != prev_.hud.score) drawNumber(kScoreY, f.hud.score, 1);
    if (full || f.hud.speed != prev_.hud.speed) drawNumber(kSpeedY, f.hud.speed, 1);
    if (full || f.hud.level != prev_.hud.level) drawNumber(kLevelY, f.hud.level, 1);
    if (full || f.soundOn != prev_.soundOn) drawSoundIcon(f.soundOn);
    if (full || f.paused != prev_.paused) drawPauseIcon(f.paused);
    if (full || strncmp(status, prevStatus_, sizeof(prevStatus_)) != 0) {
        drawStatus(status);
        strncpy(prevStatus_, status, sizeof(prevStatus_) - 1);
    }

    g_.endWrite();
    prev_ = f;
    prev_.status = prevStatus_;
    full_ = false;
}
