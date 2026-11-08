#pragma once
#include <stdint.h>
#include "../core/Game.h"
#include "../core/Grid.h"
#include "../core/Rng.h"

// Brick-Game Tetris.
//   LEFT/RIGHT move (auto-repeat), DOWN soft drop (auto-repeat), UP hard drop, A rotate CW.
//   Rotation tries wall-kick offsets {0, -1, +1, -2, +2}. 7-bag randomizer.
//   Score 100/300/500/800 × (L+1), where L = start speed + level; level up every 10 lines.
//   Gravity ≈ 800 ms × 0.85^L, minimum 50 ms. Pieces lock as soon as they can't fall.
class Tetris : public Game {
public:
    static constexpr int kSpawnX = 3;
    static constexpr uint32_t kClearFlashMs = 300;
    static constexpr uint32_t kFlashToggleMs = 75;
    static constexpr uint32_t kMaxScore = 999999;

    const char* name() const override { return "TETRIS"; }
    const char* hiKey() const override { return "hi_tetris"; }
    uint16_t icon() const override { return shape(kT, 0); }

    void reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) override;
    void onButton(Button b, ButtonEvent ev, uint32_t nowMs) override;
    void update(uint32_t nowMs) override;
    bool isOver() const override { return over_; }
    uint32_t score() const override { return score_; }
    void draw(Frame& f) const override;

    // Exposed for host tests.
    static uint16_t shape(uint8_t type, uint8_t rot);
    static uint32_t dropIntervalFor(uint8_t effLevel);
    const Field& board() const { return board_; }
    uint32_t lines() const { return lines_; }
    uint8_t level() const { return level_; }
    bool clearing() const { return clearMask_ != 0; }
    uint8_t pieceType() const { return type_; }
    int pieceX() const { return x_; }
    int pieceY() const { return y_; }
    uint8_t pieceRot() const { return rot_; }
    void debugSetBoard(const Field& b) { board_ = b; }

    enum : uint8_t { kI, kJ, kL, kO, kS, kT, kZ, kPieceCount };

private:
    bool fits(uint8_t rot, int x, int y) const;
    bool tryMove(int dx, int dy);
    bool tryRotate();
    void stepDown();
    void lock();
    void spawn();
    void finishClear();
    uint8_t takeFromBag();
    uint8_t effLevel() const { return uint8_t(startSpeed_ + level_); }
    void addScore(uint32_t pts);

    Field board_;
    Rng rng_;
    uint8_t bag_[kPieceCount] = {};
    uint8_t bagLeft_ = 0;

    uint8_t type_ = 0, rot_ = 0, next_ = 0;
    int x_ = 0, y_ = 0;

    uint32_t score_ = 0;
    uint32_t lines_ = 0;
    uint8_t level_ = 0;
    uint8_t startSpeed_ = 0;

    uint32_t now_ = 0;
    uint32_t lastDrop_ = 0;
    uint32_t clearStart_ = 0;
    uint32_t clearMask_ = 0;   // bit y = row y is being cleared
    bool over_ = false;
};
