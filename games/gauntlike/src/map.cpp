#include "map.hpp"

#include <queue>
#include <stdexcept>

namespace gl {

Map Map::from_ascii(std::span<const std::string_view> rows) {
    if (rows.empty()) throw std::invalid_argument("empty map");
    int h = static_cast<int>(rows.size());
    int w = static_cast<int>(rows[0].size());
    Map m(w, h);
    for (int y = 0; y < h; ++y) {
        if (static_cast<int>(rows[y].size()) != w) {
            throw std::invalid_argument("ragged map rows");
        }
        for (int x = 0; x < w; ++x) {
            switch (rows[y][x]) {
                case '#': m.set(x, y, Tile::Wall); break;
                case '.': m.set(x, y, Tile::Floor); break;
                case '+': m.set(x, y, Tile::Door); break;
                case '>': m.set(x, y, Tile::Exit); break;
                default: throw std::invalid_argument("unknown map glyph");
            }
        }
    }
    return m;
}

int Map::reachable_count(int x, int y) const {
    if (!walkable(x, y)) return 0;
    std::vector<char> seen(tiles_.size(), 0);
    std::queue<Pos> q;
    q.push({x, y});
    seen[y * w_ + x] = 1;
    int count = 0;
    while (!q.empty()) {
        Pos p = q.front();
        q.pop();
        ++count;
        constexpr int dx[] = {1, -1, 0, 0};
        constexpr int dy[] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i], ny = p.y + dy[i];
            if (walkable(nx, ny) && !seen[ny * w_ + nx]) {
                seen[ny * w_ + nx] = 1;
                q.push({nx, ny});
            }
        }
    }
    return count;
}

bool Map::reachable(Pos from, Pos to) const {
    if (!walkable(from.x, from.y) || !walkable(to.x, to.y)) return false;
    std::vector<char> seen(tiles_.size(), 0);
    std::queue<Pos> q;
    q.push(from);
    seen[from.y * w_ + from.x] = 1;
    while (!q.empty()) {
        Pos p = q.front();
        q.pop();
        if (p == to) return true;
        constexpr int dx[] = {1, -1, 0, 0};
        constexpr int dy[] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i], ny = p.y + dy[i];
            if (walkable(nx, ny) && !seen[ny * w_ + nx]) {
                seen[ny * w_ + nx] = 1;
                q.push({nx, ny});
            }
        }
    }
    return false;
}

}  // namespace gl
