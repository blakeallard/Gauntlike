cmake_minimum_required(VERSION 3.20)
project(terminal_arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_subdirectory(engine)
add_subdirectory(games/gauntlike)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
