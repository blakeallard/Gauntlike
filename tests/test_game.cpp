#include <catch2/catch_test_macros.hpp>

#include "game.hpp"

using namespace gl;

namespace {

// 12x7 arena: open room, door at (6,3) in an inner wall, exit at (10,3).
constexpr std::string_view kArena[] = {
    "############",
    "#....#.....#",
    "#....#.....#",
    "#....+....>#",
    "#....#.....#",
    "#....#.....#",
    "############",
};

FloorPlan arena_plan() {
    FloorPlan p;
    p.map = Map::from_ascii(kArena);
    p.start = {2, 3};
    p.exit = {10, 3};
    p.key = {1, 1};
    return p;
}

Game make_game(FloorPlan plan, PlayerClass cls = PlayerClass::Valkyrie) {
    return Game(cls, tae::Rng(42), std::move(plan));
}

void idle_ticks(Game& g, int n) {
    for (int i = 0; i < n; ++i) g.tick({});
}

// Step in direction d, waiting out the move cooldown.
void walk(Game& g, tae::Dir d, int steps = 1) {
    for (int i = 0; i < steps; ++i) {
        Pos before = g.player();
        for (int t = 0; t < 12 && g.player() == before; ++t) {
            g.tick({.move = d});
        }
    }
}

}  // namespace

TEST_CASE("player movement respects walls and cooldown") {
    Game g = make_game(arena_plan());
    CHECK(g.player() == Pos{2, 3});

    // One tick with move input: exactly one step, then the cooldown blocks.
    g.tick({.move = tae::Dir{1, 0}});
    CHECK(g.player() == Pos{3, 3});
    g.tick({.move = tae::Dir{1, 0}});
    CHECK(g.player() == Pos{3, 3});  // still cooling down

    // Walls block: walk up into the top wall.
    walk(g, {0, -1}, 4);
    CHECK(g.player().y == 1);
    walk(g, {0, -1});
    CHECK(g.player().y == 1);
}

TEST_CASE("doors open on bump, then are walkable") {
    Game g = make_game(arena_plan());
    walk(g, {1, 0}, 2);  // (2,3) -> (4,3), door ahead at (5,3)
    CHECK(g.player() == Pos{4, 3});
    REQUIRE(g.map().at(5, 3) == Tile::Door);

    // Bumping opens the door but does not move the player.
    Pos before = g.player();
    for (int i = 0; i < 12 && g.map().at(5, 3) == Tile::Door; ++i) {
        g.tick({.move = tae::Dir{1, 0}});
    }
    CHECK(g.map().at(5, 3) == Tile::DoorOpen);
    CHECK(g.player() == before);

    walk(g, {1, 0});
    CHECK(g.player() == Pos{5, 3});
}

TEST_CASE("exit refuses without a key") {
    Game g = make_game(arena_plan());
    walk(g, {1, 0}, 7);  // through the door, ends at (9,3) next to the exit
    REQUIRE(g.player() == Pos{9, 3});
    walk(g, {1, 0});
    CHECK(g.floor() == 1);
    CHECK(g.player() == Pos{9, 3});
}

TEST_CASE("key unlocks the exit and descends to a new floor") {
    Game g = make_game(arena_plan());

    // Grab the key at (1,1).
    walk(g, {-1, 0});
    walk(g, {0, -1}, 2);
    REQUIRE(g.player() == Pos{1, 1});
    CHECK(g.keys() == 1);

    // Head to the exit at (10,3): down-right to the door row, then east.
    walk(g, {1, 1}, 2);
    walk(g, {1, 0}, 20);  // bumps the door open along the way, then exits
    CHECK(g.floor() == 2);
    CHECK(g.score() >= 250);
    CHECK(g.keys() == 0);
    CHECK(g.map().width() == 12);  // regenerated at the same size
}

TEST_CASE("pickups: food heals after drain, treasure scores, potion stocks") {
    FloorPlan p = arena_plan();
    p.food = {{3, 3}};
    p.treasure = {{4, 3}};
    p.potions = {{2, 2}};
    Game g = make_game(p);

    idle_ticks(g, Game::kDrainPeriod * 20);  // drain 20 hp
    int drained = g.hp();
    CHECK(drained < stats_for(PlayerClass::Valkyrie).max_hp);

    walk(g, {1, 0});  // food
    CHECK(g.hp() > drained);
    walk(g, {1, 0});  // treasure
    CHECK(g.score() == 50);
    CHECK(g.potions() == 0);
    walk(g, {-1, -1});  // (4,3) -> (3,2)
    walk(g, {-1, 0});   // -> (2,2), the potion
    CHECK(g.potions() == 1);
}

TEST_CASE("health drains over time and kills the player at zero") {
    FloorPlan p = arena_plan();
    Game g = make_game(p, PlayerClass::Wizard);  // 500 hp
    int max = stats_for(PlayerClass::Wizard).max_hp;
    idle_ticks(g, Game::kDrainPeriod);
    CHECK(g.hp() == max - 1);

    idle_ticks(g, Game::kDrainPeriod * (max + 5));
    CHECK(g.dead());
    // Ticks after death are no-ops.
    int hp_at_death = g.hp();
    idle_ticks(g, 50);
    CHECK(g.hp() == hp_at_death);
}

TEST_CASE("melee bump damages and destroys a spawner for the bounty") {
    FloorPlan p = arena_plan();
    p.spawners = {{3, 3}};  // right next to the player start
    Game g = make_game(p, PlayerClass::Warrior);  // melee 40; spawner hp 60

    // First bump: 40 damage, spawner survives, player does not move.
    g.tick({.move = tae::Dir{1, 0}});
    CHECK(g.score() == 0);
    CHECK(g.player() == Pos{2, 3});

    // Wait out the move cooldown, then the second bump kills it.
    for (int i = 0; i < 12 && g.score() == 0; ++i) {
        g.tick({.move = tae::Dir{1, 0}});
    }
    CHECK(g.score() == 100);  // spawner bounty
}

TEST_CASE("player projectiles fly, hit and kill") {
    FloorPlan p = arena_plan();
    p.spawners = {{4, 1}};
    Game g = make_game(p, PlayerClass::Wizard);  // shot dmg 35; spawner hp 60

    // Stand below the spawner at (4,2), facing up.
    walk(g, {1, 0}, 2);
    REQUIRE(g.player() == Pos{4, 3});
    walk(g, {0, -1});
    REQUIRE(g.player() == Pos{4, 2});

    // Two hits needed; fire until the bounty lands.
    Game::Input fire_in;
    fire_in.fire = true;
    for (int i = 0; i < 60 && g.score() < 100; ++i) {
        g.tick(fire_in);
    }
    CHECK(g.score() >= 100);
    bool spawner_alive = false;
    for (const Entity& e : g.entities()) {
        if (e.kind == EntityKind::Spawner) spawner_alive = true;
    }
    CHECK_FALSE(spawner_alive);
}

TEST_CASE("spawners emit enemies that chase the player") {
    FloorPlan p = arena_plan();
    p.spawners = {{9, 1}};
    Game g = make_game(p);

    // Wait for the spawner to emit (cd is randomized up to the period).
    bool spawned = false;
    for (int i = 0; i < 200 && !spawned; ++i) {
        g.tick({});
        for (const Entity& e : g.entities()) {
            if (is_enemy(e.kind)) spawned = true;
        }
    }
    REQUIRE(spawned);

    // The grunt should close in on the (stationary) player over time.
    auto enemy_dist = [&]() {
        for (const Entity& e : g.entities()) {
            if (is_enemy(e.kind)) {
                return std::max(std::abs(e.pos.x - g.player().x),
                                std::abs(e.pos.y - g.player().y));
            }
        }
        return 999;
    };
    int d0 = enemy_dist();
    int hp0 = g.hp();
    idle_ticks(g, 150);
    // Either it reached us (and is biting) or it got closer.
    CHECK((enemy_dist() < d0 || g.hp() < hp0 - 150 / Game::kDrainPeriod));
}

TEST_CASE("potion is a smart bomb") {
    FloorPlan p = arena_plan();
    p.potions = {{3, 3}};
    p.spawners = {{9, 1}};
    Game g = make_game(p);
    walk(g, {1, 0});  // grab potion
    REQUIRE(g.potions() == 1);

    // Let some enemies accumulate.
    idle_ticks(g, 250);
    int enemies = 0;
    for (const Entity& e : g.entities()) {
        if (is_enemy(e.kind)) ++enemies;
    }
    REQUIRE(enemies > 0);

    Game::Input potion_in;
    potion_in.potion = true;
    g.tick(potion_in);
    int enemies_after = 0;
    for (const Entity& e : g.entities()) {
        if (is_enemy(e.kind)) ++enemies_after;
    }
    CHECK(enemies_after == 0);
    CHECK(g.potions() == 0);
}
