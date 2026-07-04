#pragma once

#include <optional>
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
inline constexpr int Escape = 0x1000007;
}  // namespace rawkey

// Drain all pending keypresses (non-blocking). Curses-specific codes are
// translated to the rawkey constants above; everything else is the raw char.
std::vector<int> poll_raw();

// Logical keys, as the games see them.
enum class Key {
    None,
    Up,
    Down,
    Left,
    Right,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight,
    Fire,     // space
    Potion,   // e
    Pause,    // p
    Quit,     // q
    Confirm,  // enter
    Cancel,   // escape
    Resize,
};

// Map a raw key to a logical Key. Arrows, WASD and vi keys (hjkl + yubn
// diagonals) all work; letters are case-insensitive. Pure and unit-testable.
Key map_key(int raw);

struct Dir {
    int dx = 0;
    int dy = 0;
    constexpr bool operator==(const Dir&) const = default;
};

// Direction vector for a movement key, or nullopt for non-movement keys.
std::optional<Dir> key_dir(Key k);

// Terminals only report key *presses* (plus autorepeat) — there is no key-up
// event. To get a "held direction" feel, remember the last direction pressed
// and keep it alive for a short window; autorepeat refreshes it while the key
// is physically held.
class HeldDirection {
public:
    explicit HeldDirection(int hold_ticks = 4) : hold_ticks_(hold_ticks) {}

    // Call once per tick with this tick's keys. Returns the active direction,
    // or nullopt if no direction is held.
    std::optional<Dir> update(const std::vector<Key>& keys);

    void reset() { remaining_ = 0; }

private:
    int hold_ticks_;
    int remaining_ = 0;
    Dir dir_{};
};

}  // namespace tae
