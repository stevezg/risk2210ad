cmake_minimum_required(VERSION 3.16)
project(risk2210_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(risk2210_engine
  engine/MapGraph.cpp
  engine/Cards.cpp
  engine/GameDirector.cpp
)
target_include_directories(risk2210_engine PUBLIC engine)
target_compile_options(risk2210_engine PRIVATE -Wall -Wextra)

enable_testing()
add_executable(self_play_test tests/self_play_test.cpp)
target_link_libraries(self_play_test PRIVATE risk2210_engine)
add_test(NAME self_play COMMAND self_play_test)
