#include "Snake.h"
#include "../core/Speed.h"

namespace {

bool opposite(Snake::Dir a, Snake::Dir b) {
    using D = Snake::Dir;
    return (a == D::Up && b == D::Down) || (a == D::Down && b == D::Up) ||
           (a == D::Left && b == D::Right) || (a == D::Right && b == D::Left);
}

void delta(Snake::Dir d, int& dx, int& dy) {
    dx = dy = 0;
    switch (d) {
    case Snake::Dir::Up: dy = -1; break;
    case Snake::Dir::Down: dy = +1; break;
    case Snake::Dir::Left: dx = -1; break;
    case Snake::Dir::Right: dx = +1; break;
    }
}

} // namespace

uint32_t Snake::stepIntervalFor(uint8_t effLevel) { return decayInterval(400, effLevel, 85, 60); }

uint32_t Snake::stepInterval() const {
    const uint32_t ms = stepIntervalFor(effLevel());
    return boost_ ? ms / kBoostDiv : ms;
}

void Snake::reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) {
    rng_.reseed(seed);
    occ_.clear();
    // Head at the start cell, body trailing below it, heading up.
    len_ = kStartLen;
    head_ = kStartLen - 1;
    for (int i = 0; i < kStartLen; ++i) {
        const int idx = head_ - i;
        xs_[idx] = int8_t(kStartX);
        ys_[idx] = int8_t(kStartY + i);
        occ_.set(kStartX, kStartY + i);
    }
    dir_ = Dir::Up;
    queued_ = 0;
    boost_ = false;
    score_ = 0;
    apples_ = 0;
    level_ = 0;
    startSpeed_ = startSpeed;
    over_ = false;
    now_ = nowMs;
    lastStep_ = nowMs;
    placeFood();
}

bool Snake::placeFood() {
    const int free = Field::kWidth * Field::kHeight - len_;
    if (free <= 0) return false;
    int k = int(rng_.below(uint32_t(free)));
    for (int y = 0; y < Field::kHeight; ++y)
        for (int x = 0; x < Field::kWidth; ++x)
            if (!occ_.get(x, y) && k-- == 0) {
                foodX_ = int8_t(x);
                foodY_ = int8_t(y);
                return true;
            }
    return false;
}

void Snake::die() {
    over_ = true;
    sfx(Sfx::GameOver);
}

void Snake::step() {
    if (queued_) {
        dir_ = queue_[0];
        queue_[0] = queue_[1];
        --queued_;
    }
    int dx, dy;
    delta(dir_, dx, dy);
    const int nx = xs_[head_] + dx, ny = ys_[head_] + dy;

    if (!Field::inside(nx, ny)) return die();

    const bool eating = nx == foodX_ && ny == foodY_;
    const int tail = (head_ - (len_ - 1) + kMaxLen) % kMaxLen;
    // The tail moves away this step unless we grow, so moving into it is allowed.
    const bool intoTail = nx == xs_[tail] && ny == ys_[tail];
    if (occ_.get(nx, ny) && !(intoTail && !eating)) return die();

    if (!eating) {
        occ_.set(xs_[tail], ys_[tail], false);
    } else {
        ++len_;
    }
    head_ = (head_ + 1) % kMaxLen;
    xs_[head_] = int8_t(nx);
    ys_[head_] = int8_t(ny);
    occ_.set(nx, ny);

    if (eating) {
        score_ += 10 * (uint32_t(effLevel()) + 1);
        if (score_ > kMaxScore) score_ = kMaxScore;
        ++apples_;
        sfx(Sfx::Line);
        const uint8_t newLevel = uint8_t(apples_ / kApplesPerLevel > 99 ? 99 : apples_ / kApplesPerLevel);
        if (newLevel != level_) {
            level_ = newLevel;
            sfx(Sfx::LevelUp);
        }
        if (!placeFood()) die();   // board full: you win, game ends
    }
}

void Snake::onButton(Button b, ButtonEvent ev, uint32_t nowMs) {
    now_ = nowMs;
    if (over_) return;

    if (b == Button::A) {
        if (ev == ButtonEvent::Press) boost_ = true;
        else if (ev == ButtonEvent::Release) boost_ = false;
        return;
    }
    if (ev != ButtonEvent::Press) return;

    Dir d;
    switch (b) {
    case Button::Up: d = Dir::Up; break;
    case Button::Down: d = Dir::Down; break;
    case Button::Left: d = Dir::Left; break;
    case Button::Right: d = Dir::Right; break;
    default: return;
    }
    // Compare against the last direction we will be travelling in.
    const Dir last = queued_ ? queue_[queued_ - 1] : dir_;
    if (d == last || opposite(d, last) || queued_ >= 2) return;
    queue_[queued_++] = d;
    sfx(Sfx::Move);
}

void Snake::update(uint32_t nowMs) {
    now_ = nowMs;
    if (over_) return;
    if (now_ - lastStep_ >= stepInterval()) {
        lastStep_ = now_;
        step();
    }
}

void Snake::draw(Frame& f) const {
    f.field = occ_;
    if (!over_ && ((now_ / kFoodBlinkMs) & 1) == 0) f.field.set(foodX_, foodY_);
    maskToPreview(icon(), f.next);
    f.hud.score = score_;
    f.hud.speed = effLevel() > 99 ? 99 : effLevel();
    f.hud.level = level_;
}
