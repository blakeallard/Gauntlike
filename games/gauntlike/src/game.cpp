#include "game.hpp"

#include <algorithm>
#include <cstdlib>
#include <queue>

namespace gl {

const ClassStats& stats_for(PlayerClass c) {
    static constexpr ClassStats kStats[] = {
        {"WARRIOR", 800, 40, 12, 3, 12},   // melee monster, high HP
        {"WIZARD", 500, 12, 35, 3, 7},     // glass cannon
        {"ELF", 600, 18, 18, 2, 8},        // fast
        {"VALKYRIE", 700, 25, 20, 3, 9},   // balanced
    };
    return kStats[static_cast<int>(c)];
}

bool is_enemy(EntityKind k) {
    return k == EntityKind::Grunt || k == EntityKind::Ghost || k == EntityKind::Demon ||
           k == EntityKind::Lobber;
}

bool is_pickup(EntityKind k) {
    return k == EntityKind::Food || k == EntityKind::Treasure ||
           k == EntityKind::PotionPickup || k == EntityKind::KeyPickup;
}

EnemyStats enemy_stats(EntityKind k, int floor) {
    EnemyStats s{};
    switch (k) {
        case EntityKind::Grunt: s = {20, 8, 6, 20, 10}; break;
        case EntityKind::Ghost: s = {14, 12, 5, 25, 15}; break;
        case EntityKind::Demon: s = {40, 18, 8, 30, 25}; break;
        case EntityKind::Lobber: s = {22, 10, 9, 30, 20}; break;
        default: s = {1, 0, 1, 1, 0}; break;
    }
    // +15% hp and +1 damage per floor past the first.
    s.hp = s.hp + s.hp * 15 * (floor - 1) / 100;
    s.dmg += floor - 1;
    return s;
}

namespace {

EntityKind pick_spawner_kind(tae::Rng& rng, int floor) {
    // Deeper floors unlock nastier spawners.
    std::vector<EntityKind> pool = {EntityKind::Grunt};
    if (floor >= 2) pool.push_back(EntityKind::Ghost);
    if (floor >= 3) pool.push_back(EntityKind::Demon);
    if (floor >= 4) pool.push_back(EntityKind::Lobber);
    return pool[static_cast<size_t>(rng.range(0, static_cast<int>(pool.size()) - 1))];
}

int spawner_period(int floor) {
    int p = 90 - (floor - 1) * 8;
    return std::max(p, 40);
}

}  // namespace

Game::Game(PlayerClass cls, tae::Rng rng, int map_w, int map_h)
    : cls_(cls), stats_(stats_for(cls)), rng_(rng), seed_(rng.seed()),
      map_w_(map_w), map_h_(map_h) {
    hp_ = stats_.max_hp;
    load_plan(generate_floor(rng_, map_w_, map_h_, floor_));
    say(std::string(stats_.name) + " enters the dungeon! (seed " + std::to_string(seed_) + ")");
}

Game::Game(PlayerClass cls, tae::Rng rng, FloorPlan plan)
    : cls_(cls), stats_(stats_for(cls)), rng_(rng), seed_(rng.seed()),
      map_w_(plan.map.width()), map_h_(plan.map.height()) {
    hp_ = stats_.max_hp;
    load_plan(plan);
}

void Game::load_plan(const FloorPlan& plan) {
    map_ = plan.map;
    player_ = plan.start;
    entities_.clear();
    projectiles_.clear();
    dist_age_ = 1 << 20;
    move_cd_ = shot_cd_ = 0;

    auto add = [&](EntityKind kind, Pos p) -> Entity& {
        Entity e;
        e.id = next_id_++;
        e.kind = kind;
        e.pos = p;
        entities_.push_back(e);
        return entities_.back();
    };

    for (Pos p : plan.spawners) {
        Entity& s = add(EntityKind::Spawner, p);
        s.hp = 60 + 10 * (floor_ - 1);
        s.spawn_kind = pick_spawner_kind(rng_, floor_);
        s.spawn_cd = rng_.range(20, spawner_period(floor_));
    }
    for (Pos p : plan.food) add(EntityKind::Food, p);
    for (Pos p : plan.treasure) add(EntityKind::Treasure, p);
    for (Pos p : plan.potions) add(EntityKind::PotionPickup, p);
    add(EntityKind::KeyPickup, plan.key);
}

void Game::say(std::string msg) {
    messages_.push_back(std::move(msg));
    while (messages_.size() > 6) messages_.pop_front();
}

void Game::tick(const Input& in) {
    if (dead_) return;
    ++ticks_;
    ++dist_age_;
    if (shake_ > 0) --shake_;
    if (move_cd_ > 0) --move_cd_;
    if (shot_cd_ > 0) --shot_cd_;

    if (in.potion) use_potion();
    if (in.move) {
        facing_ = *in.move;  // aim even while the step cooldown runs
        if (move_cd_ == 0) try_move(*in.move);
    }
    if (in.fire && shot_cd_ == 0) fire();

    step_projectiles();
    step_spawners();
    step_enemies();
    health_drain();
    remove_dead_entities();

    if (hp_ <= 0 && !dead_) {
        dead_ = true;
        say(std::string(stats_.name) + " has died on floor " + std::to_string(floor_) + "...");
    }
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

void Game::try_move(tae::Dir d) {
    Pos to{player_.x + d.dx, player_.y + d.dy};
    if (!map_.in_bounds(to.x, to.y)) return;

    // Bump attack: melee anything solid standing there.
    if (Entity* e = solid_entity_at(to)) {
        damage_enemy(*e, stats_.melee_dmg);
        move_cd_ = stats_.move_period;
        return;
    }

    Tile t = map_.at(to.x, to.y);
    if (t == Tile::Wall) return;

    if (t == Tile::Door) {
        map_.set(to.x, to.y, Tile::DoorOpen);
        say("The door creaks open.");
        move_cd_ = stats_.move_period;
        return;  // opening the door takes the step
    }

    if (t == Tile::Exit) {
        if (keys_ > 0) {
            --keys_;
            descend();
        } else {
            say("The exit is locked — find the key!");
        }
        move_cd_ = stats_.move_period;
        return;
    }

    player_ = to;
    move_cd_ = stats_.move_period;
    dist_age_ = 1 << 20;  // enemies re-path toward the new position

    if (Entity* p = pickup_at(player_)) pick_up(*p);
}

void Game::fire() {
    // Spawn on the player's own cell; the same-tick projectile step moves it
    // one cell out and runs the collision check there, so a point-blank
    // target is hit rather than tunneled through.
    projectiles_.push_back(Projectile{player_, facing_, stats_.shot_dmg, true});
    shot_cd_ = stats_.shot_period;
}

void Game::use_potion() {
    if (potions_ <= 0) return;
    --potions_;
    int kills = 0;
    for (Entity& e : entities_) {
        if (is_enemy(e.kind)) {
            e.hp -= 150;
            if (e.hp <= 0) ++kills;
        } else if (e.kind == EntityKind::Spawner) {
            e.hp -= 100;
        }
    }
    shake_ = 3;
    say("The potion detonates! " + std::to_string(kills) + " enemies vaporized.");
}

void Game::pick_up(Entity& e) {
    switch (e.kind) {
        case EntityKind::Food:
            hp_ = std::min(hp_ + kFoodValue, stats_.max_hp);
            warned_low_hp_ = false;
            say("Food. Delicious.");
            break;
        case EntityKind::Treasure:
            score_ += 50;
            say("Treasure! +50");
            break;
        case EntityKind::PotionPickup:
            ++potions_;
            say("Picked up a potion. (e to drink)");
            break;
        case EntityKind::KeyPickup:
            ++keys_;
            say("Found a key!");
            break;
        default:
            return;
    }
    e.hp = 0;  // consumed; culled at end of tick
}

void Game::hurt_player(int dmg) {
    hp_ -= dmg;
    shake_ = 2;
}

void Game::descend() {
    ++floor_;
    score_ += 250;
    load_plan(generate_floor(rng_, map_w_, map_h_, floor_));
    say("You descend... floor " + std::to_string(floor_) + "!");
}

// ---------------------------------------------------------------------------
// World stepping
// ---------------------------------------------------------------------------

void Game::step_projectiles() {
    for (auto& pr : projectiles_) {
        pr.pos.x += pr.dir.dx;
        pr.pos.y += pr.dir.dy;
    }
    std::erase_if(projectiles_, [&](Projectile& pr) {
        if (map_.blocks_shots(pr.pos.x, pr.pos.y)) return true;
        if (pr.from_player) {
            if (Entity* e = solid_entity_at(pr.pos)) {
                damage_enemy(*e, pr.dmg);
                return true;
            }
        } else if (pr.pos == player_) {
            hurt_player(pr.dmg);
            return true;
        }
        return false;
    });
}

void Game::step_spawners() {
    int enemy_count = 0;
    for (const Entity& e : entities_) {
        if (is_enemy(e.kind)) ++enemy_count;
    }
    // Iterate by index: spawning invalidates iterators.
    size_t n = entities_.size();
    for (size_t i = 0; i < n; ++i) {
        if (entities_[i].kind != EntityKind::Spawner || entities_[i].hp <= 0) continue;
        if (entities_[i].spawn_cd > 0) {
            --entities_[i].spawn_cd;
            continue;
        }
        if (enemy_count >= kEnemyCap) continue;
        // Emit into a random free neighbor cell.
        static constexpr int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
        static constexpr int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
        int start = rng_.range(0, 7);
        Pos home = entities_[i].pos;
        EntityKind kind = entities_[i].spawn_kind;
        for (int k = 0; k < 8; ++k) {
            Pos p{home.x + dx[(start + k) % 8], home.y + dy[(start + k) % 8]};
            if (!map_.walkable(p.x, p.y) || map_.at(p.x, p.y) == Tile::Exit) continue;
            if (solid_entity_at(p) || pickup_at(p) || p == player_) continue;
            EnemyStats es = enemy_stats(kind, floor_);
            Entity m;
            m.id = next_id_++;
            m.kind = kind;
            m.pos = p;
            m.hp = es.hp;
            m.move_cd = es.move_period;
            entities_.push_back(m);
            ++enemy_count;
            break;
        }
        entities_[i].spawn_cd = spawner_period(floor_);
    }
}

void Game::step_enemies() {
    if (dist_age_ >= 5) recompute_dist();

    for (Entity& e : entities_) {
        if (!is_enemy(e.kind) || e.hp <= 0) continue;
        EnemyStats es = enemy_stats(e.kind, floor_);

        if (e.attack_cd > 0) --e.attack_cd;
        if (e.shoot_cd > 0) --e.shoot_cd;
        if (e.move_cd > 0) {
            --e.move_cd;
            continue;
        }

        int pdx = std::abs(e.pos.x - player_.x);
        int pdy = std::abs(e.pos.y - player_.y);

        // Adjacent: claw the player instead of moving.
        if (pdx <= 1 && pdy <= 1) {
            if (e.attack_cd == 0) {
                hurt_player(es.dmg);
                say(std::string("The ") +
                    (e.kind == EntityKind::Grunt ? "grunt" :
                     e.kind == EntityKind::Ghost ? "ghost" :
                     e.kind == EntityKind::Demon ? "demon" : "lobber") +
                    " hits you for " + std::to_string(es.dmg) + "!");
                e.attack_cd = es.attack_period;
            }
            e.move_cd = es.move_period;
            continue;
        }

        // Lobbers throw from range.
        if (e.kind == EntityKind::Lobber && e.shoot_cd == 0 && pdx <= 8 && pdy <= 8) {
            tae::Dir d{(player_.x > e.pos.x) - (player_.x < e.pos.x),
                       (player_.y > e.pos.y) - (player_.y < e.pos.y)};
            projectiles_.push_back(Projectile{e.pos, d, es.dmg, false});
            e.shoot_cd = 60;
        }

        // Chase: step to the 8-neighbor with the lowest BFS distance.
        static constexpr int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
        static constexpr int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
        int best = dist_at(e.pos);
        Pos best_pos = e.pos;
        int start = rng_.range(0, 7);  // random tiebreak
        for (int k = 0; k < 8; ++k) {
            Pos p{e.pos.x + dx[(start + k) % 8], e.pos.y + dy[(start + k) % 8]};
            if (!map_.walkable(p.x, p.y) || map_.at(p.x, p.y) == Tile::Exit) continue;
            if (map_.at(p.x, p.y) == Tile::Door) continue;  // doors stop monsters
            if (solid_entity_at(p) || pickup_at(p) || p == player_) continue;
            int d = dist_at(p);
            if (d >= 0 && (best < 0 || d < best)) {
                best = d;
                best_pos = p;
            }
        }
        if (!(best_pos == e.pos)) e.pos = best_pos;
        e.move_cd = es.move_period;
    }
}

void Game::health_drain() {
    if (ticks_ % kDrainPeriod == 0) {
        --hp_;
        if (hp_ > 0 && hp_ <= 150 && !warned_low_hp_) {
            warned_low_hp_ = true;
            say(std::string(stats_.name) + " needs food, badly!");
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void Game::recompute_dist() {
    dist_.assign(static_cast<size_t>(map_.width()) * map_.height(), -1);
    std::queue<Pos> q;
    dist_[player_.y * map_.width() + player_.x] = 0;
    q.push(player_);
    while (!q.empty()) {
        Pos p = q.front();
        q.pop();
        int d = dist_[p.y * map_.width() + p.x];
        static constexpr int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
        static constexpr int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
        for (int i = 0; i < 8; ++i) {
            int nx = p.x + dx[i], ny = p.y + dy[i];
            if (!map_.walkable(nx, ny)) continue;
            if (dist_[ny * map_.width() + nx] >= 0) continue;
            dist_[ny * map_.width() + nx] = d + 1;
            q.push({nx, ny});
        }
    }
    dist_age_ = 0;
}

int Game::dist_at(Pos p) const {
    if (!map_.in_bounds(p.x, p.y)) return -1;
    return dist_[p.y * map_.width() + p.x];
}

Entity* Game::solid_entity_at(Pos p) {
    for (Entity& e : entities_) {
        if (e.hp > 0 && e.pos == p && (is_enemy(e.kind) || e.kind == EntityKind::Spawner)) {
            return &e;
        }
    }
    return nullptr;
}

Entity* Game::pickup_at(Pos p) {
    for (Entity& e : entities_) {
        if (e.hp > 0 && e.pos == p && is_pickup(e.kind)) return &e;
    }
    return nullptr;
}

void Game::damage_enemy(Entity& e, int dmg) {
    e.hp -= dmg;
    if (e.hp <= 0) {
        EnemyStats es = enemy_stats(e.kind, floor_);
        if (e.kind == EntityKind::Spawner) {
            score_ += 100;
            say("Spawner destroyed! +100");
        } else {
            score_ += es.score;
        }
        spawn_burst(e.pos);
    }
}

void Game::spawn_burst(Pos p) {
    Entity b;
    b.id = next_id_++;
    b.kind = EntityKind::Burst;
    b.pos = p;
    b.hp = 1;
    b.ttl = 3;
    entities_.push_back(b);
}

void Game::remove_dead_entities() {
    for (Entity& e : entities_) {
        if (e.kind == EntityKind::Burst && e.ttl >= 0 && --e.ttl < 0) e.hp = 0;
    }
    std::erase_if(entities_, [](const Entity& e) { return e.hp <= 0; });
}

}  // namespace gl
