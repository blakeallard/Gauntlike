#include <catch2/catch_test_macros.hpp>

#include "mapgen.hpp"

using namespace gl;

namespace {
void check_plan(const FloorPlan& p, int floor) {
    const Map& m = p.map;

    // Player start, key and exit are placed on walkable tiles and connected.
    REQUIRE(m.walkable(p.start.x, p.start.y));
    REQUIRE(m.at(p.exit.x, p.exit.y) == Tile::Exit);
    CHECK(m.reachable(p.start, p.exit));
    CHECK(m.reachable(p.start, p.key));

    // Every walkable tile is reachable from the start (fully connected).
    int total = 0;
    for (int y = 0; y < m.height(); ++y) {
        for (int x = 0; x < m.width(); ++x) {
            if (m.walkable(x, y)) ++total;
        }
    }
    CHECK(m.reachable_count(p.start.x, p.start.y) == total);

    // Item placement contracts.
    size_t expected_spawners = std::min(2 + (floor - 1) / 2, 4);
    CHECK(p.spawners.size() >= 2);
    CHECK(p.spawners.size() <= expected_spawners);
    CHECK(p.food.size() >= 1);
    CHECK(p.food.size() <= 4);
    CHECK(p.treasure.size() <= 5);
    CHECK(p.potions.size() <= 2);
    for (const Pos& s : p.spawners) {
        CHECK(m.at(s.x, s.y) == Tile::Floor);
        CHECK(m.reachable(p.start, s));
        CHECK_FALSE(s == p.start);
    }
    for (const Pos& f : p.food) CHECK(m.reachable(p.start, f));
    for (const Pos& t : p.potions) CHECK(m.reachable(p.start, t));
}
}  // namespace

TEST_CASE("generated floors are connected with valid placement across seeds") {
    for (std::uint32_t seed : {1u, 2u, 42u, 1337u, 99999u}) {
        tae::Rng rng(seed);
        for (int floor = 1; floor <= 6; ++floor) {
            FloorPlan p = generate_floor(rng, 80, 21, floor);
            check_plan(p, floor);
        }
    }
}

TEST_CASE("same seed generates the same floor (reproducible dungeons)") {
    tae::Rng a(12345), b(12345);
    FloorPlan pa = generate_floor(a, 80, 21, 1);
    FloorPlan pb = generate_floor(b, 80, 21, 1);
    CHECK(pa.start == pb.start);
    CHECK(pa.exit == pb.exit);
    CHECK(pa.key == pb.key);
    REQUIRE(pa.spawners.size() == pb.spawners.size());
    for (size_t i = 0; i < pa.spawners.size(); ++i) CHECK(pa.spawners[i] == pb.spawners[i]);
    for (int y = 0; y < pa.map.height(); ++y) {
        for (int x = 0; x < pa.map.width(); ++x) {
            REQUIRE(pa.map.at(x, y) == pb.map.at(x, y));
        }
    }
}

TEST_CASE("bigger terminals generate bigger floors") {
    tae::Rng rng(7);
    FloorPlan p = generate_floor(rng, 120, 40, 1);
    CHECK(p.map.width() == 120);
    CHECK(p.map.height() == 40);
    check_plan(p, 1);
}
