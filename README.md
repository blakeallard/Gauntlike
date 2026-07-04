# Terminal Arcade

Retro games in your terminal — C++20, ncurses, zero assets. Everything is
Unicode glyphs and ANSI color at a fixed 30 FPS, built on a small shared
engine (`tae`). Perfect for playing between compiles.

**Gauntlike** — a Gauntlet-style dungeon crawler — is the main event, joined
by **Snake** and **Breakout**, all launchable from a single `arcade` menu.

## Build

Requires CMake ≥ 3.20, a C++20 compiler (gcc/clang) and ncurses (wide-char).

```sh
cmake -B build
cmake --build build
```

Binaries land in `build/games/*/`:

```sh
./build/games/arcade/arcade        # menu launcher
./build/games/gauntlike/gauntlike  # or run any game directly
./build/games/snake/snake
./build/games/breakout/breakout
```

Install them on your PATH:

```sh
cmake --install build --prefix ~/.local
```

Run the tests (Catch2 is fetched automatically):

```sh
ctest --test-dir build
```

Terminals must be at least **80×24**; the games pause with a friendly prompt
if yours is smaller.

## Controls

Shared across all games:

| Key | Action |
|---|---|
| arrows / `wasd` / `hjkl` | move (plus `y u b n` diagonals in Gauntlike) |
| `space` | fire / launch / start |
| `enter` | confirm |
| `p` | pause |
| `q` | quit / back out |

### Gauntlike

Pick a class — **Warrior** (melee bruiser), **Wizard** (glass cannon),
**Elf** (fast), **Valkyrie** (balanced) — and descend. Your health drains
constantly, Gauntlet-style: grab food (`%`) to survive. Enemies pour out of
spawners (`▓`) until you destroy them. Find the key (`k`), open the exit,
descend; every floor is deeper and meaner. `e` drinks a potion — a
screen-clearing smart bomb. Bump into enemies for melee, `space` shoots in
the direction you last moved.

Each run prints its dungeon seed in the message line, so a given seed
reproduces the same floors.

### Snake

Classic. Toggle wrap-around walls on the title screen with ◄ ►. The snake
speeds up as you score.

### Breakout

Half-block glyphs (`▀ ▄`) double the vertical resolution, so the ball moves
smoothly. Steer the rebound with where the ball meets the paddle. Catch
falling power-ups: `W` widens the paddle for a while, `M` splits the ball
three ways.

High scores persist per game under
`$XDG_DATA_HOME/terminal-arcade/` (default `~/.local/share/terminal-arcade/`).

## Ghostty notes

- Ghostty supports 24-bit truecolor and advertises it via
  `COLORTERM=truecolor`; the engine detects that and allocates extended
  color pairs, falling back to the xterm-256 palette elsewhere.
- Any font with good box-drawing/block coverage looks great — the games only
  use Unicode glyphs. A Nerd Font or a bitmap-style font (e.g. Terminus)
  leans into the retro look.
- A UTF-8 locale is required for the block glyphs; the engine falls back to
  `C.UTF-8` automatically if your environment doesn't set one.

## Repository layout

```
engine/            # "tae" — shared terminal-arcade engine (static lib)
games/gauntlike/   # the dungeon crawler
games/snake/
games/breakout/
games/arcade/      # menu launcher (runs games in-process)
tests/             # Catch2 unit tests for all headless game logic
docs/PLAN.md       # the original execution plan this project was built from
```

Engine design in one breath: fixed 30 Hz timestep, off-screen cell buffer
with diff-based drawing (unchanged frames cost nothing), scene stack, RGB
colors quantized to whatever the terminal supports, and a curses session
wrapper that restores your terminal on every exit path — including crashes
and Ctrl-C. Game logic never touches curses, so all of it runs headless in
the test suite.
