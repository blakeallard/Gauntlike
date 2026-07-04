#pragma once

#include <string_view>
#include <vector>

#include "color.hpp"

namespace tae {

struct Cell {
    char32_t ch = U' ';
    Color fg = pal::White;
    Color bg = pal::Black;

    bool operator==(const Cell&) const = default;
};

// Off-screen cell buffer. All drawing happens headlessly into the buffer;
// present() is the only method that talks to curses, and it draws only the
// cells that changed since the previous present() (diff-based rendering).
class Renderer {
public:
    Renderer(int w = 0, int h = 0);

    void resize(int w, int h);
    int width() const { return w_; }
    int height() const { return h_; }

    void clear(Color bg = pal::Black);
    void put(int x, int y, char32_t ch, Color fg, Color bg);
    void put(int x, int y, char32_t ch, Color fg);  // keeps existing bg
    void text(int x, int y, std::string_view ascii, Color fg, Color bg);
    void text(int x, int y, std::u32string_view s, Color fg, Color bg);

    const Cell& at(int x, int y) const { return cells_[y * w_ + x]; }
    bool in_bounds(int x, int y) const { return x >= 0 && x < w_ && y >= 0 && y < h_; }

    // Flush to the terminal (curses). Only changed cells are redrawn.
    void present();
    // Force a full redraw on the next present() (e.g. after a resize).
    void invalidate();

private:
    int w_ = 0, h_ = 0;
    std::vector<Cell> cells_;
    std::vector<Cell> prev_;
    bool full_redraw_ = true;
};

}  // namespace tae
