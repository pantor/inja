cmake_minimum_required(VERSION 3.10)


project(inja LANGUAGES CXX VERSION 2.0.0)


option(BUILD_TESTS "Build the inja unit tests" ON)
option(BUILD_BENCHMARK "Build the inja benchmark" ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


add_library(inja INTERFACE)
target_include_directories(inja INTERFACE include)


# execute_process(COMMAND python3 amalgamate/amalgamate.py -c amalgamate/config.json -s include WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})


if(BUILD_TESTS)
  add_executable(inja_test test/unit.cpp test/unit-renderer.cpp)
  target_link_libraries(inja_test PRIVATE inja)
endif()


if(BUILD_BENCHMARK)
  add_executable(inja_benchmark test/benchmark.cpp)
  target_link_libraries(inja_benchmark PRIVATE inja)
endif()
