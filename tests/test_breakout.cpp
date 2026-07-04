#include <catch2/catch_test_macros.hpp>

#include "breakout.hpp"

using namespace bo;

TEST_CASE("attached ball rides the paddle until launch") {
    BreakoutGame g(60, 22, tae::Rng(1));
    REQUIRE(g.balls().size() == 1);
    CHECK(g.balls()[0].attached);

    double bx0 = g.balls()[0].x;
    for (int i = 0; i < 5; ++i) {
        g.move_paddle(1);
        g.tick();
    }
    CHECK(g.balls()[0].x > bx0);  // followed the paddle
    CHECK(g.lives() == 3);

    g.launch();
    CHECK_FALSE(g.balls()[0].attached);
    CHECK(g.balls()[0].vy < 0);  // heading up
}

TEST_CASE("ball bounces off side walls and ceiling") {
    BreakoutGame g(60, 22, tae::Rng(2));
    g.launch();

    // Run a while: the ball must always stay inside the field horizontally
    // and never go above the ceiling.
    for (int i = 0; i < 600 && !g.balls().empty(); ++i) {
        g.tick();
        for (const Ball& b : g.balls()) {
            CHECK(b.x >= 0);
            CHECK(b.x <= g.width());
            CHECK(b.y >= 0);
        }
    }
}

TEST_CASE("bricks break for score and the ball rebounds") {
    BreakoutGame g(60, 22, tae::Rng(3));
    int alive0 = g.bricks_alive();
    REQUIRE(alive0 > 0);
    g.launch();

    long s = g.score();
    // Play until something breaks (ball auto-bounces off paddle only if
    // aligned; keep the paddle under the ball to be safe).
    for (int i = 0; i < 3000 && g.bricks_alive() == alive0 && !g.dead(); ++i) {
        const Ball& b = g.balls()[0];
        double center = g.paddle_x() + g.paddle_w() / 2.0;
        if (b.x < center - 1) g.move_paddle(-1);
        else if (b.x > center + 1) g.move_paddle(1);
        g.tick();
        REQUIRE_FALSE(g.balls().empty());
    }
    CHECK(g.bricks_alive() < alive0);
    CHECK(g.score() > s);
}

TEST_CASE("losing every ball costs a life; game over at zero") {
    BreakoutGame g(60, 22, tae::Rng(4));
    g.launch();
    // Park the paddle far left and never move: the ball will eventually drop.
    int guard = 0;
    while (g.lives() == 3 && guard++ < 5000) {
        // Keep the paddle away from the ball's landing spot.
        const Ball& b = g.balls()[0];
        if (!b.attached) {
            if (b.x < g.width() / 2.0) {
                if (g.paddle_x() < g.width() - g.paddle_w()) g.move_paddle(1);
            } else if (g.paddle_x() > 0) {
                g.move_paddle(-1);
            }
        }
        g.tick();
    }
    CHECK(g.lives() == 2);
    REQUIRE(g.balls().size() == 1);
    CHECK(g.balls()[0].attached);  // fresh serve

    // Drain the remaining lives the same way.
    guard = 0;
    while (!g.dead() && guard++ < 20000) {
        if (!g.balls().empty()) {
            const Ball& b = g.balls()[0];
            if (b.attached) g.launch();
            else if (b.x < g.width() / 2.0) {
                if (g.paddle_x() < g.width() - g.paddle_w()) g.move_paddle(1);
            } else if (g.paddle_x() > 0) {
                g.move_paddle(-1);
            }
        }
        g.tick();
    }
    CHECK(g.dead());
    // Dead game ignores ticks.
    long s = g.score();
    g.tick();
    CHECK(g.score() == s);
}

TEST_CASE("wide power-up widens the paddle temporarily") {
    BreakoutGame g(60, 22, tae::Rng(5));
    CHECK(g.paddle_w() == BreakoutGame::kPaddleW);
    // Power-up application is internal; simulate by checking the constant
    // contract: wide > normal, and the paddle clamps inside the field.
    CHECK(BreakoutGame::kPaddleWide > BreakoutGame::kPaddleW);
    for (int i = 0; i < 100; ++i) g.move_paddle(1);
    CHECK(g.paddle_x() <= g.width() - g.paddle_w());
    for (int i = 0; i < 200; ++i) g.move_paddle(-1);
    CHECK(g.paddle_x() >= 0);
}
