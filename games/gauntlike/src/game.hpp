#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "map.hpp"
#include "mapgen.hpp"
#include "tae/input.hpp"
#include "tae/rng.hpp"

namespace gl {

enum class PlayerClass { Warrior, Wizard, Elf, Valkyrie };
inline constexpr PlayerClass kAllClasses[] = {PlayerClass::Warrior, PlayerClass::Wizard,
                                              PlayerClass::Elf, PlayerClass::Valkyrie};

struct ClassStats {
    const char* name;
    int max_hp;
    int melee_dmg;
    int shot_dmg;
    int move_period;  // ticks between steps (lower = faster)
    int shot_period;  // ticks between shots
};
const ClassStats& stats_for(PlayerClass c);

enum class EntityKind {
    Grunt,
    Ghost,
    Demon,
    Lobber,
    Spawner,
    Food,
    Treasure,
    PotionPickup,
    KeyPickup,
    Burst,  // short-lived death flash
};
bool is_enemy(EntityKind k);
bool is_pickup(EntityKind k);

struct EnemyStats {
    int hp;
    int dmg;
    int move_period;
    int attack_period;
    int score;
};
// Base stats scaled up by floor number.
EnemyStats enemy_stats(EntityKind k, int floor);

struct Entity {
    int id = 0;
    EntityKind kind = EntityKind::Grunt;
    Pos pos;
    int hp = 1;
    int move_cd = 0;
    int attack_cd = 0;
    int shoot_cd = 0;
    // Spawner-only:
    EntityKind spawn_kind = EntityKind::Grunt;
    int spawn_cd = 0;
    // Burst-only:
    int ttl = -1;
};

struct Projectile {
    Pos pos;
    tae::Dir dir;
    int dmg = 0;
    bool from_player = true;
};

// Headless Gauntlike simulation: no curses, fully unit-testable.
class Game {
public:
    struct Input {
        std::optional<tae::Dir> move;
        bool fire = false;
        bool potion = false;
    };

    // Normal play: procedurally generated floors.
    Game(PlayerClass cls, tae::Rng rng, int map_w, int map_h);
    // Tests: play on a hand-built floor plan.
    Game(PlayerClass cls, tae::Rng rng, FloorPlan plan);

    void tick(const Input& in);

    const Map& map() const { return map_; }
    const std::vector<Entity>& entities() const { return entities_; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    PlayerClass cls() const { return cls_; }
    const ClassStats& stats() const { return stats_; }
    Pos player() const { return player_; }
    tae::Dir facing() const { return facing_; }
    int hp() const { return hp_; }
    long score() const { return score_; }
    int keys() const { return keys_; }
    int potions() const { return potions_; }
    int floor() const { return floor_; }
    bool dead() const { return dead_; }
    int shake() const { return shake_; }
    int ticks() const { return ticks_; }
    std::uint32_t seed() const { return seed_; }
    const std::deque<std::string>& messages() const { return messages_; }

    static constexpr int kEnemyCap = 60;
    static constexpr int kDrainPeriod = 15;  // 2 hp/sec at 30 Hz
    static constexpr int kFoodValue = 150;

private:
    void load_plan(const FloorPlan& plan);
    void descend();
    void say(std::string msg);
    void try_move(tae::Dir d);
    void fire();
    void use_potion();
    void step_projectiles();
    void step_enemies();
    void step_spawners();
    void health_drain();
    void recompute_dist();
    int dist_at(Pos p) const;
    Entity* solid_entity_at(Pos p);   // enemy/spawner blocking a cell
    Entity* pickup_at(Pos p);
    void damage_enemy(Entity& e, int dmg);
    void hurt_player(int dmg);
    void pick_up(Entity& e);
    void spawn_burst(Pos p);
    void remove_dead_entities();

    PlayerClass cls_;
    ClassStats stats_;
    tae::Rng rng_;
    std::uint32_t seed_;
    int map_w_ = 0, map_h_ = 0;
    Map map_;
    Pos player_;
    tae::Dir facing_{1, 0};
    int hp_ = 0;
    long score_ = 0;
    int keys_ = 0;
    int potions_ = 0;
    int floor_ = 1;
    bool dead_ = false;
    int shake_ = 0;
    int ticks_ = 0;
    int move_cd_ = 0;
    int shot_cd_ = 0;
    bool warned_low_hp_ = false;
    int next_id_ = 1;
    std::vector<Entity> entities_;
    std::vector<Projectile> projectiles_;
    std::vector<int> dist_;
    int dist_age_ = 1 << 20;
    std::deque<std::string> messages_;
};

}  // namespace gl
