// Terminal Arcade launcher: a menu that runs each game in-process on the
// engine's scene stack. Quitting a game's title screen pops back here.
#include <memory>
#include <string>
#include <vector>

#include "../breakout/src/scenes.hpp"
#include "../gauntlike/src/scenes.hpp"
#include "../snake/src/scenes.hpp"
#include "tae/app.hpp"
#include "tae/scores.hpp"
#include "tae/ui.hpp"

namespace {

using tae::Key;
namespace pal = tae::pal;

struct MenuItem {
    const char* name;
    const char* tagline;
    tae::Color color;
    std::unique_ptr<tae::Scene> (*make)();
};

const MenuItem kItems[] = {
    {"GAUNTLIKE", "dungeon crawl — food, keys, spawners", pal::HotMagenta,
     [] { return gl::make_title_scene(); }},
    {"SNAKE", "eat, grow, don't bite yourself", pal::Green,
     [] { return sn::make_title_scene(); }},
    {"BREAKOUT", "half-block smooth brick smashing", pal::Orange,
     [] { return bo::make_title_scene(); }},
};
constexpr int kGameCount = static_cast<int>(std::size(kItems));

class MenuScene final : public tae::Scene {
public:
    void tick(tae::App& app, const std::vector<Key>& keys) override {
        ++ticks_;
        for (Key k : keys) {
            switch (k) {
                case Key::Up:
                    sel_ = (sel_ + kGameCount) % (kGameCount + 1);
                    break;
                case Key::Down:
                    sel_ = (sel_ + 1) % (kGameCount + 1);
                    break;
                case Key::Confirm:
                case Key::Fire:
                    if (sel_ < kGameCount) {
                        app.push(kItems[sel_].make());
                    } else {
                        app.quit();
                    }
                    return;
                case Key::Quit:
                    app.quit();
                    return;
                default:
                    break;
            }
        }
    }

    void render(tae::Renderer& r) override {
        r.clear(pal::Black);
        bool flash = (ticks_ / 15) % 2 == 0;
        int y = 3;
        tae::ui::text_centered(r, y, "▄▄▄ T E R M I N A L   A R C A D E ▄▄▄",
                               flash ? pal::Cyan : pal::Teal, pal::Black);
        tae::ui::text_centered(r, y + 2, "insert coin (none required)", pal::Grey, pal::Black);

        int box_w = 56;
        int x0 = (r.width() - box_w) / 2;
        int yy = y + 5;
        for (int i = 0; i <= kGameCount; ++i) {
            bool sel = (i == sel_);
            bool is_quit = (i == kGameCount);
            std::string name = is_quit ? "QUIT" : kItems[i].name;
            tae::Color c = is_quit ? pal::Grey : kItems[i].color;

            if (sel) {
                tae::ui::panel(r, x0, yy, box_w, 3, c, pal::Black);
                r.text(x0 + 2, yy + 1, "► " + name, c, pal::Black);
            } else {
                r.text(x0 + 4, yy + 1, name, pal::Grey, pal::Black);
            }
            if (!is_quit) {
                std::string tag = kItems[i].tagline;
                r.text(x0 + box_w - 2 - static_cast<int>(tag.size()), yy + 1, tag,
                       sel ? pal::White : pal::DarkPurple, pal::Black);
            }
            yy += 3;
        }

        tae::ui::text_centered(r, yy + 1, "▲ ▼ choose    enter: play    q: quit",
                               pal::White, pal::Black);

        // Best score per game, straight from the shared score files.
        yy += 3;
        for (const MenuItem& item : kItems) {
            std::string lower(item.name);
            for (char& ch : lower) ch = static_cast<char>(tolower(ch));
            tae::HighScores hs(lower);
            if (!hs.entries().empty()) {
                tae::ui::text_centered(r, yy++,
                                       lower + " best: " +
                                           std::to_string(hs.entries()[0].score) + " (" +
                                           hs.entries()[0].name + ")",
                                       pal::DarkGreen, pal::Black);
            }
        }
    }

private:
    int sel_ = 0;
    int ticks_ = 0;
};

}  // namespace

int main() {
    tae::App app;
    app.run(std::make_unique<MenuScene>());
    return 0;
}
