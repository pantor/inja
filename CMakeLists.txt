cmake_minimum_required(VERSION 3.1)


project(inja LANGUAGES CXX VERSION 2.0.0)


set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")


add_library(inja
  src/inja.cpp
  src/parser.cpp
  src/renderer.cpp
)
target_include_directories(inja PUBLIC include)


add_executable(inja_unit_renderer test/unit-renderer.cpp)
target_link_libraries(inja_unit_renderer inja)


add_executable(inja_benchmark test/benchmark.cpp)
target_link_libraries(inja_benchmark inja)
