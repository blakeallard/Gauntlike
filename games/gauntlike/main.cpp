#include "tae/app.hpp"
#include "tae/renderer.hpp"

int main() {
    tae::App app;
    app.run([](tae::App& a, const std::vector<int>& keys) {
        for (int k : keys) {
            if (k == 'q') return false;
        }

        auto& r = a.renderer();
        r.clear();

        // Wide-char + color check: block-glyph frame around the title.
        const std::u32string title = U"HELLO DUNGEON";
        int cx = (r.width() - static_cast<int>(title.size())) / 2;
        int cy = r.height() / 2;
        r.text(cx, cy, title, tae::pal::HotMagenta, tae::pal::Black);
        r.text(cx - 2, cy, U"█", tae::pal::Purple, tae::pal::Black);
        r.text(cx + static_cast<int>(title.size()) + 1, cy, U"█", tae::pal::Purple,
               tae::pal::Black);
        r.text(cx, cy + 2, std::u32string(title.size(), U'─'), tae::pal::Teal, tae::pal::Black);
        return true;
    });
    return 0;
}
