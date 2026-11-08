#pragma once
#include <stdint.h>
#include "../core/Game.h"
#include "../core/Grid.h"
#include "../core/Rng.h"

// Brick-Game car racing. Two lanes between scrolling road edges.
//   LEFT/RIGHT change lane; hold A or UP to go 2× faster.
//   Oncoming cars spawn every kCarGap rows in a random lane, so there is always room
//   to switch lanes. Any overlap with a car is a crash.
//   Score 10 × (L+1) per car passed, L = start speed + level; level up every 15 cars.
//   Tick ≈ 180 ms × 0.88^L, minimum 40 ms.
class Racing : public Game {
public:
    static constexpr uint16_t kCar = 0x4E4A;   // .X. / XXX / .X. / X.X
    static constexpr int kCarW = 3, kCarH = 4;
    static constexpr int kLaneX[2] = {2, 5};
    static constexpr int kPlayerY = Field::kHeight - kCarH;   // 16
    static constexpr int kCarGap = 10;                         // rows from one spawn to the next
    static constexpr int kMaxCars = 4;
    static constexpr int kCarsPerLevel = 15;
    static constexpr uint32_t kBoostDiv = 2;
    static constexpr uint32_t kMaxScore = 999999;

    const char* name() const override { return "RACING"; }
    const char* hiKey() const override { return "hi_racing"; }
    uint16_t icon() const override { return kCar; }

    void reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) override;
    void onButton(Button b, ButtonEvent ev, uint32_t nowMs) override;
    void update(uint32_t nowMs) override;
    bool isOver() const override { return over_; }
    uint32_t score() const override { return score_; }
    void draw(Frame& f) const override;

    // Exposed for host tests.
    struct Car {
        int y;
        uint8_t lane;
        bool active;
    };
    static uint32_t tickIntervalFor(uint8_t effLevel);
    uint32_t tickInterval() const;
    uint8_t lane() const { return lane_; }
    uint32_t passed() const { return passed_; }
    uint8_t level() const { return level_; }
    const Car* cars() const { return cars_; }
    void debugTick() { tick(); }
    void debugSetCar(int i, int y, uint8_t lane) { cars_[i] = Car{y, lane, true}; }
    void debugClearCars() { for (auto& c : cars_) c.active = false; }

private:
    void tick();
    void spawn();
    bool crashed() const;
    void checkCrash();
    uint8_t effLevel() const { return uint8_t(startSpeed_ + level_); }

    Car cars_[kMaxCars] = {};
    uint8_t lane_ = 0;
    int rowsSinceSpawn_ = 0;
    uint8_t scroll_ = 0;
    bool boostA_ = false, boostUp_ = false;
    Rng rng_;

    uint32_t score_ = 0;
    uint32_t passed_ = 0;
    uint8_t level_ = 0;
    uint8_t startSpeed_ = 0;
    uint32_t now_ = 0;
    uint32_t lastTick_ = 0;
    bool over_ = false;
};
