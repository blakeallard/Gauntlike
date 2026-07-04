#pragma once

#include <vector>

#include "map.hpp"
#include "tae/rng.hpp"

namespace gl {

// Everything a fresh floor needs: the tile map plus placement for the
// player, objective items, spawners and pickups.
struct FloorPlan {
    Map map;
    Pos start;
    Pos exit;
    Pos key;
    std::vector<Pos> spawners;   // 2-4, scaled by floor
    std::vector<Pos> food;
    std::vector<Pos> treasure;
    std::vector<Pos> potions;
};

// Rooms-and-corridors generation. Guaranteed connected (flood-fill verified;
// regenerates on the rare failure). `floor` (1-based) scales spawner count.
FloorPlan generate_floor(tae::Rng& rng, int w, int h, int floor);

}  // namespace gl
