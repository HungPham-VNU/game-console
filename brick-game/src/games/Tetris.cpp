#include "Tetris.h"

namespace {

// 7 pieces × 4 clockwise rotations, 4×4 masks (bit 15 = top-left).
constexpr uint16_t kShapes[Tetris::kPieceCount][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444},   // I
    {0x8E00, 0x6440, 0x0E20, 0x44C0},   // J
    {0x2E00, 0x4460, 0x0E80, 0xC440},   // L
    {0x6600, 0x6600, 0x6600, 0x6600},   // O
    {0x6C00, 0x4620, 0x06C0, 0x8C40},   // S
    {0x4E00, 0x4640, 0x0E40, 0x4C40},   // T
    {0xC600, 0x2640, 0x0C60, 0x4C80},   // Z
};

constexpr int kKicks[] = {0, -1, +1, -2, +2};
constexpr uint32_t kLinePoints[5] = {0, 100, 300, 500, 800};

} // namespace

uint16_t Tetris::shape(uint8_t type, uint8_t rot) { return kShapes[type % kPieceCount][rot & 3]; }

uint32_t Tetris::dropIntervalFor(uint8_t effLevel) {
    uint32_t ms = 800;
    for (uint8_t i = 0; i < effLevel && ms > 50; ++i) ms = ms * 85 / 100;
    return ms < 50 ? 50 : ms;
}

void Tetris::reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) {
    board_.clear();
    rng_.reseed(seed);
    bagLeft_ = 0;
    score_ = 0;
    lines_ = 0;
    level_ = 0;
    startSpeed_ = startSpeed;
    clearMask_ = 0;
    over_ = false;
    now_ = nowMs;
    lastDrop_ = nowMs;
    next_ = takeFromBag();
    spawn();
}

uint8_t Tetris::takeFromBag() {
    if (bagLeft_ == 0) {
        for (uint8_t i = 0; i < kPieceCount; ++i) bag_[i] = i;
        for (int i = kPieceCount - 1; i > 0; --i) {   // Fisher–Yates
            const int j = int(rng_.below(uint32_t(i + 1)));
            const uint8_t t = bag_[i];
            bag_[i] = bag_[j];
            bag_[j] = t;
        }
        bagLeft_ = kPieceCount;
    }
    return bag_[--bagLeft_];
}

bool Tetris::fits(uint8_t rot, int x, int y) const {
    const uint16_t m = shape(type_, rot);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            if (!maskCell(m, r, c)) continue;
            const int bx = x + c, by = y + r;
            if (bx < 0 || bx >= Field::kWidth || by >= Field::kHeight) return false;
            if (by >= 0 && board_.get(bx, by)) return false;
        }
    return true;
}

bool Tetris::tryMove(int dx, int dy) {
    if (!fits(rot_, x_ + dx, y_ + dy)) return false;
    x_ += dx;
    y_ += dy;
    return true;
}

bool Tetris::tryRotate() {
    const uint8_t r = uint8_t((rot_ + 1) & 3);
    for (int k : kKicks) {
        if (fits(r, x_ + k, y_)) {
            rot_ = r;
            x_ += k;
            return true;
        }
    }
    return false;
}

void Tetris::spawn() {
    type_ = next_;
    next_ = takeFromBag();
    rot_ = 0;
    x_ = kSpawnX;
    y_ = 0;
    lastDrop_ = now_;
    if (!fits(rot_, x_, y_)) {
        over_ = true;
        sfx(Sfx::GameOver);
    }
}

void Tetris::addScore(uint32_t pts) {
    score_ += pts;
    if (score_ > kMaxScore) score_ = kMaxScore;
}

void Tetris::lock() {
    const uint16_t m = shape(type_, rot_);
    bool above = false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (maskCell(m, r, c)) {
                if (y_ + r < 0) above = true;
                board_.set(x_ + c, y_ + r);
            }
    if (above) {
        over_ = true;
        sfx(Sfx::GameOver);
        return;
    }

    clearMask_ = 0;
    int n = 0;
    for (int y = 0; y < Field::kHeight; ++y)
        if (board_.rowFull(y)) {
            clearMask_ |= 1u << y;
            ++n;
        }

    if (n == 0) {
        sfx(Sfx::Drop);
        spawn();
        return;
    }

    addScore(kLinePoints[n] * (uint32_t(effLevel()) + 1));
    lines_ += uint32_t(n);
    const uint8_t newLevel = uint8_t(lines_ / 10 > 99 ? 99 : lines_ / 10);
    sfx(n == 4 ? Sfx::Tetris : Sfx::Line);
    if (newLevel != level_) {
        level_ = newLevel;
        sfx(Sfx::LevelUp);
    }
    clearStart_ = now_;   // finishClear() runs after the flash
}

void Tetris::finishClear() {
    Field out;
    int dst = Field::kHeight - 1;
    for (int y = Field::kHeight - 1; y >= 0; --y)
        if (!(clearMask_ & (1u << y))) out.rows[dst--] = board_.rows[y];
    board_ = out;
    clearMask_ = 0;
    spawn();
}

void Tetris::stepDown() {
    if (!tryMove(0, 1)) lock();
    lastDrop_ = now_;
}

void Tetris::onButton(Button b, ButtonEvent ev, uint32_t nowMs) {
    now_ = nowMs;
    if (over_ || clearMask_ || ev == ButtonEvent::Release) return;
    const bool press = ev == ButtonEvent::Press;

    switch (b) {
    case Button::Left:
        if (tryMove(-1, 0)) sfx(Sfx::Move);
        break;
    case Button::Right:
        if (tryMove(+1, 0)) sfx(Sfx::Move);
        break;
    case Button::Down:   // soft drop: one row per press/repeat
        stepDown();
        break;
    case Button::Up:     // hard drop
        if (press) {
            while (tryMove(0, 1)) {}
            lock();
            lastDrop_ = now_;
        }
        break;
    case Button::A:
        if (press && tryRotate()) sfx(Sfx::Rotate);
        break;
    default:
        break;
    }
}

void Tetris::update(uint32_t nowMs) {
    now_ = nowMs;
    if (over_) return;
    if (clearMask_) {
        if (now_ - clearStart_ >= kClearFlashMs) finishClear();
        return;
    }
    if (now_ - lastDrop_ >= dropIntervalFor(effLevel())) stepDown();
}

void Tetris::draw(Frame& f) const {
    f.field = board_;

    if (clearMask_) {
        // Blink the full rows: off during even phases.
        if (((now_ - clearStart_) / kFlashToggleMs) % 2 == 0)
            for (int y = 0; y < Field::kHeight; ++y)
                if (clearMask_ & (1u << y)) f.field.rows[y] = 0;
    } else if (!over_) {
        const uint16_t m = shape(type_, rot_);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                if (maskCell(m, r, c)) f.field.set(x_ + c, y_ + r);
    }

    maskToPreview(shape(next_, 0), f.next);
    f.hud.score = score_;
    f.hud.speed = effLevel() > 99 ? 99 : effLevel();
    f.hud.level = level_;
}
