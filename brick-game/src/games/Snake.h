#pragma once
#include <stdint.h>
#include "../core/Game.h"
#include "../core/Grid.h"
#include "../core/Rng.h"

// Brick-Game Snake.
//   D-pad steers (no reversing into yourself); up to 2 turns are buffered so quick
//   zig-zags work. Hold A to go 3× faster.
//   Walls and your own body are deadly. The apple blinks.
//   Score 10 × (L+1) per apple, L = start speed + level; level up every 8 apples.
//   Step ≈ 400 ms × 0.85^L, minimum 60 ms.
class Snake : public Game {
public:
    static constexpr int kMaxLen = Field::kWidth * Field::kHeight;
    static constexpr int kStartLen = 3;
    static constexpr int kStartX = 4, kStartY = 12;
    static constexpr int kApplesPerLevel = 8;
    static constexpr uint32_t kFoodBlinkMs = 180;
    static constexpr uint32_t kBoostDiv = 3;
    static constexpr uint32_t kMaxScore = 999999;

    enum class Dir : uint8_t { Up, Down, Left, Right };

    const char* name() const override { return "SNAKE"; }
    const char* hiKey() const override { return "hi_snake"; }
    uint16_t icon() const override { return 0xE2E8; }   // an "S"

    void reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) override;
    void onButton(Button b, ButtonEvent ev, uint32_t nowMs) override;
    void update(uint32_t nowMs) override;
    bool isOver() const override { return over_; }
    uint32_t score() const override { return score_; }
    void draw(Frame& f) const override;

    // Exposed for host tests.
    static uint32_t stepIntervalFor(uint8_t effLevel);
    uint32_t stepInterval() const;
    int length() const { return len_; }
    int headX() const { return xs_[head_]; }
    int headY() const { return ys_[head_]; }
    int foodX() const { return foodX_; }
    int foodY() const { return foodY_; }
    Dir dir() const { return dir_; }
    uint8_t level() const { return level_; }
    void debugSetFood(int x, int y) { foodX_ = int8_t(x); foodY_ = int8_t(y); }
    void debugStep() { step(); }

private:
    void step();
    bool placeFood();
    uint8_t effLevel() const { return uint8_t(startSpeed_ + level_); }
    void die();

    // Body as a ring buffer; head_ is the index of the head, the tail is len_-1 behind it.
    int8_t xs_[kMaxLen] = {}, ys_[kMaxLen] = {};
    int head_ = 0;
    int len_ = 0;
    Field occ_;   // body cells, for O(1) self-collision

    Dir dir_ = Dir::Up;
    Dir queue_[2] = {};
    uint8_t queued_ = 0;
    bool boost_ = false;

    int8_t foodX_ = -1, foodY_ = -1;
    Rng rng_;

    uint32_t score_ = 0;
    uint32_t apples_ = 0;
    uint8_t level_ = 0;
    uint8_t startSpeed_ = 0;
    uint32_t now_ = 0;
    uint32_t lastStep_ = 0;
    bool over_ = false;
};
