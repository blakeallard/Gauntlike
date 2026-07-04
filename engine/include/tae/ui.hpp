#pragma once

#include <string_view>

#include "renderer.hpp"

namespace tae::ui {

// Box-drawing frame (single line), interior untouched.
void box(Renderer& r, int x, int y, int w, int h, Color fg, Color bg);

// Frame + filled interior.
void panel(Renderer& r, int x, int y, int w, int h, Color fg, Color bg);

// Text centered horizontally at row y.
void text_centered(Renderer& r, int y, std::string_view s, Color fg, Color bg);

// Horizontal meter (HP/score bars): `frac` in [0,1] filled with █.
void hbar(Renderer& r, int x, int y, int w, double frac, Color full, Color empty, Color bg);

}  // namespace tae::ui
