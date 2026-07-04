#include "curses_session.hpp"

#include <curses.h>

#include <clocale>
#include <csignal>
#include <cstdlib>
#include <initializer_list>

namespace tae {
namespace {

volatile std::sig_atomic_t g_active = 0;

extern "C" void fatal_signal_handler(int sig) {
    if (g_active) {
        g_active = 0;
        endwin();
    }
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void atexit_handler() {
    CursesSession::shutdown();
}

}  // namespace

CursesSession::CursesSession() {
    std::setlocale(LC_ALL, "");  // required for wide-char (Unicode) output

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
    }

    g_active = 1;
    static bool handlers_installed = false;
    if (!handlers_installed) {
        handlers_installed = true;
        std::atexit(atexit_handler);
        for (int sig : {SIGINT, SIGTERM, SIGHUP, SIGSEGV, SIGABRT, SIGFPE}) {
            std::signal(sig, fatal_signal_handler);
        }
    }
}

CursesSession::~CursesSession() {
    shutdown();
}

void CursesSession::shutdown() {
    if (g_active) {
        g_active = 0;
        endwin();
    }
}

int CursesSession::cols() const { return COLS; }
int CursesSession::rows() const { return LINES; }

}  // namespace tae
