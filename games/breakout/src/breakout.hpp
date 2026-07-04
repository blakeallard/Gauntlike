#pragma once

#include <vector>

#include "tae/rng.hpp"

namespace bo {

// The playfield uses half-block sub-cell resolution vertically: a terminal
// cell is two "subrows" tall (rendered with ▀/▄/█), so ball movement is
// twice as smooth as the character grid. All y coordinates below are in
// subrows; x stays in cell units.

struct Ball {
    double x = 0, y = 0;
    double vx = 0, vy = 0;
    bool attached = false;  // riding the paddle, waiting for launch
};

struct Brick {
    int x = 0;      // leftmost cell
    int y = 0;      // top subrow (bricks are 2 subrows = 1 cell tall)
    int w = 0;      // width in cells
    int row = 0;    // color row index
    bool alive = true;
};

enum class PowerType { Wide, Multi };

struct PowerUp {
    double x = 0, y = 0;  // falling; y in subrows
    PowerType type = PowerType::Wide;
};

class BreakoutGame {
public:
    // w cells wide, h cells tall (playfield => 2*h subrows).
    BreakoutGame(int w, int h, tae::Rng rng);

    void move_paddle(int dir);  // -1 or +1, once per tick
    void launch();              // release an attached ball
    void tick();

    int width() const { return w_; }
    int subrows() const { return sub_h_; }
    const std::vector<Ball>& balls() const { return balls_; }
    const std::vector<Brick>& bricks() const { return bricks_; }
    const std::vector<PowerUp>& powerups() const { return powerups_; }
    double paddle_x() const { return paddle_x_; }  // left edge, cells
    int paddle_w() const { return wide_ticks_ > 0 ? kPaddleWide : kPaddleW; }
    int paddle_cell_y() const { return sub_h_ / 2 - 1; }
    long score() const { return score_; }
    int lives() const { return lives_; }
    int level() const { return level_; }
    bool dead() const { return lives_ <= 0; }
    int ticks() const { return ticks_; }
    int bricks_alive() const;

    static constexpr int kPaddleW = 9;
    static constexpr int kPaddleWide = 15;
    static constexpr int kWideDuration = 450;  // 15s at 30 Hz

private:
    void load_level();
    void reset_ball();
    void step_ball(Ball& b);
    Brick* brick_at(double x, double y);

    int w_, sub_h_;
    tae::Rng rng_;
    int level_ = 1;
    long score_ = 0;
    int lives_ = 3;
    double paddle_x_;
    int wide_ticks_ = 0;
    int ticks_ = 0;
    std::vector<Ball> balls_;
    std::vector<Brick> bricks_;
    std::vector<PowerUp> powerups_;
};

}  // namespace bo
