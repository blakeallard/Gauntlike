#pragma once

#include <memory>

#include "tae/app.hpp"

namespace bo {

std::unique_ptr<tae::Scene> make_title_scene();

}  // namespace bo
