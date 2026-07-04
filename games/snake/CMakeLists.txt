add_library(snake_core STATIC
  src/snake.cpp
)
target_include_directories(snake_core PUBLIC src)
target_link_libraries(snake_core PUBLIC tae)

add_library(snake_scenes STATIC
  src/scenes.cpp
)
target_link_libraries(snake_scenes PUBLIC snake_core)

add_executable(snake
  main.cpp
)
target_link_libraries(snake PRIVATE snake_scenes)
