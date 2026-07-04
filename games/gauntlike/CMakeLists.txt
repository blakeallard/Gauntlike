# Headless game logic — no curses calls, unit-testable.
add_library(gauntlike_core STATIC
  src/map.cpp
)
target_include_directories(gauntlike_core PUBLIC src)
target_link_libraries(gauntlike_core PUBLIC tae)

add_executable(gauntlike
  main.cpp
)
target_link_libraries(gauntlike PRIVATE gauntlike_core)
