#include <catch2/catch_test_macros.hpp>

#include "tae/color.hpp"

using namespace tae;

TEST_CASE("detect_color_mode honors COLORTERM=truecolor") {
    CHECK(detect_color_mode("truecolor", 256) == ColorMode::TrueColor);
    CHECK(detect_color_mode("24bit", 256) == ColorMode::TrueColor);
}

TEST_CASE("detect_color_mode falls back by reported color count") {
    CHECK(detect_color_mode(nullptr, 256) == ColorMode::Xterm256);
    CHECK(detect_color_mode("", 256) == ColorMode::Xterm256);
    CHECK(detect_color_mode(nullptr, 88) == ColorMode::Basic8);
    CHECK(detect_color_mode(nullptr, 8) == ColorMode::Basic8);
    CHECK(detect_color_mode(nullptr, 0) == ColorMode::Mono);
    CHECK(detect_color_mode(nullptr, -1) == ColorMode::Mono);
}

TEST_CASE("rgb_to_xterm256 hits exact cube corners") {
    CHECK(rgb_to_xterm256(Color{0, 0, 0}) == 16);         // cube 0,0,0
    CHECK(rgb_to_xterm256(Color{255, 255, 255}) == 231);  // cube 5,5,5
    CHECK(rgb_to_xterm256(Color{255, 0, 0}) == 196);      // pure red
    CHECK(rgb_to_xterm256(Color{0, 255, 0}) == 46);       // pure green
    CHECK(rgb_to_xterm256(Color{0, 0, 255}) == 21);       // pure blue
}

TEST_CASE("rgb_to_xterm256 prefers the grey ramp for greys") {
    // 128,128,128 -> grey ramp value 128 = index 232 + 12
    CHECK(rgb_to_xterm256(Color{128, 128, 128}) == 244);
    // near-grey should still land on the ramp, not a cube corner
    int idx = rgb_to_xterm256(Color{120, 121, 119});
    CHECK(idx >= 232);
}

TEST_CASE("rgb_to_ansi8 thresholds channels") {
    CHECK(rgb_to_ansi8(Color{0, 0, 0}) == 0);        // black
    CHECK(rgb_to_ansi8(Color{255, 0, 0}) == 1);      // red
    CHECK(rgb_to_ansi8(Color{0, 255, 0}) == 2);      // green
    CHECK(rgb_to_ansi8(Color{0, 0, 255}) == 4);      // blue
    CHECK(rgb_to_ansi8(Color{255, 255, 0}) == 3);    // yellow
    CHECK(rgb_to_ansi8(Color{255, 255, 255}) == 7);  // white
}

TEST_CASE("every palette entry quantizes to a distinct 256 index") {
    using namespace tae::pal;
    const Color colors[] = {Black, DarkPurple, Purple, HotMagenta, DarkTeal, Teal,
                            Cyan,  DeepBlue,   SkyBlue, BloodRed,  Orange,  Gold,
                            Green, DarkGreen,  Grey,    White};
    for (size_t i = 0; i < std::size(colors); ++i) {
        for (size_t j = i + 1; j < std::size(colors); ++j) {
            CHECK(rgb_to_xterm256(colors[i]) != rgb_to_xterm256(colors[j]));
        }
    }
}
