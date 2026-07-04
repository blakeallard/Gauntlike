// Phase 1 demo scene: an @ walking around at 30 FPS. Replaced by the real
// game in Phase 2.
#include "tae/app.hpp"
#include "tae/input.hpp"
#include "tae/renderer.hpp"
#include "tae/ui.hpp"

namespace {

class DemoScene final : public tae::Scene {
public:
    void on_enter(tae::App& app) override {
        x_ = app.renderer().width() / 2;
        y_ = app.renderer().height() / 2;
    }

    void tick(tae::App& app, const std::vector<tae::Key>& keys) override {
        for (tae::Key k : keys) {
            if (k == tae::Key::Quit) {
                app.quit();
                return;
            }
        }
        if (auto d = held_.update(keys)) {
            int nx = x_ + d->dx;
            int ny = y_ + d->dy;
            auto& r = app.renderer();
            if (nx > 1 && nx < r.width() - 2) x_ = nx;
            if (ny > 1 && ny < r.height() - 2) y_ = ny;
        }
    }

    void render(tae::Renderer& r) override {
        r.clear(tae::pal::Black);
        tae::ui::box(r, 0, 0, r.width(), r.height(), tae::pal::DarkPurple, tae::pal::Black);
        tae::ui::text_centered(r, 1, " engine demo — move with wasd/arrows/hjkl, q quits ",
                               tae::pal::Grey, tae::pal::Black);
        r.put(x_, y_, U'@', tae::pal::Gold, tae::pal::Black);
    }

private:
    int x_ = 0, y_ = 0;
    tae::HeldDirection held_;
};

}  // namespace

int main() {
    tae::App app;
    app.run(std::make_unique<DemoScene>());
    return 0;
}
