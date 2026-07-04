#pragma once

#include <cstdint>

namespace tae {

// Plain RGB color. Game code only ever deals in RGB; the renderer decides how
// to map it onto whatever the terminal supports (truecolor / 256 / 8 colors).
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    constexpr bool operator==(const Color&) const = default;
};

// Curated 16-color retro palette — deep purples, teals, hot magenta.
namespace pal {
inline constexpr Color Black{10, 8, 16};
inline constexpr Color DarkPurple{56, 24, 84};
inline constexpr Color Purple{110, 50, 160};
inline constexpr Color HotMagenta{240, 40, 160};
inline constexpr Color DarkTeal{16, 84, 92};
inline constexpr Color Teal{32, 160, 160};
inline constexpr Color Cyan{80, 230, 230};
inline constexpr Color DeepBlue{30, 40, 130};
inline constexpr Color SkyBlue{90, 140, 240};
inline constexpr Color BloodRed{170, 24, 40};
inline constexpr Color Orange{240, 130, 40};
inline constexpr Color Gold{250, 200, 60};
inline constexpr Color Green{60, 190, 80};
inline constexpr Color DarkGreen{20, 100, 50};
inline constexpr Color Grey{130, 130, 145};
inline constexpr Color White{240, 240, 235};
}  // namespace pal

enum class ColorMode {
    Mono,       // no color support at all
    Basic8,     // 8 ANSI colors
    Xterm256,   // 256-color palette
    TrueColor,  // 24-bit via extended color pairs
};

// Decide the best color mode from the COLORTERM env value and the number of
// colors the terminal reports. Pure function so it is unit-testable.
ColorMode detect_color_mode(const char* colorterm, int terminal_colors);

// Quantize an RGB color to the nearest xterm-256 palette index.
int rgb_to_xterm256(Color c);

// Quantize an RGB color to the nearest of the 8 basic ANSI colors (0-7).
int rgb_to_ansi8(Color c);

}  // namespace tae
