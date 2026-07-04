#include "scenes.hpp"

#include <algorithm>
#include <string>

#include "tae/scores.hpp"
#include "tae/ui.hpp"

namespace gl {
namespace {

using tae::Color;
using tae::Key;
using tae::Renderer;
namespace pal = tae::pal;

constexpr const char* kScoresGame = "gauntlike";

Color class_color(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return pal::Orange;
        case PlayerClass::Wizard: return pal::SkyBlue;
        case PlayerClass::Elf: return pal::Green;
        case PlayerClass::Valkyrie: return pal::HotMagenta;
    }
    return pal::White;
}

// Per-floor color themes: dungeon purples → teal caves → hellish reds.
struct Theme {
    Color wall;
    Color wall_deep;
    Color floor;
    Color accent;
};

const Theme& theme_for(int floor) {
    static const Theme kThemes[] = {
        {pal::Purple, pal::DarkPurple, pal::Grey, pal::HotMagenta},
        {pal::Teal, pal::DarkTeal, pal::Grey, pal::Cyan},
        {pal::BloodRed, pal::DarkPurple, pal::Grey, pal::Orange},
    };
    return kThemes[((floor - 1) / 2) % 3];
}

// ---------------------------------------------------------------------------
// Pause overlay
// ---------------------------------------------------------------------------

class PauseScene final : public tae::Scene {
public:
    bool transparent() const override { return true; }

    void tick(tae::App& app, const std::vector<Key>& keys) override {
        for (Key k : keys) {
            if (k == Key::Pause || k == Key::Cancel || k == Key::Confirm) {
                app.pop();
                return;
            }
            if (k == Key::Quit) {
                // Abandon the run, back to the title.
                app.pop();
                app.replace(make_title_scene());
                return;
            }
        }
    }

    void render(Renderer& r) override {
        int w = 36, h = 7;
        int x = (r.width() - w) / 2, y = (r.height() - h) / 2;
        tae::ui::panel(r, x, y, w, h, pal::Gold, pal::Black);
        tae::ui::text_centered(r, y + 2, "* PAUSED *", pal::Gold, pal::Black);
        tae::ui::text_centered(r, y + 4, "p resume   q abandon run", pal::Grey, pal::Black);
    }
};

// ---------------------------------------------------------------------------
// Game over
// ---------------------------------------------------------------------------

class GameOverScene final : public tae::Scene {
public:
    GameOverScene(PlayerClass cls, long score, int floor)
        : cls_(cls), score_(score), floor_(floor) {}

    void on_enter(tae::App&) override {
        tae::HighScores hs(kScoresGame);
        rank_ = hs.add(stats_for(cls_).name, score_);
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
        int cy = r.height() / 2 - 8;
        tae::ui::text_centered(r, cy, "G A M E   O V E R", pal::BloodRed, pal::Black);
        tae::ui::text_centered(r, cy + 2,
                               std::string(stats_for(cls_).name) + " fell on floor " +
                                   std::to_string(floor_),
                               class_color(cls_), pal::Black);
        tae::ui::text_centered(r, cy + 3, "score " + std::to_string(score_),
                               pal::Gold, pal::Black);
        if (rank_ == 0) {
            tae::ui::text_centered(r, cy + 4, "NEW BEST!", pal::HotMagenta, pal::Black);
        } else if (rank_ > 0) {
            tae::ui::text_centered(r, cy + 4, "high score #" + std::to_string(rank_ + 1),
                                   pal::HotMagenta, pal::Black);
        }

        int y = cy + 6;
        tae::ui::text_centered(r, y++, "— HALL OF FAME —", pal::Grey, pal::Black);
        int shown = 0;
        for (const auto& e : entries_) {
            if (shown++ >= 5) break;
            char line[48];
            std::snprintf(line, sizeof line, "%-10s %8ld", e.name.c_str(), e.score);
            tae::ui::text_centered(r, y++, line,
                                   shown == rank_ + 1 ? pal::Gold : pal::Grey, pal::Black);
        }
        tae::ui::text_centered(r, y + 1, "enter: title    q: quit", pal::Grey, pal::Black);
    }

private:
    PlayerClass cls_;
    long score_;
    int floor_;
    int rank_ = -1;
    std::vector<tae::ScoreEntry> entries_;
};

// ---------------------------------------------------------------------------
// Gameplay
// ---------------------------------------------------------------------------

class PlayScene final : public tae::Scene {
public:
    PlayScene(PlayerClass cls, int term_w, int term_h)
        : game_(cls, tae::Rng(), std::min(term_w, 120), term_h - 2) {}

    void tick(tae::App& app, const std::vector<Key>& keys) override {
        if (game_.dead()) {
            if (++death_ticks_ > 45) {
                app.replace(std::make_unique<GameOverScene>(game_.cls(), game_.score(),
                                                            game_.floor()));
            }
            game_.tick({});
            return;
        }

        Game::Input in;
        for (Key k : keys) {
            switch (k) {
                case Key::Pause:
                    app.push(std::make_unique<PauseScene>());
                    return;
                case Key::Quit:
                    app.push(std::make_unique<PauseScene>());
                    return;
                case Key::Fire: in.fire = true; break;
                case Key::Potion: in.potion = true; break;
                default: break;
            }
        }
        in.move = held_.update(keys);
        game_.tick(in);
    }

    void render(Renderer& r) override {
        r.clear(pal::Black);
        draw_hud(r);
        draw_map(r);
        draw_messages(r);
    }

private:
    // Screen shake: jitter the map viewport by one cell while shake ticks
    // remain.
    int shake_dx() const {
        if (game_.shake() <= 0) return 0;
        return (game_.ticks() % 2 == 0) ? 1 : -1;
    }

    void draw_hud(Renderer& r) {
        const auto& st = game_.stats();
        Color cc = class_color(game_.cls());
        int x = 1;
        auto label = [&](const std::string& s, Color c) {
            r.text(x, 0, s, c, pal::Black);
            x += static_cast<int>(s.size());
        };
        label(st.name, cc);
        label("  HP ", pal::Grey);
        double frac = static_cast<double>(std::max(game_.hp(), 0)) / st.max_hp;
        tae::ui::hbar(r, x, 0, 12, frac,
                      frac > 0.3 ? pal::Green : pal::BloodRed, pal::DarkPurple, pal::Black);
        x += 13;
        label(std::to_string(std::max(game_.hp(), 0)), frac > 0.3 ? pal::White : pal::BloodRed);
        label("  SCORE ", pal::Grey);
        label(std::to_string(game_.score()), pal::Gold);
        label("  FLOOR ", pal::Grey);
        label(std::to_string(game_.floor()), pal::White);
        label("  KEYS ", pal::Grey);
        label(std::to_string(game_.keys()), pal::Gold);
        label("  POTIONS ", pal::Grey);
        label(std::to_string(game_.potions()), pal::HotMagenta);
    }

    void draw_map(Renderer& r) {
        const Map& m = game_.map();
        const Theme& th = theme_for(game_.floor());
        int ox = shake_dx();
        int oy = 1;  // row 0 is the HUD
        bool blink = (game_.ticks() / 8) % 2 == 0;

        for (int y = 0; y < m.height() && y + oy < r.height() - 1; ++y) {
            for (int x = 0; x < m.width(); ++x) {
                int sx = x + ox;
                switch (m.at(x, y)) {
                    case Tile::Wall: {
                        // Two-tone walls for a bit of depth.
                        bool deep = ((x * 7 + y * 13) % 5) == 0;
                        r.put(sx, y + oy, U'█', deep ? th.wall_deep : th.wall, pal::Black);
                        break;
                    }
                    case Tile::Floor:
                        r.put(sx, y + oy, U'·', pal::DarkPurple, pal::Black);
                        break;
                    case Tile::Door:
                        r.put(sx, y + oy, U'+', pal::Gold, pal::Black);
                        break;
                    case Tile::DoorOpen:
                        r.put(sx, y + oy, U'\'', pal::Grey, pal::Black);
                        break;
                    case Tile::Exit:
                        r.put(sx, y + oy, U'▓', blink ? pal::Gold : th.accent, pal::Black);
                        break;
                }
            }
        }

        for (const auto& pr : game_.projectiles()) {
            r.put(pr.pos.x + ox, pr.pos.y + oy, U'•',
                  pr.from_player ? class_color(game_.cls()) : pal::BloodRed, pal::Black);
        }

        bool spawner_frame = (game_.ticks() / 10) % 2 == 0;
        for (const auto& e : game_.entities()) {
            char32_t g = U'?';
            Color c = pal::White;
            switch (e.kind) {
                case EntityKind::Grunt: g = U'g'; c = pal::Orange; break;
                case EntityKind::Ghost: g = U'G'; c = pal::Cyan; break;
                case EntityKind::Demon: g = U'd'; c = pal::BloodRed; break;
                case EntityKind::Lobber: g = U'l'; c = pal::Green; break;
                case EntityKind::Spawner:
                    g = spawner_frame ? U'▓' : U'▒';
                    c = pal::BloodRed;
                    break;
                case EntityKind::Food: g = U'%'; c = pal::Green; break;
                case EntityKind::Treasure: g = U'$'; c = pal::Gold; break;
                case EntityKind::PotionPickup: g = U'!'; c = pal::HotMagenta; break;
                case EntityKind::KeyPickup: g = U'k'; c = pal::Gold; break;
                case EntityKind::Burst:
                    g = U'*';
                    c = (e.ttl % 2 == 0) ? pal::White : pal::Orange;
                    break;
            }
            r.put(e.pos.x + ox, e.pos.y + oy, g, c, pal::Black);
        }

        Pos p = game_.player();
        Color pc = game_.dead() ? pal::Grey : class_color(game_.cls());
        r.put(p.x + ox, p.y + oy, game_.dead() ? U'%' : U'@', pc, pal::Black);
    }

    void draw_messages(Renderer& r) {
        if (game_.messages().empty()) return;
        const std::string& msg = game_.messages().back();
        r.text(1, r.height() - 1, msg, pal::White, pal::Black);
    }

    Game game_;
    tae::HeldDirection held_;
    int death_ticks_ = 0;
};

// ---------------------------------------------------------------------------
// Title / class select
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
                    sel_ = (sel_ + 3) % 4;
                    break;
                case Key::Right:
                    sel_ = (sel_ + 1) % 4;
                    break;
                case Key::Confirm:
                case Key::Fire:
                    app.replace(make_play_scene(kAllClasses[sel_], app.renderer().width(),
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
        int y = 2;
        static constexpr std::string_view kLogo[] = {
            R"( ██████   █████  ██    ██ ███    ██ ████████ ██      ██ ██   ██ ███████)",
            R"(██       ██   ██ ██    ██ ████   ██    ██    ██      ██ ██  ██  ██     )",
            R"(██   ███ ███████ ██    ██ ██ ██  ██    ██    ██      ██ █████   █████  )",
            R"(██    ██ ██   ██ ██    ██ ██  ██ ██    ██    ██      ██ ██  ██  ██     )",
            R"( ██████  ██   ██  ██████  ██   ████    ██    ███████ ██ ██   ██ ███████)",
        };
        bool flash = (ticks_ / 15) % 2 == 0;
        for (const auto& line : kLogo) {
            int x = (r.width() - static_cast<int>(line.size())) / 2;
            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] != ' ') {
                    r.put(x + static_cast<int>(i), y, U'█',
                          flash ? pal::HotMagenta : pal::Purple, pal::Black);
                }
            }
            ++y;
        }
        tae::ui::text_centered(r, ++y, "a Gauntlet-style dungeon crawl", pal::Grey, pal::Black);

        // Class cards.
        y += 2;
        int card_w = 17, gap = 2;
        int total = 4 * card_w + 3 * gap;
        int x0 = (r.width() - total) / 2;
        for (int i = 0; i < 4; ++i) {
            PlayerClass c = kAllClasses[i];
            const ClassStats& st = stats_for(c);
            int x = x0 + i * (card_w + gap);
            bool sel = (i == sel_);
            Color frame = sel ? class_color(c) : pal::DarkPurple;
            tae::ui::panel(r, x, y, card_w, 8, frame, pal::Black);
            r.text(x + 2, y + 1, st.name, sel ? class_color(c) : pal::Grey, pal::Black);
            r.text(x + 2, y + 3, "hp    " + std::to_string(st.max_hp), pal::Grey, pal::Black);
            r.text(x + 2, y + 4, "melee " + std::to_string(st.melee_dmg), pal::Grey, pal::Black);
            r.text(x + 2, y + 5, "shot  " + std::to_string(st.shot_dmg), pal::Grey, pal::Black);
            r.text(x + 2, y + 6, st.move_period <= 2 ? "speed fast" : "speed norm",
                   pal::Grey, pal::Black);
            if (sel) r.put(x + card_w / 2, y + 7, U'▲', class_color(c), pal::Black);
        }
        y += 9;
        tae::ui::text_centered(r, y, "◄ ► choose class    enter: descend    q: quit",
                               pal::White, pal::Black);

        if (!entries_.empty()) {
            y += 2;
            tae::ui::text_centered(r, y++, "— HALL OF FAME —", pal::Grey, pal::Black);
            int shown = 0;
            for (const auto& e : entries_) {
                if (shown++ >= 3) break;
                char line[48];
                std::snprintf(line, sizeof line, "%-10s %8ld", e.name.c_str(), e.score);
                tae::ui::text_centered(r, y++, line, pal::Gold, pal::Black);
            }
        }
        tae::ui::text_centered(r, r.height() - 2,
                               "move: wasd/arrows/hjkl+yubn   fire: space   potion: e   pause: p",
                               pal::DarkPurple, pal::Black);
    }

private:
    int sel_ = 0;
    int ticks_ = 0;
    std::vector<tae::ScoreEntry> entries_;
};

}  // namespace

std::unique_ptr<tae::Scene> make_title_scene() {
    return std::make_unique<TitleScene>();
}

std::unique_ptr<tae::Scene> make_play_scene(PlayerClass cls, int term_w, int term_h) {
    return std::make_unique<PlayScene>(cls, term_w, term_h);
}

}  // namespace gl
