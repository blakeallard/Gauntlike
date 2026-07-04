#include "snake.hpp"

#include <algorithm>

namespace sn {

SnakeGame::SnakeGame(int w, int h, bool wrap, tae::Rng rng)
    : w_(w), h_(h), wrap_(wrap), rng_(rng) {
    P head{w / 2, h / 2};
    body_ = {head, {head.x - 1, head.y}, {head.x - 2, head.y}};
    place_food();
}

void SnakeGame::set_dir(tae::Dir d) {
    if (d.dx != 0 && d.dy != 0) return;  // 4-directional only
    if (d.dx == -dir_.dx && d.dy == -dir_.dy) return;  // no instant reverse
    pending_dir_ = d;
}

int SnakeGame::move_period() const {
    return std::max(2, 6 - static_cast<int>(score_ / 50));
}

void SnakeGame::tick() {
    if (dead_) return;
    ++ticks_;
    if (cd_ > 0) {
        --cd_;
        return;
    }
    cd_ = move_period() - 1;

    dir_ = pending_dir_;
    P head = body_.front();
    head.x += dir_.dx;
    head.y += dir_.dy;

    if (wrap_) {
        head.x = (head.x + w_) % w_;
        head.y = (head.y + h_) % h_;
    } else if (head.x < 0 || head.x >= w_ || head.y < 0 || head.y >= h_) {
        dead_ = true;
        return;
    }

    // Moving into the tail cell is fine when the tail is about to vacate it.
    P tail = body_.back();
    if (hits_body(head) && !(head == tail && grow_ == 0)) {
        dead_ = true;
        return;
    }

    body_.push_front(head);
    if (head == food_) {
        score_ += 10;
        grow_ += 3;
        place_food();
    }
    if (grow_ > 0) {
        --grow_;
    } else {
        body_.pop_back();
    }
}

bool SnakeGame::hits_body(P p) const {
    return std::find(body_.begin(), body_.end(), p) != body_.end();
}

void SnakeGame::place_food() {
    for (int tries = 0; tries < 10000; ++tries) {
        P p{rng_.range(0, w_ - 1), rng_.range(0, h_ - 1)};
        if (!hits_body(p)) {
            food_ = p;
            return;
        }
    }
    food_ = {0, 0};  // board is nearly full; you have basically won
}

}  // namespace sn
