#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace gl {

enum class Tile : std::uint8_t {
    Wall,
    Floor,
    Door,      // closed: blocks shots, opens when walked into
    DoorOpen,
    Exit,      // locked stairs down; needs a key
};

struct Pos {
    int x = 0;
    int y = 0;
    constexpr bool operator==(const Pos&) const = default;
};

class Map {
public:
    Map() = default;
    Map(int w, int h, Tile fill = Tile::Wall)
        : w_(w), h_(h), tiles_(static_cast<size_t>(w) * h, fill) {}

    // Parse a hand-authored ASCII map: '#' wall, '.' floor, '+' door,
    // '>' exit. Rows must be equal length.
    static Map from_ascii(std::span<const std::string_view> rows);

    int width() const { return w_; }
    int height() const { return h_; }
    bool in_bounds(int x, int y) const { return x >= 0 && x < w_ && y >= 0 && y < h_; }

    Tile at(int x, int y) const { return tiles_[y * w_ + x]; }
    void set(int x, int y, Tile t) { tiles_[y * w_ + x] = t; }

    bool walkable(int x, int y) const {
        if (!in_bounds(x, y)) return false;
        return at(x, y) != Tile::Wall;
    }

    // Walls and closed doors stop projectiles.
    bool blocks_shots(int x, int y) const {
        if (!in_bounds(x, y)) return true;
        Tile t = at(x, y);
        return t == Tile::Wall || t == Tile::Door;
    }

    // Number of walkable tiles reachable from (x,y) — flood fill, used to
    // verify generated floors are fully connected.
    int reachable_count(int x, int y) const;
    bool reachable(Pos from, Pos to) const;

private:
    int w_ = 0, h_ = 0;
    std::vector<Tile> tiles_;
};

}  // namespace gl
