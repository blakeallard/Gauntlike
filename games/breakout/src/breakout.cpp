#include "breakout.hpp"

#include <algorithm>
#include <cmath>

namespace bo {

BreakoutGame::BreakoutGame(int w, int h, tae::Rng rng)
    : w_(w), sub_h_(h * 2), rng_(rng), paddle_x_((w - kPaddleW) / 2.0) {
    load_level();
}

void BreakoutGame::load_level() {
    bricks_.clear();
    powerups_.clear();

    // Rows of bricks across the top, more rows on deeper levels.
    int rows = std::min(4 + level_, 8);
    int brick_w = 5;
    int margin = 2;
    int cols = (w_ - 2 * margin) / brick_w;
    int x0 = (w_ - cols * brick_w) / 2;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            // Deeper levels punch a few gaps for variety.
            if (level_ > 1 && rng_.chance(0.08)) continue;
            bricks_.push_back(Brick{x0 + c * brick_w, 4 + r * 2, brick_w, r, true});
        }
    }
    reset_ball();
}

void BreakoutGame::reset_ball() {
    balls_.clear();
    Ball b;
    b.attached = true;
    balls_.push_back(b);
}

int BreakoutGame::bricks_alive() const {
    int n = 0;
    for (const Brick& b : bricks_) {
        if (b.alive) ++n;
    }
    return n;
}

void BreakoutGame::move_paddle(int dir) {
    paddle_x_ += dir * 1.5;
    paddle_x_ = std::clamp(paddle_x_, 0.0, static_cast<double>(w_ - paddle_w()));
}

void BreakoutGame::launch() {
    for (Ball& b : balls_) {
        if (b.attached) {
            b.attached = false;
            double speed_up = 1.0 + 0.08 * (level_ - 1);
            b.vx = (rng_.chance(0.5) ? 1 : -1) * 0.35 * speed_up;
            b.vy = -0.7 * speed_up;
        }
    }
}

Brick* BreakoutGame::brick_at(double x, double y) {
    int cx = static_cast<int>(x);
    int sy = static_cast<int>(y);
    for (Brick& b : bricks_) {
        if (b.alive && cx >= b.x && cx < b.x + b.w && sy >= b.y && sy < b.y + 2) {
            return &b;
        }
    }
    return nullptr;
}

void BreakoutGame::step_ball(Ball& b) {
    if (b.attached) {
        b.x = paddle_x_ + paddle_w() / 2.0;
        b.y = sub_h_ - 3;
        return;
    }

    // Substep to avoid tunneling through 1-cell bricks at high speed.
    constexpr int kSubsteps = 2;
    for (int s = 0; s < kSubsteps; ++s) {
        b.x += b.vx / kSubsteps;
        b.y += b.vy / kSubsteps;

        // Walls.
        if (b.x < 0) {
            b.x = 0;
            b.vx = std::abs(b.vx);
        } else if (b.x >= w_ - 0.001) {
            b.x = w_ - 0.001;
            b.vx = -std::abs(b.vx);
        }
        if (b.y < 0) {
            b.y = 0;
            b.vy = std::abs(b.vy);
        }

        // Bricks.
        if (Brick* br = brick_at(b.x, b.y)) {
            br->alive = false;
            score_ += 10 + (7 - std::min(br->row, 7));  // higher rows worth more
            b.vy = -b.vy;
            if (rng_.chance(0.18)) {
                powerups_.push_back(PowerUp{br->x + br->w / 2.0,
                                            static_cast<double>(br->y + 2),
                                            rng_.chance(0.5) ? PowerType::Wide
                                                             : PowerType::Multi});
            }
        }

        // Paddle: bottom two subrows; reflect with an angle from the hit
        // offset for control.
        double paddle_top = sub_h_ - 2;
        if (b.vy > 0 && b.y >= paddle_top && b.y < sub_h_ &&
            b.x >= paddle_x_ - 0.5 && b.x <= paddle_x_ + paddle_w() + 0.5) {
            double center = paddle_x_ + paddle_w() / 2.0;
            double off = (b.x - center) / (paddle_w() / 2.0);  // -1..1
            double speed = std::hypot(b.vx, b.vy);
            b.vx = off * 0.8 * speed;
            b.vy = -std::max(0.4 * speed, std::sqrt(std::max(speed * speed - b.vx * b.vx, 0.01)));
            b.y = paddle_top - 0.001;
        }
    }
}

void BreakoutGame::tick() {
    if (dead()) return;
    ++ticks_;
    if (wide_ticks_ > 0) {
        --wide_ticks_;
        // Shrinking back mustn't push the paddle off the field.
        paddle_x_ = std::clamp(paddle_x_, 0.0, static_cast<double>(w_ - paddle_w()));
    }

    for (Ball& b : balls_) step_ball(b);

    // Balls below the floor are lost.
    std::erase_if(balls_, [&](const Ball& b) { return !b.attached && b.y >= sub_h_; });
    if (balls_.empty()) {
        --lives_;
        if (lives_ > 0) reset_ball();
        return;
    }

    // Falling power-ups.
    for (PowerUp& p : powerups_) p.y += 0.35;
    std::erase_if(powerups_, [&](const PowerUp& p) {
        if (p.y >= sub_h_ - 2 && p.x >= paddle_x_ && p.x <= paddle_x_ + paddle_w()) {
            score_ += 25;
            if (p.type == PowerType::Wide) {
                wide_ticks_ = kWideDuration;
            } else {
                // Multi-ball: split the first live ball into three.
                for (Ball& b : balls_) {
                    if (!b.attached) {
                        Ball l = b, r = b;
                        l.vx -= 0.25;
                        r.vx += 0.25;
                        balls_.push_back(l);
                        balls_.push_back(r);
                        break;
                    }
                }
            }
            return true;
        }
        return p.y >= sub_h_;
    });

    // Level cleared: next layout, keep score/lives, fresh ball.
    if (bricks_alive() == 0) {
        ++level_;
        score_ += 100;
        load_level();
    }
}

}  // namespace bo
