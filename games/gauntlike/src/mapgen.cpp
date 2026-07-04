#include "mapgen.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace gl {
namespace {

struct Room {
    int x, y, w, h;
    Pos center() const { return {x + w / 2, y + h / 2}; }
    bool intersects(const Room& o, int pad) const {
        return x - pad < o.x + o.w && o.x - pad < x + w &&
               y - pad < o.y + o.h && o.y - pad < y + h;
    }
};

void carve_room(Map& m, const Room& r) {
    for (int j = r.y; j < r.y + r.h; ++j) {
        for (int i = r.x; i < r.x + r.w; ++i) {
            m.set(i, j, Tile::Floor);
        }
    }
}

// L-shaped corridor between two points; drops a door where the corridor
// pierces a room wall (approximated: cells that were Wall become Floor,
// occasionally Door for flavor).
void carve_corridor(Map& m, tae::Rng& rng, Pos a, Pos b) {
    Pos cur = a;
    bool horizontal_first = rng.chance(0.5);
    auto step = [&](int tx, int ty) {
        while (cur.x != tx) {
            cur.x += (tx > cur.x) ? 1 : -1;
            if (m.at(cur.x, cur.y) == Tile::Wall) {
                m.set(cur.x, cur.y, rng.chance(0.12) ? Tile::Door : Tile::Floor);
            }
        }
        while (cur.y != ty) {
            cur.y += (ty > cur.y) ? 1 : -1;
            if (m.at(cur.x, cur.y) == Tile::Wall) {
                m.set(cur.x, cur.y, rng.chance(0.12) ? Tile::Door : Tile::Floor);
            }
        }
    };
    if (horizontal_first) {
        step(b.x, a.y);
        step(b.x, b.y);
    } else {
        step(a.x, b.y);
        step(b.x, b.y);
    }
}

// BFS distances from `from` over walkable tiles; -1 = unreachable.
std::vector<int> distance_field(const Map& m, Pos from) {
    std::vector<int> dist(static_cast<size_t>(m.width()) * m.height(), -1);
    std::queue<Pos> q;
    dist[from.y * m.width() + from.x] = 0;
    q.push(from);
    while (!q.empty()) {
        Pos p = q.front();
        q.pop();
        int d = dist[p.y * m.width() + p.x];
        constexpr int dx[] = {1, -1, 0, 0};
        constexpr int dy[] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i], ny = p.y + dy[i];
            if (m.walkable(nx, ny) && dist[ny * m.width() + nx] < 0) {
                dist[ny * m.width() + nx] = d + 1;
                q.push({nx, ny});
            }
        }
    }
    return dist;
}

bool try_generate(tae::Rng& rng, int w, int h, int floor, FloorPlan& out) {
    Map m(w, h, Tile::Wall);

    // Scatter non-overlapping rooms.
    std::vector<Room> rooms;
    int attempts = 60;
    while (attempts-- > 0 && static_cast<int>(rooms.size()) < 10) {
        Room r;
        r.w = rng.range(6, 12);
        r.h = rng.range(4, 7);
        r.x = rng.range(1, w - r.w - 2);
        r.y = rng.range(1, h - r.h - 2);
        bool ok = true;
        for (const Room& o : rooms) {
            if (r.intersects(o, 1)) {
                ok = false;
                break;
            }
        }
        if (ok) rooms.push_back(r);
    }
    if (rooms.size() < 4) return false;

    for (const Room& r : rooms) carve_room(m, r);
    for (size_t i = 1; i < rooms.size(); ++i) {
        carve_corridor(m, rng, rooms[i - 1].center(), rooms[i].center());
    }
    // One extra loop connection for less linear floors.
    if (rooms.size() >= 4) {
        carve_corridor(m, rng, rooms.front().center(), rooms[rooms.size() / 2].center());
    }

    Pos start = rooms.front().center();

    // Verify full connectivity of all carved tiles from the start.
    int total_walkable = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (m.walkable(x, y)) ++total_walkable;
        }
    }
    if (m.reachable_count(start.x, start.y) != total_walkable) return false;

    // Exit goes in the room farthest (BFS) from the start; key in the
    // second-farthest, so the objective forces a real trip.
    auto dist = distance_field(m, start);
    auto room_dist = [&](const Room& r) {
        Pos c = r.center();
        return dist[c.y * w + c.x];
    };
    std::vector<size_t> order(rooms.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](size_t a, size_t b) { return room_dist(rooms[a]) > room_dist(rooms[b]); });

    Pos exit = rooms[order[0]].center();
    Pos key = rooms[order[1]].center();
    m.set(exit.x, exit.y, Tile::Exit);

    // Track occupied cells so nothing stacks.
    std::vector<Pos> used = {start, exit, key};
    auto occupied = [&](Pos p) {
        return std::find(used.begin(), used.end(), p) != used.end();
    };
    auto random_floor = [&](int min_dist) -> Pos {
        for (int tries = 0; tries < 200; ++tries) {
            const Room& r = rooms[static_cast<size_t>(rng.range(0, static_cast<int>(rooms.size()) - 1))];
            Pos p{rng.range(r.x, r.x + r.w - 1), rng.range(r.y, r.y + r.h - 1)};
            if (m.at(p.x, p.y) != Tile::Floor || occupied(p)) continue;
            if (dist[p.y * w + p.x] < min_dist) continue;
            used.push_back(p);
            return p;
        }
        return {-1, -1};
    };

    FloorPlan& plan = out;
    plan = FloorPlan{};
    // Spawners: 2 on floor 1, up to 4 later; never right next to the start.
    int n_spawners = std::min(2 + (floor - 1) / 2, 4);
    for (int i = 0; i < n_spawners; ++i) {
        Pos p = random_floor(8);
        if (p.x >= 0) plan.spawners.push_back(p);
    }
    if (plan.spawners.size() < 2) return false;

    for (int i = 0, n = rng.range(2, 4); i < n; ++i) {
        Pos p = random_floor(0);
        if (p.x >= 0) plan.food.push_back(p);
    }
    for (int i = 0, n = rng.range(3, 5); i < n; ++i) {
        Pos p = random_floor(0);
        if (p.x >= 0) plan.treasure.push_back(p);
    }
    for (int i = 0, n = rng.range(1, 2); i < n; ++i) {
        Pos p = random_floor(0);
        if (p.x >= 0) plan.potions.push_back(p);
    }
    if (plan.food.empty()) return false;

    plan.map = std::move(m);
    plan.start = start;
    plan.exit = exit;
    plan.key = key;
    return true;
}

}  // namespace

FloorPlan generate_floor(tae::Rng& rng, int w, int h, int floor) {
    FloorPlan plan;
    // Generation is randomized; retry until a plan validates. In practice
    // the first or second attempt succeeds.
    for (int i = 0; i < 100; ++i) {
        if (try_generate(rng, w, h, floor, plan)) return plan;
    }
    // Pathological terminal sizes shouldn't be reachable (App enforces
    // 80x24), but fail loudly rather than loop forever.
    throw std::runtime_error("mapgen: could not generate a valid floor");
}

}  // namespace gl
