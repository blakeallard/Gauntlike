add_library(breakout_core STATIC
  src/breakout.cpp
)
target_include_directories(breakout_core PUBLIC src)
target_link_libraries(breakout_core PUBLIC tae)

add_executable(breakout
  main.cpp
  src/scenes.cpp
)
target_link_libraries(breakout PRIVATE breakout_core)
