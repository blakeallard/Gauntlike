#pragma once

#include <memory>

#include "game.hpp"
#include "tae/app.hpp"

namespace gl {

// Title + class select. Entry point scene.
std::unique_ptr<tae::Scene> make_title_scene();

// Gameplay, driving a Game sized to the given renderer.
std::unique_ptr<tae::Scene> make_play_scene(PlayerClass cls, int term_w, int term_h);

}  // namespace gl
