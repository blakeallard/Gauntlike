add_library(snake_core STATIC
  src/snake.cpp
)
target_include_directories(snake_core PUBLIC src)
target_link_libraries(snake_core PUBLIC tae)

add_executable(snake
  main.cpp
  src/scenes.cpp
)
target_link_libraries(snake PRIVATE snake_core)
