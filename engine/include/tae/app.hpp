#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "renderer.hpp"

namespace tae {

class CursesSession;

// Minimal application shell: owns the curses session (with guaranteed
// teardown on exit, exceptions and signals) and runs a fixed-timestep loop.
class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    Renderer& renderer() { return renderer_; }

    // Runs `frame` at ~30 Hz with the keys pressed since the last tick.
    // Return false from the callback (or call quit()) to stop.
    void run(const std::function<bool(App&, const std::vector<int>&)>& frame);

    void quit() { running_ = false; }

    static constexpr int kTickHz = 30;

private:
    std::unique_ptr<CursesSession> session_;
    Renderer renderer_;
    bool running_ = false;
};

}  // namespace tae
