#include "scenes.hpp"

#include <cstdio>
#include <string>

#include "snake.hpp"
#include "tae/scores.hpp"
#include "tae/ui.hpp"

namespace sn {
namespace {

using tae::Key;
using tae::Renderer;
namespace pal = tae::pal;

constexpr const char* kScoresGame = "snake";

class GameOverScene;

// ---------------------------------------------------------------------------
// Gameplay
// ---------------------------------------------------------------------------

class PlayScene final : public tae::Scene {
public:
    PlayScene(bool wrap, int term_w, int term_h)
        : game_(std::min(term_w - 2, 100), term_h - 4, wrap, tae::Rng()) {}

    void tick(tae::App& app, const std::vector<Key>& keys) override;

    void render(Renderer& r) override {
        r.clear(pal::Black);

        // HUD.
        r.text(1, 0, "SNAKE", pal::Green, pal::Black);
        r.text(8, 0, "SCORE " + std::to_string(game_.score()), pal::Gold, pal::Black);
        std::string mode = game_.wrap() ? "wrap" : "walls";
        r.text(r.width() - static_cast<int>(mode.size()) - 1, 0, mode, pal::Grey, pal::Black);

        // Board frame below the HUD row; wrap mode gets a dimmer frame to
        // hint pass-through.
        int ox = 1, oy = 2;
        tae::ui::box(r, ox - 1, oy - 1, game_.width() + 2, game_.height() + 2,
                     game_.wrap() ? pal::DarkTeal : pal::Teal, pal::Black);

        r.put(game_.food().x + ox, game_.food().y + oy, U'●', pal::HotMagenta, pal::Black);

        bool first = true;
        for (const P& p : game_.body()) {
            r.put(p.x + ox, p.y + oy, first ? U'█' : U'▓',
                  first ? pal::Gold : pal::Green, pal::Black);
            first = false;
        }
    }

private:
    SnakeGame game_;
    int death_ticks_ = 0;
};

// ---------------------------------------------------------------------------
// Game over
// ---------------------------------------------------------------------------

class GameOverScene final : public tae::Scene {
public:
    GameOverScene(long score, bool wrap) : score_(score), wrap_(wrap) {}

    void on_enter(tae::App&) override {
        tae::HighScores hs(kScoresGame);
        rank_ = hs.add(wrap_ ? "WRAP" : "WALLS", score_);
        entries_ = hs.entries();
    }

    void tick(tae::App& app, const std::vector<Key>& keys) override {
        for (Key k : keys) {
            if (k == Key::Confirm || k == Key::Fire) {
                app.replace(make_title_scene());
                return;
            }
            if (k == Key::Quit) {
                app.quit();
                return;
            }
        }
    }

    void render(Renderer& r) override {
        r.clear(pal::Black);
        int cy = r.height() / 2 - 6;
        tae::ui::text_centered(r, cy, "G A M E   O V E R", pal::BloodRed, pal::Black);
        tae::ui::text_centered(r, cy + 2, "score " + std::to_string(score_), pal::Gold,
                               pal::Black);
        if (rank_ == 0) {
            tae::ui::text_centered(r, cy + 3, "NEW BEST!", pal::HotMagenta, pal::Black);
        }
        int y = cy + 5;
        tae::ui::text_centered(r, y++, "— HALL OF FAME —", pal::Grey, pal::Black);
        int shown = 0;
        for (const auto& e : entries_) {
            if (shown++ >= 5) break;
            char line[48];
            std::snprintf(line, sizeof line, "%-8s %8ld", e.name.c_str(), e.score);
            tae::ui::text_centered(r, y++, line,
                                   shown == rank_ + 1 ? pal::Gold : pal::Grey, pal::Black);
        }
        tae::ui::text_centered(r, y + 1, "enter: title    q: quit", pal::Grey, pal::Black);
    }

private:
    long score_;
    bool wrap_;
    int rank_ = -1;
    std::vector<tae::ScoreEntry> entries_;
};

void PlayScene::tick(tae::App& app, const std::vector<Key>& keys) {
    if (game_.dead()) {
        if (++death_ticks_ > 30) {
            app.replace(std::make_unique<GameOverScene>(game_.score(), game_.wrap()));
        }
        return;
    }
    for (Key k : keys) {
        if (k == Key::Pause || k == Key::Quit) {
            // Snake is small: pause == title, no overlay ceremony.
            app.replace(make_title_scene());
            return;
        }
        if (auto d = tae::key_dir(k)) game_.set_dir(*d);
    }
    game_.tick();
}

// ---------------------------------------------------------------------------
// Title
// ---------------------------------------------------------------------------

class TitleScene final : public tae::Scene {
public:
    void on_enter(tae::App&) override {
        entries_ = tae::HighScores(kScoresGame).entries();
    }

    void tick(tae::App& app, const std::vector<Key>& keys) override {
        ++ticks_;
        for (Key k : keys) {
            switch (k) {
                case Key::Left:
                case Key::Right:
                    wrap_ = !wrap_;
                    break;
                case Key::Confirm:
                case Key::Fire:
                    app.replace(std::make_unique<PlayScene>(wrap_, app.renderer().width(),
                                                            app.renderer().height()));
                    return;
                case Key::Quit:
                    app.quit();
                    return;
                default:
                    break;
            }
        }
    }

    void render(Renderer& r) override {
        r.clear(pal::Black);
        int y = 4;
        bool flash = (ticks_ / 15) % 2 == 0;
        tae::ui::text_centered(r, y, "S N A K E", flash ? pal::Green : pal::DarkGreen,
                               pal::Black);
        tae::ui::text_centered(r, y + 2, "eat ● grow long, don't bite yourself",
                               pal::Grey, pal::Black);

        std::string mode = wrap_ ? "[ wrap-around walls ]" : "[ solid walls ]";
        tae::ui::text_centered(r, y + 5, mode, pal::Cyan, pal::Black);
        tae::ui::text_centered(r, y + 7, "◄ ► toggle walls    enter: play    q: quit",
                               pal::White, pal::Black);

        if (!entries_.empty()) {
            int yy = y + 10;
            tae::ui::text_centered(r, yy++, "— HALL OF FAME —", pal::Grey, pal::Black);
            int shown = 0;
            for (const auto& e : entries_) {
                if (shown++ >= 3) break;
                char line[48];
                std::snprintf(line, sizeof line, "%-8s %8ld", e.name.c_str(), e.score);
                tae::ui::text_centered(r, yy++, line, pal::Gold, pal::Black);
            }
        }
    }

private:
    bool wrap_ = false;
    int ticks_ = 0;
    std::vector<tae::ScoreEntry> entries_;
};

}  // namespace

std::unique_ptr<tae::Scene> make_title_scene() {
    return std::make_unique<TitleScene>();
}

}  // namespace sn
