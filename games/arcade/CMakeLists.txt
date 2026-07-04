add_executable(arcade
  main.cpp
)
target_link_libraries(arcade PRIVATE gauntlike_scenes snake_scenes breakout_scenes)
