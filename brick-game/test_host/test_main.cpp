// Host tests for core/ + games/ (no Arduino, no display). Run:  make -C test_host
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "../src/core/App.h"
#include "../src/core/Font3x5.h"
#include "../src/games/Racing.h"
#include "../src/games/Snake.h"
#include "../src/games/Tetris.h"
#include "../src/render/Layout.h"

static int g_fail = 0, g_pass = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) ++g_pass;                                                      \
        else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static int popcount16(uint16_t v) { return __builtin_popcount(v); }

static int cellsOnBoard(const Field& f) {
    int n = 0;
    for (auto r : f.rows) n += popcount16(r);
    return n;
}

struct FakePlatform : Platform {
    std::map<std::string, uint32_t> nvs;
    std::vector<Sfx> played;
    uint32_t seed = 12345;
    uint32_t loadHiScore(const char* k) override { return nvs.count(k) ? nvs[k] : 0; }
    void saveHiScore(const char* k, uint32_t v) override { nvs[k] = v; }
    void play(Sfx s) override { played.push_back(s); }
    uint32_t randomSeed() override { return seed++; }
};

static void testShapes() {
    // Every rotation has 4 cells; rotating 4 times returns to start (table is cyclic).
    for (uint8_t t = 0; t < Tetris::kPieceCount; ++t)
        for (uint8_t r = 0; r < 4; ++r) CHECK(popcount16(Tetris::shape(t, r)) == 4);

    // Rotation table is a true clockwise rotation of a 3×3 box (or 4×4 for I, fixed for O).
    for (uint8_t t = 0; t < Tetris::kPieceCount; ++t) {
        if (t == Tetris::kO) continue;
        const int n = t == Tetris::kI ? 4 : 3;
        for (uint8_t r = 0; r < 4; ++r) {
            const uint16_t a = Tetris::shape(t, r), b = Tetris::shape(t, uint8_t(r + 1));
            uint16_t rot = 0;
            for (int y = 0; y < n; ++y)
                for (int x = 0; x < n; ++x)
                    if (maskCell(a, y, x)) rot |= uint16_t(0x8000u >> ((x) * 4 + (n - 1 - y)));
            CHECK(rot == b);
        }
    }
}

static void testDropInterval() {
    CHECK(Tetris::dropIntervalFor(0) == 800);
    CHECK(Tetris::dropIntervalFor(1) == 680);
    CHECK(Tetris::dropIntervalFor(5) < Tetris::dropIntervalFor(4));
    CHECK(Tetris::dropIntervalFor(30) == 50);
    CHECK(Tetris::dropIntervalFor(255) == 50);
}

static void testBag() {
    // Each run of 7 pieces (aligned to the bag) contains every piece exactly once.
    Tetris t;
    t.reset(42, 0, 0);
    uint32_t now = 0;
    std::vector<uint8_t> seq;
    for (int i = 0; i < 70 && !t.isOver(); ++i) {
        seq.push_back(t.pieceType());
        t.onButton(Button::Up, ButtonEvent::Press, now);   // hard drop
        // move it out of the way so we don't top out quickly: clear board each time
        Field empty;
        t.debugSetBoard(empty);
        now += 1;
        t.update(now);
    }
    CHECK(seq.size() == 70);
    // First piece comes from bag 0; bags are aligned at index 0.
    for (size_t b = 0; b + 7 <= seq.size(); b += 7) {
        int seen = 0;
        for (size_t i = b; i < b + 7; ++i) seen |= 1 << seq[i];
        CHECK(seen == 0x7F);
    }
}

static void testGravityAndLock() {
    Tetris t;
    t.reset(1, 0, 0);
    const int y0 = t.pieceY();
    t.update(799);
    CHECK(t.pieceY() == y0);
    t.update(800);
    CHECK(t.pieceY() == y0 + 1);

    // Hard drop: piece lands, 4 cells on the board, new piece spawns at the top.
    t.onButton(Button::Up, ButtonEvent::Press, 900);
    CHECK(cellsOnBoard(t.board()) == 4);
    CHECK(t.pieceY() == 0);
    CHECK(t.takeSfx() == Sfx::Drop);
}

static void testWallsAndKick() {
    Tetris t;
    t.reset(7, 0, 0);
    // Slam left: x never goes past the wall.
    for (int i = 0; i < 20; ++i) t.onButton(Button::Left, ButtonEvent::Repeat, 0);
    const int leftX = t.pieceX();
    for (int i = 0; i < 20; ++i) t.onButton(Button::Left, ButtonEvent::Repeat, 0);
    CHECK(t.pieceX() == leftX);
    // Rotating against the wall must still work (kick), 4 times back to rot 0.
    const uint8_t type = t.pieceType();
    for (int i = 0; i < 4; ++i) t.onButton(Button::A, ButtonEvent::Press, 0);
    CHECK(t.pieceType() == type);
    CHECK(t.pieceRot() == 0 || type == Tetris::kO);
    for (int i = 0; i < 20; ++i) t.onButton(Button::Right, ButtonEvent::Repeat, 0);
    const int rightX = t.pieceX();
    t.onButton(Button::A, ButtonEvent::Press, 0);
    // After a rotation near the right wall the piece is still fully inside.
    Frame f;
    t.draw(f);
    CHECK(cellsOnBoard(f.field) == 4);
    (void)rightX;
}

static void testLineClearAndScore() {
    Tetris t;
    t.reset(3, 0, 0);
    // Bottom 4 rows full except column 0 → any I piece dropped vertically there clears 4.
    // Use a generic approach: fill bottom row except where the piece will land.
    Field b;
    for (int y = 16; y < 20; ++y) b.rows[y] = Field::kFullRow & ~1u;   // column 0 empty
    t.debugSetBoard(b);

    // Cycle pieces until we get an I (hard-dropping elsewhere would ruin the setup, so
    // instead restart with different seeds until the first piece is an I).
    uint32_t seed = 3;
    while (t.pieceType() != Tetris::kI) {
        t.reset(++seed, 0, 0);
        t.debugSetBoard(b);
    }
    t.onButton(Button::A, ButtonEvent::Press, 0);   // vertical: rot 1 = column 2 of the box
    for (int i = 0; i < 10; ++i) t.onButton(Button::Left, ButtonEvent::Repeat, 0);
    t.onButton(Button::Up, ButtonEvent::Press, 0);
    CHECK(t.clearing());
    CHECK(t.takeSfx() == Sfx::Tetris);
    Frame f;
    t.draw(f);   // phase 0 → full rows blank
    CHECK(f.field.rows[19] == 0);
    t.update(Tetris::kClearFlashMs - 1);
    CHECK(t.clearing());
    t.update(Tetris::kClearFlashMs);
    CHECK(!t.clearing());
    CHECK(cellsOnBoard(t.board()) == 0);
    CHECK(t.lines() == 4);
    CHECK(t.score() == 800);
}

static void testLevelUp() {
    // Three tetrises in one game, start speed 2. Before each, rebuild a 4-row well in
    // column 0 and burn non-I pieces (they land above the well and get wiped by the rebuild).
    uint32_t seed = 100;
    uint32_t now = 0;
    Field b;
    for (int y = 16; y < 20; ++y) b.rows[y] = Field::kFullRow & ~1u;
    Tetris g;
    g.reset(seed, 2, 0);
    while (g.pieceType() != Tetris::kI) g.reset(++seed, 2, 0);
    for (int k = 0; k < 3; ++k) {
        g.debugSetBoard(b);
        while (g.pieceType() != Tetris::kI) {   // burn pieces by dropping them on the right
            g.onButton(Button::Up, ButtonEvent::Press, now);
            now += 400;
            g.update(now);
            g.debugSetBoard(b);
        }
        g.onButton(Button::A, ButtonEvent::Press, now);
        for (int i = 0; i < 10; ++i) g.onButton(Button::Left, ButtonEvent::Repeat, now);
        g.onButton(Button::Up, ButtonEvent::Press, now);
        CHECK(g.clearing());
        now += 400;
        g.update(now);
    }
    CHECK(g.lines() == 12);
    CHECK(g.level() == 1);
    // Score: 800×(2+1) twice (level 0), then the third tetris lands while level is still 0
    // (level is updated after scoring) → 3 × 2400.
    CHECK(g.score() == 3 * 800 * 3);
}

static void testGameOver() {
    Tetris t;
    t.reset(9, 0, 0);
    uint32_t now = 0;
    for (int i = 0; i < 200 && !t.isOver(); ++i) {
        t.onButton(Button::Up, ButtonEvent::Press, now);
        now += 400;
        t.update(now);
    }
    CHECK(t.isOver());
}

static void testApp() {
    FakePlatform p;
    Tetris t;
    Game* games[] = {&t};
    App app(p, games, 1);
    app.begin(0);
    CHECK(app.state() == App::State::Menu);

    Frame f;
    app.draw(f);
    CHECK(f.next.rows[0] != 0 || f.next.rows[1] != 0);   // icon shown

    app.onButton(Button::Right, ButtonEvent::Press, 0);
    app.onButton(Button::Right, ButtonEvent::Repeat, 0);
    app.draw(f);
    CHECK(f.hud.speed == 2);

    app.onButton(Button::Start, ButtonEvent::Press, 10);
    CHECK(app.state() == App::State::Playing);

    // Pause freezes gravity.
    app.update(20);
    app.draw(f);
    const Field before = f.field;
    app.onButton(Button::Start, ButtonEvent::Press, 30);
    CHECK(app.state() == App::State::Paused);
    app.update(30 + 10000);
    app.draw(f);
    CHECK(f.paused);
    CHECK(f.field == before);
    app.onButton(Button::Start, ButtonEvent::Press, 10030);
    CHECK(app.state() == App::State::Playing);

    // B toggles sound.
    app.onButton(Button::B, ButtonEvent::Press, 10040);
    CHECK(!app.soundOn());
    app.onButton(Button::B, ButtonEvent::Press, 10050);
    CHECK(app.soundOn());

    // Play until game over by hard-dropping everything.
    uint32_t now = 10100;
    for (int i = 0; i < 300 && app.state() == App::State::Playing; ++i) {
        app.onButton(Button::Up, ButtonEvent::Press, now);
        now += 400;
        app.update(now);
    }
    CHECK(app.state() == App::State::Curtain);
    app.update(now + 2 * 20 * App::kCurtainRowMs);
    CHECK(app.state() == App::State::GameOver);
    app.draw(f);
    CHECK(std::string(f.status) == "GAME OVER");

    app.onButton(Button::Start, ButtonEvent::Press, now + 2000);
    CHECK(app.state() == App::State::Menu);

    // Hold SELECT 1 s mid-game → back to menu, high score kept if beaten.
    app.onButton(Button::Start, ButtonEvent::Press, now + 3000);
    CHECK(app.state() == App::State::Playing);
    app.onButton(Button::Select, ButtonEvent::Press, now + 3100);
    app.update(now + 3100 + 999);
    CHECK(app.state() == App::State::Playing);
    app.update(now + 3100 + 1000);
    CHECK(app.state() == App::State::Menu);
}

static void testHiScorePersist() {
    FakePlatform p;
    p.nvs["hi_tetris"] = 0;
    Tetris t;
    Game* games[] = {&t};
    App app(p, games, 1);
    app.begin(0);
    app.onButton(Button::Start, ButtonEvent::Press, 0);

    // Give the game a guaranteed score: a 4-row well in column 0, then wait for an I.
    Field b;
    for (int y = 16; y < 20; ++y) b.rows[y] = Field::kFullRow & ~1u;
    t.debugSetBoard(b);
    uint32_t now = 0;
    while (t.pieceType() != Tetris::kI) {
        app.onButton(Button::Up, ButtonEvent::Press, now);
        now += 400;
        app.update(now);
        t.debugSetBoard(b);
    }
    app.onButton(Button::A, ButtonEvent::Press, now);
    for (int i = 0; i < 10; ++i) app.onButton(Button::Left, ButtonEvent::Repeat, now);
    app.onButton(Button::Up, ButtonEvent::Press, now);
    now += 400;
    app.update(now);
    CHECK(t.score() == 800);

    // Then play to game over: the high score must be saved exactly once the game ends.
    CHECK(p.nvs["hi_tetris"] == 0);
    for (int i = 0; i < 300 && app.state() == App::State::Playing; ++i) {
        app.onButton(Button::Up, ButtonEvent::Press, now);
        now += 400;
        app.update(now);
    }
    CHECK(app.state() == App::State::Curtain);
    CHECK(p.nvs["hi_tetris"] == t.score());
}

static void testFont() {
    Field f;
    font3x5::drawDigit(f, 8, 0, 0, 1);
    CHECK(cellsOnBoard(f) == 13);
    f.clear();
    font3x5::drawDigit(f, 1, 2, 1, 2);
    CHECK(cellsOnBoard(f) == 8 * 4);
}

// ---------------------------------------------------------------- Snake

static void testSnakeBasics() {
    Snake s;
    s.reset(1, 0, 0);
    CHECK(s.length() == Snake::kStartLen);
    CHECK(s.headX() == Snake::kStartX && s.headY() == Snake::kStartY);
    CHECK(s.dir() == Snake::Dir::Up);
    s.debugSetFood(0, 0);   // keep the apple out of the way

    // Moves up one cell per interval.
    s.update(Snake::stepIntervalFor(0) - 1);
    CHECK(s.headY() == Snake::kStartY);
    s.update(Snake::stepIntervalFor(0));
    CHECK(s.headY() == Snake::kStartY - 1);

    // Reversing is ignored; a perpendicular turn is taken on the next step.
    s.onButton(Button::Down, ButtonEvent::Press, 0);
    s.debugStep();
    CHECK(s.dir() == Snake::Dir::Up);
    s.onButton(Button::Right, ButtonEvent::Press, 0);
    s.debugStep();
    CHECK(s.dir() == Snake::Dir::Right);
    CHECK(s.headX() == Snake::kStartX + 1);

    // Two buffered turns (Down then Left) are both applied, one per step: a U-turn.
    const int y = s.headY();
    s.onButton(Button::Down, ButtonEvent::Press, 0);
    s.onButton(Button::Left, ButtonEvent::Press, 0);
    s.debugStep();
    CHECK(s.dir() == Snake::Dir::Down && s.headY() == y + 1);
    s.debugStep();
    CHECK(s.dir() == Snake::Dir::Left);

    // Boost: A held makes steps 3× faster, release restores.
    s.onButton(Button::A, ButtonEvent::Press, 0);
    CHECK(s.stepInterval() == Snake::stepIntervalFor(0) / Snake::kBoostDiv);
    s.onButton(Button::A, ButtonEvent::Release, 0);
    CHECK(s.stepInterval() == Snake::stepIntervalFor(0));

    CHECK(Snake::stepIntervalFor(0) == 400);
    CHECK(Snake::stepIntervalFor(50) == 60);
}

static void testSnakeEatAndGrow() {
    Snake s;
    s.reset(2, 0, 0);
    s.debugSetFood(Snake::kStartX, Snake::kStartY - 1);   // right in front
    s.debugStep();
    CHECK(s.length() == Snake::kStartLen + 1);
    CHECK(s.score() == 10);
    CHECK(s.takeSfx() == Sfx::Line);
    // New apple is not on the snake.
    Frame f;
    s.draw(f);
    CHECK(!(s.foodX() == s.headX() && s.foodY() == s.headY()));

    // Level up after kApplesPerLevel apples (feed it straight up the column).
    Snake t;
    t.reset(3, 1, 0);
    for (int i = 0; i < Snake::kApplesPerLevel; ++i) {
        t.debugSetFood(t.headX(), t.headY() - 1);
        t.debugStep();
    }
    CHECK(t.level() == 1);
    CHECK(t.length() == Snake::kStartLen + Snake::kApplesPerLevel);
    // Start speed 1 → L = 1 → 20 points per apple; the level changes after the 8th is scored.
    CHECK(t.score() == uint32_t(Snake::kApplesPerLevel) * 20);
}

static void testSnakeDeath() {
    // Wall.
    Snake s;
    s.reset(4, 0, 0);
    s.debugSetFood(9, 19);
    for (int i = 0; i < Snake::kStartY && !s.isOver(); ++i) s.debugStep();
    CHECK(!s.isOver());   // head at y = 0 now
    s.debugStep();
    CHECK(s.isOver());
    CHECK(s.takeSfx() == Sfx::GameOver);

    // Self: grow to 5, then turn into the body (right, down, left).
    Snake t;
    t.reset(5, 0, 0);
    for (int i = 0; i < 2; ++i) {
        t.debugSetFood(t.headX(), t.headY() - 1);
        t.debugStep();
    }
    CHECK(t.length() == 5);
    t.debugSetFood(0, 0);
    t.onButton(Button::Right, ButtonEvent::Press, 0);
    t.debugStep();
    t.onButton(Button::Down, ButtonEvent::Press, 0);
    t.debugStep();
    t.onButton(Button::Left, ButtonEvent::Press, 0);
    t.debugStep();
    CHECK(t.isOver());

    // Following your own tail exactly is fine (the tail moves away): a 2×2 loop with length 4.
    Snake u;
    u.reset(6, 0, 0);
    u.debugSetFood(u.headX(), u.headY() - 1);
    u.debugStep();   // length 4: head (4,11), body (4,12),(4,13),(4,14)
    u.debugSetFood(0, 19);
    const Button loop[] = {Button::Right, Button::Down, Button::Left, Button::Up};
    for (int lap = 0; lap < 3; ++lap)
        for (Button b : loop) {
            u.onButton(b, ButtonEvent::Press, 0);
            u.debugStep();
        }
    CHECK(!u.isOver());
}

// ---------------------------------------------------------------- Racing

static void testRacingBasics() {
    Racing r;
    r.reset(1, 0, 0);
    CHECK(r.lane() == 0);
    r.onButton(Button::Right, ButtonEvent::Press, 0);
    CHECK(r.lane() == 1);
    r.onButton(Button::Right, ButtonEvent::Press, 0);
    CHECK(r.lane() == 1);
    r.onButton(Button::Left, ButtonEvent::Press, 0);
    CHECK(r.lane() == 0);

    CHECK(Racing::tickIntervalFor(0) == 180);
    CHECK(Racing::tickIntervalFor(60) == 40);
    r.onButton(Button::Up, ButtonEvent::Press, 0);
    CHECK(r.tickInterval() == 90);
    r.onButton(Button::A, ButtonEvent::Press, 0);
    r.onButton(Button::Up, ButtonEvent::Release, 0);
    CHECK(r.tickInterval() == 90);   // A still held
    r.onButton(Button::A, ButtonEvent::Release, 0);
    CHECK(r.tickInterval() == 180);

    // Road edges and the player's car are drawn.
    Frame f;
    r.draw(f);
    int edge = 0;
    for (int y = 0; y < 20; ++y) edge += f.field.get(0, y);
    CHECK(edge == 15);   // 3 on, 1 off
    CHECK(f.field.get(Racing::kLaneX[0] + 1, Racing::kPlayerY));
}

static void testRacingCrash() {
    // A car coming down the player's lane: rows 11..14 → it first shares a lit cell with
    // the player (rows 16..19) at y = 14, i.e. on the 3rd tick (the X.X tail and .X. nose miss).
    Racing r;
    r.reset(2, 0, 0);
    r.debugClearCars();
    r.debugSetCar(0, 11, 0);
    r.debugTick();
    r.debugTick();
    CHECK(!r.isOver());
    r.debugTick();
    CHECK(r.isOver());
    CHECK(r.takeSfx() == Sfx::GameOver);

    // The same car in the other lane passes safely and scores once it leaves the field.
    Racing s;
    s.reset(3, 0, 0);
    s.debugClearCars();
    s.debugSetCar(0, 10, 1);
    for (int i = 0; i < 10; ++i) s.debugTick();
    CHECK(!s.isOver());
    CHECK(s.passed() == 1);
    CHECK(s.score() == 10);

    // Steering into a car alongside is a crash.
    Racing t;
    t.reset(4, 0, 0);
    t.debugClearCars();
    t.debugSetCar(0, Racing::kPlayerY, 1);
    t.onButton(Button::Right, ButtonEvent::Press, 0);
    CHECK(t.isOver());
}

static void testRacingAlwaysPassable() {
    // A simple bot that moves to the lane whose nearest car is further away must survive
    // a long run at every speed: spawns never block both lanes at once.
    for (uint8_t speed = 0; speed <= 9; ++speed) {
        Racing r;
        r.reset(1000 + speed, speed, 0);
        for (int t = 0; t < 3000 && !r.isOver(); ++t) {
            int threat[2] = {99, 99};   // rows until a car in that lane reaches the player
            for (int i = 0; i < Racing::kMaxCars; ++i) {
                const auto& c = r.cars()[i];
                if (!c.active || c.y > Racing::kPlayerY + Racing::kCarH) continue;
                const int d = Racing::kPlayerY - (c.y + Racing::kCarH);
                if (d < threat[c.lane]) threat[c.lane] = d;
            }
            const uint8_t want = threat[1] > threat[0] ? 1 : 0;
            if (threat[r.lane()] < 3 && want != r.lane())
                r.onButton(want ? Button::Right : Button::Left, ButtonEvent::Press, 0);
            r.debugTick();
        }
        CHECK(!r.isOver());
        CHECK(r.passed() > 200);
        CHECK(r.level() >= 1);
    }
}

// ---------------------------------------------------------------- menu with all games

static void testMenuThreeGames() {
    FakePlatform p;
    p.nvs["hi_snake"] = 77;
    Tetris t;
    Snake s;
    Racing r;
    Game* games[] = {&t, &s, &r};
    App app(p, games, 3);
    app.begin(0);
    Frame f;
    app.draw(f);
    CHECK(std::string(f.status) == "TETRIS");
    app.onButton(Button::Down, ButtonEvent::Press, 0);
    app.draw(f);
    CHECK(std::string(f.status) == "SNAKE");
    CHECK(f.hud.hiScore == 77);
    app.onButton(Button::Down, ButtonEvent::Press, 0);
    app.draw(f);
    CHECK(std::string(f.status) == "RACING");
    app.onButton(Button::Down, ButtonEvent::Press, 0);   // wraps
    app.draw(f);
    CHECK(std::string(f.status) == "TETRIS");
    app.onButton(Button::Up, ButtonEvent::Press, 0);     // wraps back
    app.draw(f);
    CHECK(std::string(f.status) == "RACING");

    // Start Racing and crash it: its own hi-score key is written.
    app.onButton(Button::Start, ButtonEvent::Press, 0);
    CHECK(app.state() == App::State::Playing);
    uint32_t now = 0;
    for (int i = 0; i < 20000 && app.state() == App::State::Playing; ++i) {
        now += 50;
        app.update(now);
    }
    CHECK(app.state() == App::State::Curtain);
    CHECK(p.nvs.count("hi_racing") == (r.score() > 0 ? 1u : 0u));
}

static bool overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void testLayouts() {
    for (const Layout* L : {&kClassicLayout, &kTouchLayout}) {
        const int fw = Field::kWidth * L->cell, fh = Field::kHeight * L->cell;
        // Field + its 2 px border fit on the 240×320 screen and stay left of the panel.
        CHECK(L->fieldX - 2 >= 0 && L->fieldY - 2 >= 0);
        CHECK(L->fieldX + fw + 2 <= L->panelX);
        CHECK(L->fieldY + fh + 2 <= L->statusY);
        CHECK(L->statusY + 16 <= 320);
        CHECK(L->panelRight < 240);
        CHECK(L->nextGridX + 4 * L->cell <= L->panelRight + 1);
    }
    // Touch buttons: on screen, below the status line, never overlapping each other.
    for (int i = 0; i < kTouchButtonCount; ++i) {
        const auto& a = kTouchButtons[i];
        CHECK(a.x >= 0 && a.y >= 0 && a.x + a.w <= 240 && a.y + a.h <= 320);
        CHECK(a.y >= kTouchLayout.statusY + 16);
        for (int j = i + 1; j < kTouchButtonCount; ++j) {
            const auto& b = kTouchButtons[j];
            CHECK(!overlaps(a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h));
        }
    }
    // Every logical button has exactly one on-screen button.
    uint8_t seen = 0;
    for (const auto& tb : kTouchButtons) {
        CHECK(!(seen & buttonBit(tb.button)));
        seen |= buttonBit(tb.button);
    }
    CHECK(seen == 0xFF);

    // The D-pad zone must not overlap the other on-screen buttons.
    for (const auto& tb : kTouchButtons)
        if (tb.shape != TouchButton::Arrow)
            CHECK(!overlaps(kDpad.x0, kDpad.y0, kDpad.x1 - kDpad.x0, kDpad.y1 - kDpad.y0,
                            tb.x - kTouchHitPad, tb.y - kTouchHitPad, tb.w + 2 * kTouchHitPad, tb.h + 2 * kTouchHitPad));

    // Every drawn button is hit at its own centre.
    for (const auto& tb : kTouchButtons) CHECK(touchHit(tb.x + tb.w / 2, tb.y + tb.h / 2) == buttonBit(tb.button));

    // Real taps logged on the ES3C28P (2026-11-08) that used to miss: centre-of-pad and
    // between DOWN and RIGHT. They must now resolve to the intended direction.
    struct Tap { int x, y; Button want; };
    const Tap taps[] = {
        {43, 280, Button::Left}, {44, 271, Button::Left},
        {74, 310, Button::Down}, {74, 314, Button::Down}, {76, 312, Button::Down}, {76, 310, Button::Down},
        {74, 300, Button::Down}, {79, 302, Button::Down}, {84, 309, Button::Down}, {72, 296, Button::Down},
        {72, 301, Button::Down},
        // and taps that already worked keep working
        {91, 278, Button::Right}, {28, 265, Button::Left}, {69, 312, Button::Down},
        {120, 281, Button::Start}, {216, 251, Button::A}, {175, 288, Button::B}, {239, 247, Button::A},
    };
    for (const auto& t : taps) {
        const bool ok = touchHit(t.x, t.y) == buttonBit(t.want);
        if (!ok) std::printf("  tap (%d,%d) -> 0x%02X\n", t.x, t.y, touchHit(t.x, t.y));
        CHECK(ok);
    }
    CHECK(touchHit(kDpad.cx, kDpad.cy) == 0);   // dead centre is neutral
    CHECK(touchHit(60, 100) == 0);              // playfield is not a button
}

int main() {
    testLayouts();
    testShapes();
    testDropInterval();
    testBag();
    testGravityAndLock();
    testWallsAndKick();
    testLineClearAndScore();
    testLevelUp();
    testGameOver();
    testApp();
    testHiScorePersist();
    testFont();
    testSnakeBasics();
    testSnakeEatAndGrow();
    testSnakeDeath();
    testRacingBasics();
    testRacingCrash();
    testRacingAlwaysPassable();
    testMenuThreeGames();
    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
