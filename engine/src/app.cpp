#include "tae/app.hpp"

#include <chrono>
#include <thread>

#include "curses_session.hpp"
#include "tae/ui.hpp"

namespace tae {

App::App() : session_(std::make_unique<CursesSession>()) {
    renderer_.resize(session_->cols(), session_->rows());
}

App::~App() = default;

void App::push(std::unique_ptr<Scene> s) {
    pending_.push_back({Op::Push, std::move(s)});
}

void App::pop() {
    pending_.push_back({Op::Pop, nullptr});
}

void App::replace(std::unique_ptr<Scene> s) {
    pending_.push_back({Op::Replace, std::move(s)});
}

void App::apply_pending() {
    for (auto& p : pending_) {
        switch (p.op) {
            case Op::Push:
                stack_.push_back(std::move(p.scene));
                stack_.back()->on_enter(*this);
                break;
            case Op::Pop:
                if (!stack_.empty()) stack_.pop_back();
                break;
            case Op::Replace:
                if (!stack_.empty()) stack_.pop_back();
                stack_.push_back(std::move(p.scene));
                stack_.back()->on_enter(*this);
                break;
        }
    }
    pending_.clear();
}

void App::run(std::unique_ptr<Scene> initial) {
    using clock = std::chrono::steady_clock;
    const auto tick = std::chrono::nanoseconds(1'000'000'000 / kTickHz);

    push(std::move(initial));
    apply_pending();

    running_ = true;
    auto next = clock::now() + tick;
    while (running_ && !stack_.empty()) {
        std::vector<int> raw = poll_raw();
        std::vector<Key> keys;
        keys.reserve(raw.size());
        for (int rk : raw) {
            if (rk == rawkey::Resize) {
                renderer_.resize(session_->cols(), session_->rows());
            }
            Key k = map_key(rk);
            if (k != Key::None) keys.push_back(k);
        }

        if (renderer_.width() < kMinCols || renderer_.height() < kMinRows) {
            // Too small to play: park the game and ask politely instead of
            // crashing. Any input except quit is ignored.
            for (Key k : keys) {
                if (k == Key::Quit) running_ = false;
            }
            renderer_.clear(pal::Black);
            int cy = renderer_.height() / 2;
            ui::text_centered(renderer_, cy > 0 ? cy - 1 : 0,
                              "Please resize your terminal to at least 80x24",
                              pal::Gold, pal::Black);
            ui::text_centered(renderer_, cy + 1, "(q to quit)", pal::Grey, pal::Black);
            renderer_.present();
            std::this_thread::sleep_until(next);
            next += tick;
            continue;
        }

        stack_.back()->tick(*this, keys);
        apply_pending();
        if (stack_.empty() || !running_) break;

        // Render from the topmost non-transparent scene upward.
        size_t first = stack_.size() - 1;
        while (first > 0 && stack_[first]->transparent()) --first;
        for (size_t i = first; i < stack_.size(); ++i) {
            stack_[i]->render(renderer_);
        }
        renderer_.present();

        // Sleep the remainder of the tick; never busy-wait. If we fell badly
        // behind (e.g. suspended), re-anchor rather than spiral.
        std::this_thread::sleep_until(next);
        auto now = clock::now();
        next += tick;
        if (next < now) next = now + tick;
    }
}

}  // namespace tae
