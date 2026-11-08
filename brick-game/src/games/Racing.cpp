#include "Racing.h"
#include "../core/Speed.h"

namespace {

void drawCar(Field& f, int x, int y) {
    for (int r = 0; r < Racing::kCarH; ++r)
        for (int c = 0; c < Racing::kCarW; ++c)
            if (maskCell(Racing::kCar, r, c)) f.set(x + c, y + r);   // set() clips off-field rows
}

// Do two cars (top-left at (ax, ay) and (bx, by)) share any lit cell?
bool carsOverlap(int ax, int ay, int bx, int by) {
    for (int r = 0; r < Racing::kCarH; ++r)
        for (int c = 0; c < Racing::kCarW; ++c) {
            if (!maskCell(Racing::kCar, r, c)) continue;
            const int x = ax + c - bx, y = ay + r - by;
            if (x >= 0 && x < Racing::kCarW && y >= 0 && y < Racing::kCarH && maskCell(Racing::kCar, y, x))
                return true;
        }
    return false;
}

} // namespace

uint32_t Racing::tickIntervalFor(uint8_t effLevel) { return decayInterval(180, effLevel, 88, 40); }

uint32_t Racing::tickInterval() const {
    const uint32_t ms = tickIntervalFor(effLevel());
    return (boostA_ || boostUp_) ? ms / kBoostDiv : ms;
}

void Racing::reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) {
    rng_.reseed(seed);
    for (auto& c : cars_) c.active = false;
    lane_ = 0;
    rowsSinceSpawn_ = kCarGap - 4;   // first car shows up after a short run-in
    scroll_ = 0;
    boostA_ = boostUp_ = false;
    score_ = 0;
    passed_ = 0;
    level_ = 0;
    startSpeed_ = startSpeed;
    now_ = nowMs;
    lastTick_ = nowMs;
    over_ = false;
}

void Racing::spawn() {
    for (auto& c : cars_)
        if (!c.active) {
            c = Car{-kCarH, uint8_t(rng_.below(2)), true};
            return;
        }
}

bool Racing::crashed() const {
    for (const auto& c : cars_)
        if (c.active && carsOverlap(kLaneX[c.lane], c.y, kLaneX[lane_], kPlayerY)) return true;
    return false;
}

void Racing::checkCrash() {
    if (crashed()) {
        over_ = true;
        sfx(Sfx::GameOver);
    }
}

void Racing::tick() {
    ++scroll_;
    for (auto& c : cars_) {
        if (!c.active) continue;
        ++c.y;
        if (c.y >= Field::kHeight) {   // fully past the bottom
            c.active = false;
            ++passed_;
            score_ += 10 * (uint32_t(effLevel()) + 1);
            if (score_ > kMaxScore) score_ = kMaxScore;
            sfx(Sfx::Drop);
            const uint8_t newLevel = uint8_t(passed_ / kCarsPerLevel > 99 ? 99 : passed_ / kCarsPerLevel);
            if (newLevel != level_) {
                level_ = newLevel;
                sfx(Sfx::LevelUp);
            }
        }
    }
    if (++rowsSinceSpawn_ >= kCarGap) {
        rowsSinceSpawn_ = 0;
        spawn();
    }
    checkCrash();
}

void Racing::onButton(Button b, ButtonEvent ev, uint32_t nowMs) {
    now_ = nowMs;
    if (over_) return;
    const bool press = ev == ButtonEvent::Press, release = ev == ButtonEvent::Release;

    switch (b) {
    case Button::Left:
        if (press && lane_ != 0) {
            lane_ = 0;
            sfx(Sfx::Move);
            checkCrash();
        }
        break;
    case Button::Right:
        if (press && lane_ != 1) {
            lane_ = 1;
            sfx(Sfx::Move);
            checkCrash();
        }
        break;
    case Button::A:
        if (press) boostA_ = true;
        else if (release) boostA_ = false;
        break;
    case Button::Up:
        if (press) boostUp_ = true;
        else if (release) boostUp_ = false;
        break;
    default:
        break;
    }
}

void Racing::update(uint32_t nowMs) {
    now_ = nowMs;
    if (over_) return;
    if (now_ - lastTick_ >= tickInterval()) {
        lastTick_ = now_;
        tick();
    }
}

void Racing::draw(Frame& f) const {
    f.field.clear();
    // Road edges: 3 on, 1 off, scrolling down with the road.
    for (int y = 0; y < Field::kHeight; ++y) {
        const bool on = ((y - scroll_) & 3) != 0;
        f.field.set(0, y, on);
        f.field.set(Field::kWidth - 1, y, on);
    }
    for (const auto& c : cars_)
        if (c.active) drawCar(f.field, kLaneX[c.lane], c.y);
    drawCar(f.field, kLaneX[lane_], kPlayerY);

    maskToPreview(icon(), f.next);
    f.hud.score = score_;
    f.hud.speed = effLevel() > 99 ? 99 : effLevel();
    f.hud.level = level_;
}
