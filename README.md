# Terminal Arcade — Execution Plan for Claude Code

## Project Vision

Build a collection of retro-style games playable entirely in the terminal (Ghostty), written in **C++20**, with an aesthetic inspired by Gauntlet, NES/SNES, and Sega Genesis classics. The centerpiece is **"Gauntlike"** — a Gauntlet-style dungeon crawler — built on a small reusable engine that later games (Snake, Breakout, a vertical shmup) share.

Design goals:
- Instant launch from the command line (`gauntlike`), instant quit (`q`), pause anytime (`p`). Perfect for playing between compiles.
- Runs at a smooth fixed timestep (~30 FPS logic) with flicker-free rendering.
- No external assets — everything is Unicode glyphs + ANSI color. Ghostty supports 24-bit truecolor, so use it for that Genesis-palette feel.
- Zero-config: builds with CMake, depends only on ncurses (universally available).

## Tech Stack

| Choice | Decision | Rationale |
|---|---|---|
| Language | C++20 | User preference; modern features (structs w/ designated init, ranges, `std::optional`) |
| Rendering | ncurses (wide-char: `ncursesw`) | Ubiquitous, stable, handles input + double buffering. Enable 256/truecolor via `init_extended_pair` where available |
| Build | CMake ≥ 3.20 | Standard |
| Testing | Catch2 (FetchContent) | Unit-test game logic (map gen, combat math) headlessly — never test rendering |
| RNG | `std::mt19937` seeded per-run, seed printable for reproducible dungeons | Debuggability |

**Note on truecolor:** Ghostty supports 24-bit color. Detect via `COLORTERM=truecolor` env var; if present, use extended color pairs; otherwise fall back to 256-color palette. Abstract this behind the engine's `Color` type so game code never touches ncurses color pairs directly.

## Repository Structure

```
terminal-arcade/
├── CMakeLists.txt
├── README.md
├── engine/               # Shared static library: "tae" (terminal arcade engine)
│   ├── include/tae/
│   │   ├── app.hpp       # Game loop, fixed timestep, scene stack
│   │   ├── renderer.hpp  # Cell buffer, glyphs, color, diff-based draw
│   │   ├── input.hpp     # Non-blocking key polling, key repeat handling
│   │   ├── color.hpp     # RGB struct + palette, truecolor/256 fallback
│   │   ├── rng.hpp
│   │   └── ui.hpp        # Text, boxes, menus, HUD bars
│   └── src/
├── games/
│   ├── gauntlike/        # Game 1 (the main event)
│   ├── snake/            # Game 2
│   ├── breakout/         # Game 3
│   └── starfall/         # Game 4: vertical shmup (stretch)
└── tests/
```

Each game builds to its own binary. A top-level `arcade` binary presents a menu to launch any of them.

---

## Phase 0 — Scaffolding (Milestone: "hello dungeon")

1. Init repo, CMakeLists with `engine` static lib + `gauntlike` executable, `-Wall -Wextra`, C++20.
2. Link `ncursesw`. Verify wide-char + color init works.
3. Minimal `App` class: init curses (raw mode, no echo, hidden cursor, `nodelay`), run loop, restore terminal on exit **and on crash** (install atexit/signal handlers — a corrupted terminal is the #1 curses annoyance).
4. Render "HELLO DUNGEON" centered, quit on `q`.

**Acceptance:** `cmake -B build && cmake --build build && ./build/gauntlike` shows the screen, quits cleanly, terminal state restored.

## Phase 1 — Engine Core

1. **Fixed timestep loop:** logic at 30 Hz, render after each tick. Sleep the remainder; never busy-wait.
2. **Renderer:** off-screen `Cell` buffer (glyph + fg/bg color). Each frame, diff against previous buffer and only redraw changed cells (ncurses batches, but diffing keeps it snappy on large terminals). Handle terminal resize (`KEY_RESIZE`) gracefully.
3. **Input:** poll all pending keys per tick into a `std::vector<Key>`; map arrows + WASD + vi keys (hjkl). Support "held direction" feel by tracking last direction.
4. **Color:** `Color{r,g,b}` with a curated 16-color retro palette (think Genesis: deep purples, teals, hot magenta) + truecolor/256 fallback logic.
5. **Scene stack:** push/pop scenes (TitleScene, PlayScene, PauseScene, GameOverScene).
6. **UI helpers:** draw box, centered text, HP/score bar.

**Acceptance:** demo scene with a `@` moving around at 30 FPS, no flicker, resize doesn't crash, unit tests pass for input mapping and color fallback.

## Phase 2 — Gauntlike (the Gauntlet-style crawler)

### Design summary
- Top-down dungeon, one screen per floor (fits terminal, min 80×24; scale map to terminal size).
- Pick a class at start: **Warrior** (melee, high HP), **Wizard** (ranged, low HP), **Elf** (fast, medium), **Valkyrie** (balanced).
- Health drains slowly over time (classic Gauntlet pressure). Food restores it. "Warrior needs food, badly."
- Enemies stream from **spawners** (generators) until you destroy the spawner.
- Find the key, open the exit, descend. Difficulty scales per floor. Score persists to a local high-score file.

### Build order
1. **Map + tiles:** `Tile{Wall, Floor, Door, Exit, Spawner…}`, glyph/color per tile (`█` walls with per-floor palette, `·` floor). Hand-author 2 test maps first.
2. **Procedural generation:** rooms-and-corridors (BSP or simple room placement + corridor carving). Guarantee connectivity (flood fill check). Place: player start, exit, key, 2–4 spawners, food, treasure, potions. Unit-test connectivity + item placement.
3. **Entities:** simple struct-of-entities or small ECS-lite (id + component arrays — keep it boring). Player, monsters (Grunt `g`, Ghost `G`, Demon `d`, Lobber `l`), projectiles, pickups.
4. **Movement + collision:** grid-based, 8-directional for player, tile collision.
5. **Combat:** melee bump-attack + ranged projectiles (per class). Damage numbers flash. Enemies die with a brief `*` burst.
6. **Enemy AI:** greedy chase with simple pathfinding (BFS distance field recomputed every few ticks — cheap and very Gauntlet). Ghosts ignore walls partially? Keep v1 simple: all chase.
7. **Spawners:** emit an enemy every N ticks if under cap; destroyable, worth big points.
8. **Health drain + food + potions:** potion = screen-clearing smart bomb (classic).
9. **HUD:** top bar — class, HP bar, score, keys, floor, potion count.
10. **Floors + difficulty curve:** exit → regenerate map with higher spawn rates/enemy HP.
11. **Title / pause / game over scenes,** high-score table saved to `~/.local/share/terminal-arcade/scores`.

**Acceptance:** full loop — title → class select → descend ≥3 floors → die → game over → high score recorded. Playable and *fun* at 30 FPS in Ghostty.

### Juice pass (retro feel)
- Per-floor color themes (dungeon → caves → hellish reds).
- Screen shake (1-cell offset for 2 ticks) on player damage.
- Announcer-style messages in a bottom message line ("Elf shot the food!").
- Blinking exit tile, animated spawner glyphs.

## Phase 3 — Second Game: Snake (engine validation)

Small on purpose — proves the engine is reusable. Classic snake, wrap-around option, speed increases, same HUD/scene/high-score infrastructure. Should take a fraction of Gauntlike's effort; if it doesn't, refactor the engine.

## Phase 4 — Breakout

Sub-cell smoothness trick: use half-block glyphs (`▀ ▄`) to double vertical resolution so ball movement feels smooth. Paddle via arrows/a-d, brick colors by row, power-ups (multi-ball, wide paddle), levels.

## Phase 5 (stretch) — Starfall (vertical shmup)

Genesis-style shooter: player ship `▲`, enemy waves in patterns, bullet streams, power-ups, boss every 5 waves. Reuses projectile + spawner code from Gauntlike.

## Phase 6 — Arcade launcher + packaging

- `arcade` binary: menu listing installed games, launches in-process via scene stack.
- `cmake --install` support; short README with Ghostty-specific notes (truecolor, font recommendations like a Nerd Font or classic bitmap-style font).

---

## Working Instructions for Claude Code

1. Work phase by phase, milestone by milestone. **Do not start Phase N+1 until Phase N's acceptance criteria pass.**
2. Commit at each numbered step with a descriptive message.
3. After each milestone, build and run a smoke test; also run `ctest`.
4. Keep game logic free of ncurses calls — logic must be headless-testable. Only `renderer.cpp`/`input.cpp` touch curses.
5. Keep it boring: no heavy ECS frameworks, no threads (single-threaded loop), no dependencies beyond ncurses + Catch2.
6. Guard every curses init with proper teardown (RAII wrapper `CursesSession`), including on exceptions and SIGINT/SIGTERM.
7. If terminal is smaller than 80×24, show a friendly "resize your terminal" screen instead of crashing.
8. Performance target: <2% CPU while idle-rendering an unchanged frame (diff renderer working correctly).

## Definition of Done (v1.0)

- Gauntlike + Snake + Breakout fully playable, launcher works.
- Clean build with no warnings on macOS (clang) and Linux (gcc).
- Terminal never left in a broken state, under any exit path.
- High scores persist. README documents controls and build steps.
