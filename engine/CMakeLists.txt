set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)

add_library(tae STATIC
  src/curses_session.cpp
  src/app.cpp
  src/renderer.cpp
  src/input.cpp
  src/input_map.cpp
  src/color.cpp
)

target_include_directories(tae PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tae PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(tae PUBLIC ${CURSES_LIBRARIES})
target_compile_definitions(tae PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
