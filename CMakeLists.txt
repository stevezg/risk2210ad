cmake_minimum_required(VERSION 3.16)
project(risk2210 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(risk2210
  src/Map.cpp
  src/Cards.cpp
  src/Game.cpp
  src/Agents.cpp
)
target_include_directories(risk2210 PUBLIC include)
target_compile_options(risk2210 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(risk2210_cli src/main.cpp)
target_link_libraries(risk2210_cli PRIVATE risk2210)

enable_testing()
add_executable(risk2210_tests tests/tests.cpp)
target_link_libraries(risk2210_tests PRIVATE risk2210)
add_test(NAME engine_tests COMMAND risk2210_tests)

# Optional raylib GUI (brew install raylib / apt install libraylib-dev)
find_package(raylib QUIET)
if(raylib_FOUND)
  add_executable(risk2210_gui src/gui/gui_main.cpp)
  target_include_directories(risk2210_gui PRIVATE src/gui)
  target_link_libraries(risk2210_gui PRIVATE risk2210 raylib)
  find_package(Threads REQUIRED)
  target_link_libraries(risk2210_gui PRIVATE Threads::Threads)
  if(APPLE)
    target_link_libraries(risk2210_gui PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
  endif()
  message(STATUS "raylib found: building risk2210_gui")
else()
  message(STATUS "raylib not found: skipping risk2210_gui")
endif()
