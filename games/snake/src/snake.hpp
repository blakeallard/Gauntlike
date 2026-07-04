#pragma once

#include <deque>

#include "tae/input.hpp"
#include "tae/rng.hpp"

namespace sn {

struct P {
    int x = 0;
    int y = 0;
    constexpr bool operator==(const P&) const = default;
};

// Headless classic snake on a w×h board. Optional wrap-around walls; the
// snake speeds up as the score grows.
class SnakeGame {
public:
    SnakeGame(int w, int h, bool wrap, tae::Rng rng);

    // Steer; reversing straight into yourself is ignored.
    void set_dir(tae::Dir d);
    void tick();

    int width() const { return w_; }
    int height() const { return h_; }
    bool wrap() const { return wrap_; }
    const std::deque<P>& body() const { return body_; }  // front = head
    P food() const { return food_; }
    long score() const { return score_; }
    bool dead() const { return dead_; }
    int ticks() const { return ticks_; }

    // Ticks between steps: starts leisurely, accelerates with score.
    int move_period() const;

private:
    void place_food();
    bool hits_body(P p) const;

    int w_, h_;
    bool wrap_;
    tae::Rng rng_;
    std::deque<P> body_;
    tae::Dir dir_{1, 0};
    tae::Dir pending_dir_{1, 0};
    P food_;
    long score_ = 0;
    int grow_ = 0;
    bool dead_ = false;
    int ticks_ = 0;
    int cd_ = 0;
};

}  // namespace sn
