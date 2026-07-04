// Pure key-mapping logic — no curses here, so it is headless-testable.
#include <cctype>

#include "tae/input.hpp"

namespace tae {

Key map_key(int raw) {
    switch (raw) {
        case rawkey::Up: return Key::Up;
        case rawkey::Down: return Key::Down;
        case rawkey::Left: return Key::Left;
        case rawkey::Right: return Key::Right;
        case rawkey::Enter: return Key::Confirm;
        case rawkey::Escape: return Key::Cancel;
        case rawkey::Resize: return Key::Resize;
        default: break;
    }
    if (raw < 0 || raw > 0x10FFFF) return Key::None;
    switch (std::tolower(raw)) {
        case 'w': case 'k': return Key::Up;
        case 's': case 'j': return Key::Down;
        case 'a': case 'h': return Key::Left;
        case 'd': case 'l': return Key::Right;
        case 'y': return Key::UpLeft;
        case 'u': return Key::UpRight;
        case 'b': return Key::DownLeft;
        case 'n': return Key::DownRight;
        case ' ': return Key::Fire;
        case 'e': return Key::Potion;
        case 'p': return Key::Pause;
        case 'q': return Key::Quit;
        default: return Key::None;
    }
}

std::optional<Dir> key_dir(Key k) {
    switch (k) {
        case Key::Up: return Dir{0, -1};
        case Key::Down: return Dir{0, 1};
        case Key::Left: return Dir{-1, 0};
        case Key::Right: return Dir{1, 0};
        case Key::UpLeft: return Dir{-1, -1};
        case Key::UpRight: return Dir{1, -1};
        case Key::DownLeft: return Dir{-1, 1};
        case Key::DownRight: return Dir{1, 1};
        default: return std::nullopt;
    }
}

std::optional<Dir> HeldDirection::update(const std::vector<Key>& keys) {
    for (Key k : keys) {
        if (auto d = key_dir(k)) {
            dir_ = *d;
            remaining_ = hold_ticks_;
        }
    }
    if (remaining_ > 0) {
        --remaining_;
        return dir_;
    }
    return std::nullopt;
}

}  // namespace tae
