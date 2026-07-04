#pragma once

namespace tae {

// RAII wrapper around curses init/teardown. A corrupted terminal is the #1
// curses annoyance, so teardown is also wired into atexit and fatal-signal
// handlers: no exit path may leave the terminal in raw mode.
class CursesSession {
public:
    CursesSession();
    ~CursesSession();

    CursesSession(const CursesSession&) = delete;
    CursesSession& operator=(const CursesSession&) = delete;

    int cols() const;
    int rows() const;

    // Restores the terminal if a session is active. Safe to call repeatedly.
    static void shutdown();
};

}  // namespace tae
