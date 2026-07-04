#include "tae/renderer.hpp"

#include <curses.h>

#include <cstdlib>
#include <map>
#include <utility>

namespace tae {

// ---------------------------------------------------------------------------
// Headless buffer operations
// ---------------------------------------------------------------------------

Renderer::Renderer(int w, int h) {
    resize(w, h);
}

void Renderer::resize(int w, int h) {
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    w_ = w;
    h_ = h;
    cells_.assign(static_cast<size_t>(w) * h, Cell{});
    prev_.assign(static_cast<size_t>(w) * h, Cell{});
    full_redraw_ = true;
}

void Renderer::clear(Color bg) {
    Cell blank{U' ', pal::White, bg};
    for (auto& c : cells_) c = blank;
}

void Renderer::put(int x, int y, char32_t ch, Color fg, Color bg) {
    if (!in_bounds(x, y)) return;
    cells_[y * w_ + x] = Cell{ch, fg, bg};
}

void Renderer::put(int x, int y, char32_t ch, Color fg) {
    if (!in_bounds(x, y)) return;
    Cell& c = cells_[y * w_ + x];
    c.ch = ch;
    c.fg = fg;
}

void Renderer::text(int x, int y, std::string_view ascii, Color fg, Color bg) {
    for (size_t i = 0; i < ascii.size(); ++i) {
        put(x + static_cast<int>(i), y, static_cast<char32_t>(ascii[i]), fg, bg);
    }
}

void Renderer::text(int x, int y, std::u32string_view s, Color fg, Color bg) {
    for (size_t i = 0; i < s.size(); ++i) {
        put(x + static_cast<int>(i), y, s[i], fg, bg);
    }
}

void Renderer::invalidate() {
    full_redraw_ = true;
}

// ---------------------------------------------------------------------------
// Curses-facing side: color pair management + diff-based flush
// ---------------------------------------------------------------------------

namespace {

ColorMode g_mode = ColorMode::Mono;
bool g_mode_detected = false;

// Cache of (fg,bg) -> curses color pair.
std::map<std::pair<int, int>, int> g_pairs;
int g_next_pair = 1;

#if defined(NCURSES_EXT_COLORS) && NCURSES_EXT_COLORS
// In truecolor mode we define custom colors (indices >= 16) per unique RGB.
std::map<std::uint32_t, int> g_custom_colors;
int g_next_color = 16;

int truecolor_index(Color c) {
    std::uint32_t key = (std::uint32_t(c.r) << 16) | (std::uint32_t(c.g) << 8) | c.b;
    auto it = g_custom_colors.find(key);
    if (it != g_custom_colors.end()) return it->second;
    if (g_next_color >= COLORS) return rgb_to_xterm256(c);  // out of slots
    int idx = g_next_color++;
    // curses wants 0..1000 per channel
    init_extended_color(idx, c.r * 1000 / 255, c.g * 1000 / 255, c.b * 1000 / 255);
    g_custom_colors.emplace(key, idx);
    return idx;
}
#endif

int color_index(Color c) {
    switch (g_mode) {
#if defined(NCURSES_EXT_COLORS) && NCURSES_EXT_COLORS
        case ColorMode::TrueColor: return truecolor_index(c);
#else
        case ColorMode::TrueColor: return rgb_to_xterm256(c);
#endif
        case ColorMode::Xterm256: return rgb_to_xterm256(c);
        case ColorMode::Basic8: return rgb_to_ansi8(c);
        case ColorMode::Mono: return -1;
    }
    return -1;
}

int pair_for(Color fg, Color bg) {
    if (g_mode == ColorMode::Mono) return 0;
    int fi = color_index(fg);
    int bi = color_index(bg);
    auto key = std::make_pair(fi, bi);
    auto it = g_pairs.find(key);
    if (it != g_pairs.end()) return it->second;
    if (g_next_pair >= COLOR_PAIRS) return 0;  // out of pairs; fall back
    int id = g_next_pair++;
#if defined(NCURSES_EXT_COLORS) && NCURSES_EXT_COLORS
    init_extended_pair(id, fi, bi);
#else
    init_pair(static_cast<short>(id), static_cast<short>(fi), static_cast<short>(bi));
#endif
    g_pairs.emplace(key, id);
    return id;
}

void ensure_mode() {
    if (g_mode_detected) return;
    g_mode_detected = true;
    int colors = has_colors() ? COLORS : 0;
    g_mode = detect_color_mode(std::getenv("COLORTERM"), colors);
    // Truecolor requires ncurses extended-color support with enough slots
    // and a terminal reporting >= 256 colors; otherwise degrade gracefully.
#if defined(NCURSES_EXT_COLORS) && NCURSES_EXT_COLORS
    if (g_mode == ColorMode::TrueColor && (colors < 256 || !can_change_color())) {
        g_mode = detect_color_mode(nullptr, colors);
    }
#else
    if (g_mode == ColorMode::TrueColor) {
        g_mode = detect_color_mode(nullptr, colors);
    }
#endif
}

void draw_cell(int x, int y, const Cell& c) {
    int pair = pair_for(c.fg, c.bg);
    wchar_t wch[2] = {static_cast<wchar_t>(c.ch), L'\0'};
    cchar_t cc;
#if defined(NCURSES_EXT_COLORS) && NCURSES_EXT_COLORS
    setcchar(&cc, wch, A_NORMAL, 0, &pair);
#else
    setcchar(&cc, wch, A_NORMAL, static_cast<short>(pair), nullptr);
#endif
    mvadd_wch(y, x, &cc);
}

}  // namespace

void Renderer::present() {
    ensure_mode();
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            const Cell& c = cells_[y * w_ + x];
            if (full_redraw_ || !(c == prev_[y * w_ + x])) {
                draw_cell(x, y, c);
            }
        }
    }
    full_redraw_ = false;
    prev_ = cells_;
    refresh();
}

}  // namespace tae
