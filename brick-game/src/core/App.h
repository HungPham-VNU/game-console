#pragma once
#include <stdint.h>
#include "Buttons.h"
#include "Frame.h"
#include "Game.h"
#include "Platform.h"

// Top-level state machine:  MENU → PLAYING ⇄ PAUSED → CURTAIN → GAME OVER → MENU
//
//  MENU      UP/DOWN pick game, LEFT/RIGHT pick start speed, START or A starts.
//  PLAYING   START pauses; other buttons go to the game.
//  any       B toggles sound; hold SELECT 1 s returns to the menu (ends the game).
class App {
public:
    enum class State : uint8_t { Menu, Playing, Paused, Curtain, GameOver };

    static constexpr uint32_t kResetHoldMs = 1000;
    static constexpr uint32_t kCurtainRowMs = 30;   // per row, fill then clear
    static constexpr uint8_t kMaxSpeed = 9;

    App(Platform& platform, Game* const* games, uint8_t gameCount);

    void begin(uint32_t nowMs);
    void onButton(Button b, ButtonEvent ev, uint32_t nowMs);
    void update(uint32_t nowMs);
    void draw(Frame& f) const;

    State state() const { return state_; }
    bool soundOn() const { return sound_; }

private:
    Game& game() const { return *games_[sel_]; }
    uint32_t gameTime(uint32_t now) const { return now - pausedTotal_; }
    void play(Sfx s);
    void enterMenu();
    void startGame(uint32_t now);
    void endGame();   // saves the high score if beaten

    Platform& p_;
    Game* const* games_;
    uint8_t count_;

    State state_ = State::Menu;
    uint8_t sel_ = 0;
    uint8_t speed_ = 0;
    bool sound_ = true;
    uint32_t hi_ = 0;

    uint32_t now_ = 0;
    uint32_t pauseStart_ = 0;
    uint32_t pausedTotal_ = 0;
    uint32_t curtainStart_ = 0;
    uint32_t selectDownAt_ = 0;
    bool selectHeld_ = false;

    Frame last_;   // final game frame, used by the curtain and game-over screens
};
