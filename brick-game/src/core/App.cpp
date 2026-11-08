#include "App.h"
#include "Font3x5.h"

App::App(Platform& platform, Game* const* games, uint8_t gameCount)
    : p_(platform), games_(games), count_(gameCount) {}

void App::begin(uint32_t nowMs) {
    now_ = nowMs;
    enterMenu();
}

void App::play(Sfx s) {
    if (sound_ && s != Sfx::None) p_.play(s);
}

void App::enterMenu() {
    state_ = State::Menu;
    selectHeld_ = false;
    hi_ = p_.loadHiScore(game().hiKey());
}

void App::startGame(uint32_t now) {
    pausedTotal_ = 0;
    hi_ = p_.loadHiScore(game().hiKey());
    game().reset(p_.randomSeed(), speed_, gameTime(now));
    state_ = State::Playing;
    play(Sfx::Start);
}

void App::endGame() {
    const uint32_t s = game().score();
    if (s > hi_) {
        hi_ = s;
        p_.saveHiScore(game().hiKey(), s);
    }
}

void App::onButton(Button b, ButtonEvent ev, uint32_t nowMs) {
    now_ = nowMs;

    // Global buttons.
    if (b == Button::Select) {
        if (ev == ButtonEvent::Press) {
            selectHeld_ = true;
            selectDownAt_ = nowMs;
        } else if (ev == ButtonEvent::Release) {
            selectHeld_ = false;
        }
        return;
    }
    if (b == Button::B) {
        if (ev == ButtonEvent::Press) {
            sound_ = !sound_;
            play(Sfx::Move);
        }
        return;
    }

    const bool press = ev == ButtonEvent::Press;
    const bool pressOrRepeat = ev != ButtonEvent::Release;

    switch (state_) {
    case State::Menu:
        if (press && (b == Button::Start || b == Button::A)) {
            startGame(nowMs);
        } else if (press && b == Button::Up && count_ > 1) {
            sel_ = uint8_t((sel_ + count_ - 1) % count_);
            enterMenu();
            play(Sfx::Move);
        } else if (press && b == Button::Down && count_ > 1) {
            sel_ = uint8_t((sel_ + 1) % count_);
            enterMenu();
            play(Sfx::Move);
        } else if (pressOrRepeat && b == Button::Left && speed_ > 0) {
            --speed_;
            play(Sfx::Move);
        } else if (pressOrRepeat && b == Button::Right && speed_ < kMaxSpeed) {
            ++speed_;
            play(Sfx::Move);
        }
        break;

    case State::Playing:
        if (b == Button::Start) {
            if (press) {
                state_ = State::Paused;
                pauseStart_ = nowMs;
            }
        } else {
            game().onButton(b, ev, gameTime(nowMs));
        }
        break;

    case State::Paused:
        if (press && b == Button::Start) {
            pausedTotal_ += nowMs - pauseStart_;
            state_ = State::Playing;
        }
        break;

    case State::Curtain:
        break;

    case State::GameOver:
        if (press && (b == Button::Start || b == Button::A)) enterMenu();
        break;
    }
}

void App::update(uint32_t nowMs) {
    now_ = nowMs;

    if (selectHeld_ && nowMs - selectDownAt_ >= kResetHoldMs) {
        selectHeld_ = false;
        if (state_ == State::Playing || state_ == State::Paused) endGame();
        if (state_ != State::Menu) enterMenu();
        return;
    }

    if (state_ == State::Playing) {
        game().update(gameTime(nowMs));
        play(game().takeSfx());
        if (game().isOver()) {
            game().draw(last_);
            endGame();
            curtainStart_ = nowMs;
            state_ = State::Curtain;
        }
    } else if (state_ == State::Curtain) {
        if (nowMs - curtainStart_ >= 2 * Field::kHeight * kCurtainRowMs) state_ = State::GameOver;
    }
}

void App::draw(Frame& f) const {
    f.soundOn = sound_;
    f.paused = state_ == State::Paused;

    switch (state_) {
    case State::Menu: {
        f.field.clear();
        font3x5::drawDigit(f.field, uint8_t(sel_ + 1), 2, 1, 2);   // game number, big
        font3x5::drawDigit(f.field, speed_, 3, 14, 1);             // start speed, small
        maskToPreview(game().icon(), f.next);
        f.hud = Hud{};
        f.hud.hiScore = hi_;
        f.hud.speed = speed_;
        f.status = game().name();
        break;
    }
    case State::Playing:
    case State::Paused:
        game().draw(f);
        f.hud.hiScore = hi_ > f.hud.score ? hi_ : f.hud.score;
        f.status = state_ == State::Paused ? "PAUSE" : "";
        break;

    case State::Curtain: {
        f = last_;
        f.soundOn = sound_;
        f.paused = false;
        f.hud.hiScore = hi_;
        const int H = Field::kHeight;
        const int step = int((now_ - curtainStart_) / kCurtainRowMs);
        if (step < H) {   // fill rows bottom → top
            for (int y = H - 1 - step; y < H; ++y) f.field.rows[y] = Field::kFullRow;
        } else {          // then clear top → bottom
            f.field.fill();
            for (int y = 0; y <= step - H && y < H; ++y) f.field.rows[y] = 0;
        }
        f.status = "";
        break;
    }
    case State::GameOver:
        f = last_;
        f.soundOn = sound_;
        f.paused = false;
        f.hud.hiScore = hi_;
        f.field.clear();
        f.status = "GAME OVER";
        break;
    }
}
