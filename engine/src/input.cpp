#include "tae/input.hpp"

#include <curses.h>

namespace tae {

std::vector<int> poll_raw() {
    std::vector<int> keys;
    int ch;
    while ((ch = getch()) != ERR) {
        switch (ch) {
            case KEY_RESIZE: keys.push_back(rawkey::Resize); break;
            case KEY_UP: keys.push_back(rawkey::Up); break;
            case KEY_DOWN: keys.push_back(rawkey::Down); break;
            case KEY_LEFT: keys.push_back(rawkey::Left); break;
            case KEY_RIGHT: keys.push_back(rawkey::Right); break;
            case KEY_ENTER:
            case '\r':
            case '\n': keys.push_back(rawkey::Enter); break;
            case 27: keys.push_back(rawkey::Escape); break;
            default: keys.push_back(ch); break;
        }
    }
    return keys;
}

}  // namespace tae
