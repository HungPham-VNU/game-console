#pragma once
#include <stdint.h>
#include "Buttons.h"
#include "Frame.h"
#include "Sfx.h"

// Interface every game implements. Games are platform-free: they see only a game clock
// (milliseconds, frozen while paused), button events, and write into a Frame.
// B (sound), START (pause) and SELECT (hold = reset) are handled by App, not by games.
class Game {
public:
    virtual ~Game() = default;

    virtual const char* name() const = 0;     // shown in the menu
    virtual const char* hiKey() const = 0;    // Preferences key, max 15 chars
    virtual uint16_t icon() const = 0;        // 4×4 mask for the NEXT box in the menu

    virtual void reset(uint32_t seed, uint8_t startSpeed, uint32_t nowMs) = 0;
    virtual void onButton(Button b, ButtonEvent ev, uint32_t nowMs) = 0;
    virtual void update(uint32_t nowMs) = 0;

    virtual bool isOver() const = 0;
    virtual uint32_t score() const = 0;

    // Fill f.field, f.next, f.hud.score/speed/level. App fills the rest.
    virtual void draw(Frame& f) const = 0;

    Sfx takeSfx() {
        const Sfx s = pending_;
        pending_ = Sfx::None;
        return s;
    }

protected:
    void sfx(Sfx s) {
        if (uint8_t(s) > uint8_t(pending_)) pending_ = s;
    }

private:
    Sfx pending_ = Sfx::None;
};
