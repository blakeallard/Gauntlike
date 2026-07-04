#include "tae/app.hpp"

#include <chrono>
#include <thread>

#include "curses_session.hpp"
#include "tae/input.hpp"

namespace tae {

App::App() : session_(std::make_unique<CursesSession>()) {
    renderer_.resize(session_->cols(), session_->rows());
}

App::~App() = default;

void App::run(const std::function<bool(App&, const std::vector<int>&)>& frame) {
    using clock = std::chrono::steady_clock;
    const auto tick = std::chrono::nanoseconds(1'000'000'000 / kTickHz);

    running_ = true;
    auto next = clock::now() + tick;
    while (running_) {
        std::vector<int> keys = poll_raw();
        for (int k : keys) {
            if (k == rawkey::Resize) {
                renderer_.resize(session_->cols(), session_->rows());
            }
        }
        if (!frame(*this, keys)) break;
        renderer_.present();

        // Sleep the remainder of the tick; never busy-wait. If we fell badly
        // behind (e.g. the process was suspended), re-anchor instead of
        // spiralling to catch up.
        std::this_thread::sleep_until(next);
        auto now = clock::now();
        next += tick;
        if (next < now) next = now + tick;
    }
}

}  // namespace tae
