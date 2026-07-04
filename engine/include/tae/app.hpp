#pragma once

#include <memory>
#include <vector>

#include "input.hpp"
#include "renderer.hpp"

namespace tae {

class App;
class CursesSession;

// A screen in the scene stack (title, gameplay, pause overlay, game over...).
class Scene {
public:
    virtual ~Scene() = default;

    virtual void on_enter(App&) {}
    virtual void tick(App& app, const std::vector<Key>& keys) = 0;
    virtual void render(Renderer& r) = 0;

    // Transparent scenes (e.g. a pause overlay) let the scene below them
    // render first.
    virtual bool transparent() const { return false; }
};

// Application shell: owns the curses session (teardown guaranteed on exit,
// exceptions and signals), the renderer, and a scene stack driven by a fixed
// 30 Hz timestep loop.
class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    Renderer& renderer() { return renderer_; }

    // Scene stack. Mutations are deferred to the end of the current tick so
    // a scene can safely pop itself.
    void push(std::unique_ptr<Scene> s);
    void pop();
    void replace(std::unique_ptr<Scene> s);

    // Runs until the stack empties or quit() is called.
    void run(std::unique_ptr<Scene> initial);
    void quit() { running_ = false; }

    static constexpr int kTickHz = 30;
    static constexpr int kMinCols = 80;
    static constexpr int kMinRows = 24;

private:
    void apply_pending();

    enum class Op { Push, Pop, Replace };
    struct PendingOp {
        Op op;
        std::unique_ptr<Scene> scene;
    };

    std::unique_ptr<CursesSession> session_;
    Renderer renderer_;
    std::vector<std::unique_ptr<Scene>> stack_;
    std::vector<PendingOp> pending_;
    bool running_ = false;
};

}  // namespace tae
