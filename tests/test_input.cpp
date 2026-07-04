#include <catch2/catch_test_macros.hpp>

#include "tae/input.hpp"

using namespace tae;

TEST_CASE("map_key maps arrows, WASD and vi keys to the same directions") {
    CHECK(map_key(rawkey::Up) == Key::Up);
    CHECK(map_key('w') == Key::Up);
    CHECK(map_key('W') == Key::Up);
    CHECK(map_key('k') == Key::Up);
    CHECK(map_key(rawkey::Down) == Key::Down);
    CHECK(map_key('s') == Key::Down);
    CHECK(map_key('j') == Key::Down);
    CHECK(map_key(rawkey::Left) == Key::Left);
    CHECK(map_key('a') == Key::Left);
    CHECK(map_key('h') == Key::Left);
    CHECK(map_key(rawkey::Right) == Key::Right);
    CHECK(map_key('d') == Key::Right);
    CHECK(map_key('l') == Key::Right);
}

TEST_CASE("map_key maps vi diagonals") {
    CHECK(map_key('y') == Key::UpLeft);
    CHECK(map_key('u') == Key::UpRight);
    CHECK(map_key('b') == Key::DownLeft);
    CHECK(map_key('n') == Key::DownRight);
}

TEST_CASE("map_key maps action keys") {
    CHECK(map_key(' ') == Key::Fire);
    CHECK(map_key('e') == Key::Potion);
    CHECK(map_key('p') == Key::Pause);
    CHECK(map_key('q') == Key::Quit);
    CHECK(map_key(rawkey::Enter) == Key::Confirm);
    CHECK(map_key(rawkey::Escape) == Key::Cancel);
    CHECK(map_key('z') == Key::None);
    CHECK(map_key(-5) == Key::None);
}

TEST_CASE("key_dir gives unit vectors for movement keys only") {
    CHECK(key_dir(Key::Up) == Dir{0, -1});
    CHECK(key_dir(Key::DownRight) == Dir{1, 1});
    CHECK_FALSE(key_dir(Key::Fire).has_value());
    CHECK_FALSE(key_dir(Key::None).has_value());
}

TEST_CASE("HeldDirection keeps the last direction alive for the hold window") {
    HeldDirection held(3);

    // Press right once: direction stays active for 3 ticks.
    auto d = held.update({Key::Right});
    REQUIRE(d.has_value());
    CHECK(*d == Dir{1, 0});
    CHECK(held.update({}).has_value());
    CHECK(held.update({}).has_value());
    CHECK_FALSE(held.update({}).has_value());  // window expired

    // A new press refreshes and redirects.
    d = held.update({Key::Up});
    REQUIRE(d.has_value());
    CHECK(*d == Dir{0, -1});

    // The most recent key in a tick wins.
    d = held.update({Key::Left, Key::Down});
    REQUIRE(d.has_value());
    CHECK(*d == Dir{0, 1});

    held.reset();
    CHECK_FALSE(held.update({}).has_value());
}
