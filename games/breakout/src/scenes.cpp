#include "scenes.hpp"

#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "breakout.hpp"
#include "tae/scores.hpp"
#include "tae/ui.hpp"

namespace bo {
namespace {

using tae::Color;
using tae::Key;
using tae::Renderer;
namespace pal = tae::pal;

constexpr const char* kScoresGame = "breakout";

Color row_color(int row) {
    static const Color kRows[] = {pal::BloodRed, pal::Orange,  pal::Gold,  pal::Green,
                                  pal::Teal,     pal::SkyBlue, pal::Purple, pal::HotMagenta};
    return kRows[row % 8];
}

// Half-block compositor: a W x 2H grid of optional colors flushed to the
// renderer as ▀/▄/█ cells — doubles the vertical resolution.
class SubPixelBuffer {
public:
    SubPixelBuffer(int w, int subrows) : w_(w), h_(subrows), px_(w * subrows) {}

    void set(int x, int sy, Color c) {
        if (x < 0 || x >= w_ || sy < 0 || sy >= h_) return;
        px_[sy * w_ + x] = c;
    }

    void flush(Renderer& r, int ox, int oy, Color bg) const {
        for (int cy = 0; cy < h_ / 2; ++cy) {
            for (int x = 0; x < w_; ++x) {
                const auto& top = px_[(cy * 2) * w_ + x];
                const auto& bot = px_[(cy * 2 + 1) * w_ + x];
                if (top && bot) {
                    r.put(x + ox, cy + oy, U'▀', *top, *bot);
                } else if (top) {
                    r.put(x + ox, cy + oy, U'▀', *top, bg);
                } else if (bot) {
                    r.put(x + ox, cy + oy, U'▄', *bot, bg);
                }
            }
        }
    }

private:
    int w_, h_;
    std::vector<std::optional<Color>> px_;
};

class GameOverScene final : public tae::Scene {
public:
    GameOverScene(long score, int level) : score_(score), level_(level) {}

    void on_enter(tae::App&) override {
        tae::HighScores hs(kScoresGame);
        rank_ = hs.add("LVL" + std::to_string(level_), score_);
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
        tae::ui::text_centered(r, cy + 2,
                               "score " + std::to_string(score_) + "   reached level " +
                                   std::to_string(level_),
                               pal::Gold, pal::Black);
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
    int level_;
    int rank_ = -1;
    std::vector<tae::ScoreEntry> entries_;
};

class PlayScene final : public tae::Scene {
public:
    PlayScene(int term_w, int term_h)
        : game_(std::min(term_w - 2, 100), term_h - 2, tae::Rng()) {}

    void tick(tae::App& app, const std::vector<Key>& keys) override {
        if (game_.dead()) {
            if (++death_ticks_ > 30) {
                app.replace(std::make_unique<GameOverScene>(game_.score(), game_.level()));
            }
            return;
        }
        for (Key k : keys) {
            switch (k) {
                case Key::Quit:
                case Key::Pause:
                    app.replace(make_title_scene());
                    return;
                case Key::Fire:
                case Key::Confirm:
                    game_.launch();
                    break;
                default:
                    break;
            }
        }
        if (auto d = held_.update(keys); d && d->dx != 0) {
            game_.move_paddle(d->dx);
        }
        game_.tick();
    }

    void render(Renderer& r) override {
        r.clear(pal::Black);

        // HUD.
        r.text(1, 0, "BREAKOUT", pal::Orange, pal::Black);
        r.text(11, 0, "SCORE " + std::to_string(game_.score()), pal::Gold, pal::Black);
        r.text(26, 0, "LEVEL " + std::to_string(game_.level()), pal::White, pal::Black);
        r.text(r.width() - 12, 0, "LIVES ", pal::Grey, pal::Black);
        for (int i = 0; i < game_.lives(); ++i) {
            r.put(r.width() - 6 + i, 0, U'♥', pal::BloodRed, pal::Black);
        }

        int ox = 1, oy = 1;
        SubPixelBuffer sub(game_.width(), game_.subrows());

        for (const Brick& b : game_.bricks()) {
            if (!b.alive) continue;
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < b.w - 1; ++dx) {  // -1 leaves a seam between bricks
                    sub.set(b.x + dx, b.y + dy, row_color(b.row));
                }
            }
        }

        // Paddle: bottom two subrows, teal.
        int px = static_cast<int>(game_.paddle_x());
        for (int i = 0; i < game_.paddle_w(); ++i) {
            sub.set(px + i, game_.subrows() - 2, pal::Cyan);
            sub.set(px + i, game_.subrows() - 1, pal::DarkTeal);
        }

        for (const Ball& b : game_.balls()) {
            sub.set(static_cast<int>(b.x), static_cast<int>(b.y), pal::White);
        }

        sub.flush(r, ox, oy, pal::Black);

        // Power-ups as letters on top of the subpixel field.
        for (const PowerUp& p : game_.powerups()) {
            r.put(static_cast<int>(p.x) + ox, static_cast<int>(p.y) / 2 + oy,
                  p.type == PowerType::Wide ? U'W' : U'M', pal::HotMagenta, pal::Black);
        }

        for (const Ball& b : game_.balls()) {
            if (b.attached) {
                tae::ui::text_centered(r, r.height() / 2, "space to launch",
                                       pal::Grey, pal::Black);
                break;
            }
        }
    }

private:
    BreakoutGame game_;
    tae::HeldDirection held_;
    int death_ticks_ = 0;
};

class TitleScene final : public tae::Scene {
public:
    void on_enter(tae::App&) override {
        entries_ = tae::HighScores(kScoresGame).entries();
    }

    void tick(tae::App& app, const std::vector<Key>& keys) override {
        ++ticks_;
        for (Key k : keys) {
            if (k == Key::Confirm || k == Key::Fire) {
                app.replace(std::make_unique<PlayScene>(app.renderer().width(),
                                                        app.renderer().height()));
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
        int y = 4;
        bool flash = (ticks_ / 15) % 2 == 0;
        tae::ui::text_centered(r, y, "B R E A K O U T", flash ? pal::Orange : pal::BloodRed,
                               pal::Black);
        tae::ui::text_centered(r, y + 2, "half-block smooth edition", pal::Grey, pal::Black);
        // A little brick strip for flavor.
        int strip_w = 40;
        int x0 = (r.width() - strip_w) / 2;
        for (int i = 0; i < strip_w; ++i) {
            r.put(x0 + i, y + 4, U'▀', row_color(i / 5), pal::Black);
        }
        tae::ui::text_centered(r, y + 7, "◄ ► move    space: launch    W wide  M multi-ball",
                               pal::Cyan, pal::Black);
        tae::ui::text_centered(r, y + 9, "enter: play    q: quit", pal::White, pal::Black);

        if (!entries_.empty()) {
            int yy = y + 12;
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
    int ticks_ = 0;
    std::vector<tae::ScoreEntry> entries_;
};

}  // namespace

std::unique_ptr<tae::Scene> make_title_scene() {
    return std::make_unique<TitleScene>();
}

}  // namespace bo
