#pragma once

#include <vector>

namespace tae {

// Special raw key values (mirrors curses codes without leaking curses.h).
namespace rawkey {
inline constexpr int Resize = 0x1000001;
inline constexpr int Up = 0x1000002;
inline constexpr int Down = 0x1000003;
inline constexpr int Left = 0x1000004;
inline constexpr int Right = 0x1000005;
inline constexpr int Enter = 0x1000006;
}  // namespace rawkey

// Drain all pending keypresses (non-blocking). Curses-specific codes are
// translated to the rawkey constants above; everything else is the raw char.
std::vector<int> poll_raw();

}  // namespace tae
