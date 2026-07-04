#include "tae/color.hpp"

#include <cmath>
#include <cstring>

namespace tae {
namespace {

int sq_dist(int r1, int g1, int b1, int r2, int g2, int b2) {
    int dr = r1 - r2, dg = g1 - g2, db = b1 - b2;
    return dr * dr + dg * dg + db * db;
}

// The 6x6x6 color cube levels used by xterm-256 (indices 16..231).
constexpr int kCubeLevels[6] = {0, 95, 135, 175, 215, 255};

int nearest_cube_level(int v) {
    int best = 0, best_d = 1 << 30;
    for (int i = 0; i < 6; ++i) {
        int d = std::abs(v - kCubeLevels[i]);
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

}  // namespace

ColorMode detect_color_mode(const char* colorterm, int terminal_colors) {
    if (colorterm != nullptr &&
        (std::strcmp(colorterm, "truecolor") == 0 || std::strcmp(colorterm, "24bit") == 0)) {
        return ColorMode::TrueColor;
    }
    if (terminal_colors >= 256) return ColorMode::Xterm256;
    if (terminal_colors >= 8) return ColorMode::Basic8;
    return ColorMode::Mono;
}

int rgb_to_xterm256(Color c) {
    // Candidate 1: nearest point in the 6x6x6 color cube.
    int ri = nearest_cube_level(c.r);
    int gi = nearest_cube_level(c.g);
    int bi = nearest_cube_level(c.b);
    int cube_idx = 16 + 36 * ri + 6 * gi + bi;
    int cube_d = sq_dist(c.r, c.g, c.b, kCubeLevels[ri], kCubeLevels[gi], kCubeLevels[bi]);

    // Candidate 2: nearest greyscale ramp entry (indices 232..255: 8,18,..,238).
    int grey_level = (c.r + c.g + c.b) / 3;
    int gi2 = (grey_level - 8) / 10;
    if (gi2 < 0) gi2 = 0;
    if (gi2 > 23) gi2 = 23;
    int grey_v = 8 + gi2 * 10;
    int grey_d = sq_dist(c.r, c.g, c.b, grey_v, grey_v, grey_v);

    return grey_d < cube_d ? 232 + gi2 : cube_idx;
}

int rgb_to_ansi8(Color c) {
    // Threshold each channel; map to the ANSI 3-bit color index (BGR bits).
    int idx = (c.r > 110 ? 1 : 0) | (c.g > 110 ? 2 : 0) | (c.b > 110 ? 4 : 0);
    // Very dark colors -> black, very bright greys -> white already covered.
    return idx;
}

}  // namespace tae
