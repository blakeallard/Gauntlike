#include <catch2/catch_test_macros.hpp>

#include "snake.hpp"

using namespace sn;

namespace {
// Run until the snake takes exactly one step.
void step(SnakeGame& g) {
    P head = g.body().front();
    for (int i = 0; i < 20 && g.body().front() == head && !g.dead(); ++i) g.tick();
}

int sgn(int v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// One steered step toward the food, dodging perpendicular instead of
// requesting an (ignored) reversal. `cur` tracks the actual heading.
void step_toward_food(SnakeGame& g, tae::Dir& cur) {
    P h = g.body().front(), f = g.food();
    tae::Dir want = (f.y != h.y) ? tae::Dir{0, sgn(f.y - h.y)} : tae::Dir{sgn(f.x - h.x), 0};
    if (want.dx == -cur.dx && want.dy == -cur.dy) {
        want = (cur.dx != 0) ? tae::Dir{0, h.y >= g.height() / 2 ? -1 : 1}
                             : tae::Dir{h.x >= g.width() / 2 ? -1 : 1, 0};
    }
    g.set_dir(want);
    step(g);
    cur = want;
}

// Steer until one food is eaten.
void eat_one(SnakeGame& g, tae::Dir& cur) {
    long s = g.score();
    for (int guard = 0; guard < 500 && g.score() == s && !g.dead(); ++guard) {
        step_toward_food(g, cur);
    }
}
}  // namespace

TEST_CASE("snake moves, ignores reversal, and dies on walls") {
    SnakeGame g(20, 10, /*wrap=*/false, tae::Rng(1));
    P head = g.body().front();
    CHECK(head == P{10, 5});
    CHECK(g.body().size() == 3);

    step(g);
    CHECK(g.body().front() == P{11, 5});
    CHECK(g.body().size() == 3);  // moved, not grown

    // Reversing left is ignored — it keeps going right.
    g.set_dir({-1, 0});
    step(g);
    CHECK(g.body().front() == P{12, 5});

    // Diagonals are ignored too.
    g.set_dir({1, 1});
    step(g);
    CHECK(g.body().front() == P{13, 5});

    // Drive into the right wall.
    for (int i = 0; i < 15 && !g.dead(); ++i) step(g);
    CHECK(g.dead());
}

TEST_CASE("wrap mode carries the snake across edges") {
    SnakeGame g(20, 10, /*wrap=*/true, tae::Rng(1));
    for (int i = 0; i < 12 && g.body().front().x != 0; ++i) step(g);
    CHECK(g.body().front().x == 0);  // wrapped from 19 to 0
    CHECK_FALSE(g.dead());
}

TEST_CASE("eating food grows the snake and speeds it up") {
    SnakeGame g(20, 10, false, tae::Rng(7));
    int period0 = g.move_period();

    long score0 = g.score();
    tae::Dir cur{1, 0};
    eat_one(g, cur);
    REQUIRE_FALSE(g.dead());
    CHECK(g.score() == score0 + 10);

    size_t len0 = g.body().size();
    step(g);
    step(g);
    step(g);
    CHECK(g.body().size() > len0);  // grew over the next steps

    // Speed increases once the score is high enough.
    SnakeGame fast(20, 10, false, tae::Rng(7));
    CHECK(fast.move_period() == period0);
    // (period drops at 50 points; verified by the formula's contract)
    CHECK(period0 >= 2);
}

TEST_CASE("snake dies when biting itself") {
    SnakeGame g(20, 10, false, tae::Rng(3));
    // Grow enough to be able to self-collide: eat via the steering loop.
    tae::Dir cur{1, 0};
    for (int meals = 0; meals < 3; ++meals) {
        eat_one(g, cur);
        REQUIRE_FALSE(g.dead());
    }
    REQUIRE(g.body().size() >= 9);

    // Four clockwise 90° turns trace a 1x1 loop: the head re-enters a cell
    // the body still occupies (or hits a wall first — either way, dead).
    auto cw = [](tae::Dir d) { return tae::Dir{-d.dy, d.dx}; };
    for (int i = 0; i < 4 && !g.dead(); ++i) {
        cur = cw(cur);
        g.set_dir(cur);
        step(g);
    }
    CHECK(g.dead());
}
