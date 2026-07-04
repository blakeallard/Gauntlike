// UI drawing helpers — pure buffer operations, no curses.
#include "tae/ui.hpp"

namespace tae::ui {

void box(Renderer& r, int x, int y, int w, int h, Color fg, Color bg) {
    if (w < 2 || h < 2) return;
    r.put(x, y, U'┌', fg, bg);
    r.put(x + w - 1, y, U'┐', fg, bg);
    r.put(x, y + h - 1, U'└', fg, bg);
    r.put(x + w - 1, y + h - 1, U'┘', fg, bg);
    for (int i = 1; i < w - 1; ++i) {
        r.put(x + i, y, U'─', fg, bg);
        r.put(x + i, y + h - 1, U'─', fg, bg);
    }
    for (int j = 1; j < h - 1; ++j) {
        r.put(x, y + j, U'│', fg, bg);
        r.put(x + w - 1, y + j, U'│', fg, bg);
    }
}

void panel(Renderer& r, int x, int y, int w, int h, Color fg, Color bg) {
    for (int j = 1; j < h - 1; ++j) {
        for (int i = 1; i < w - 1; ++i) {
            r.put(x + i, y + j, U' ', fg, bg);
        }
    }
    box(r, x, y, w, h, fg, bg);
}

void text_centered(Renderer& r, int y, std::string_view s, Color fg, Color bg) {
    int x = (r.width() - static_cast<int>(s.size())) / 2;
    r.text(x, y, s, fg, bg);
}

void hbar(Renderer& r, int x, int y, int w, double frac, Color full, Color empty, Color bg) {
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    int filled = static_cast<int>(frac * w + 0.5);
    for (int i = 0; i < w; ++i) {
        r.put(x + i, y, i < filled ? U'█' : U'░', i < filled ? full : empty, bg);
    }
}

}  // namespace tae::ui
