#pragma once

#include <cstdint>
#include <random>

namespace tae {

// mt19937 seeded per-run; the seed is retrievable so dungeons are
// reproducible ("seed 12345" in a bug report regenerates the same floors).
class Rng {
public:
    Rng() : Rng(std::random_device{}()) {}
    explicit Rng(std::uint32_t seed) : seed_(seed), gen_(seed) {}

    std::uint32_t seed() const { return seed_; }

    // Uniform integer in [lo, hi] (inclusive).
    int range(int lo, int hi) {
        return std::uniform_int_distribution<int>(lo, hi)(gen_);
    }

    bool chance(double p) {
        return std::uniform_real_distribution<double>(0.0, 1.0)(gen_) < p;
    }

    std::mt19937& engine() { return gen_; }

private:
    std::uint32_t seed_;
    std::mt19937 gen_;
};

}  // namespace tae
