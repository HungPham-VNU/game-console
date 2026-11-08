// Host tests for core/ + games/ (no Arduino, no display). Run:  make -C test_host
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "../src/core/App.h"
#include "../src/core/Font3x5.h"
#include "../src/games/Tetris.h"

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

int main() {
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
    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
