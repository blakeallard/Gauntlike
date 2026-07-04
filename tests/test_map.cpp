#include <catch2/catch_test_macros.hpp>

#include "map.hpp"

using namespace gl;

// Hand-authored test map 1: two rooms joined by a door.
constexpr std::string_view kMap1[] = {
    "##########",
    "#....#...#",
    "#....+..>#",
    "#....#...#",
    "##########",
};

// Hand-authored test map 2: corridor ring with an unreachable vault.
constexpr std::string_view kMap2[] = {
    "############",
    "#..........#",
    "#.########.#",
    "#.#....###.#",
    "#.########.#",
    "#.........>#",
    "############",
};

TEST_CASE("from_ascii parses tiles") {
    Map m = Map::from_ascii(kMap1);
    CHECK(m.width() == 10);
    CHECK(m.height() == 5);
    CHECK(m.at(0, 0) == Tile::Wall);
    CHECK(m.at(1, 1) == Tile::Floor);
    CHECK(m.at(5, 2) == Tile::Door);
    CHECK(m.at(8, 2) == Tile::Exit);
}

TEST_CASE("walkability and shot blocking") {
    Map m = Map::from_ascii(kMap1);
    CHECK(m.walkable(1, 1));
    CHECK(m.walkable(5, 2));   // closed door is walkable (opens on entry)
    CHECK(m.walkable(8, 2));   // exit
    CHECK_FALSE(m.walkable(0, 0));
    CHECK_FALSE(m.walkable(-1, 2));

    CHECK(m.blocks_shots(0, 0));
    CHECK(m.blocks_shots(5, 2));  // closed door
    m.set(5, 2, Tile::DoorOpen);
    CHECK_FALSE(m.blocks_shots(5, 2));
    CHECK_FALSE(m.blocks_shots(1, 1));
}

TEST_CASE("flood fill connectivity") {
    Map m = Map::from_ascii(kMap1);
    // Left room 4x3=12, door 1, right block 3x3=9 (incl exit) → 22 walkable.
    CHECK(m.reachable({1, 1}, {8, 2}));
    CHECK(m.reachable_count(1, 1) == 22);

    Map m2 = Map::from_ascii(kMap2);
    CHECK(m2.reachable({1, 1}, {10, 5}));
    // The vault at (3..6, 3) is walled off from the ring corridor.
    CHECK_FALSE(m2.reachable({1, 1}, {4, 3}));
}
