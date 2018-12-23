cmake_minimum_required(VERSION 3.5)


project(inja LANGUAGES CXX VERSION 2.0.0)


option(BUILD_TESTS "Build the inja unit tests" ON)
option(BUILD_BENCHMARK "Build the inja benchmark" ON)
option(COVERALLS "Generate coveralls data" OFF)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


add_library(inja INTERFACE)
target_include_directories(inja INTERFACE include)


execute_process(COMMAND python3 amalgamate/amalgamate.py -c amalgamate/config.json -s include WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})


if (COVERALLS)
  include(Coveralls)
  coveralls_turn_on_coverage()

  file(GLOB_RECURSE COVERAGE_SRCS include/inja/*.hpp)

  # set(COVERAGE_SRCS test/unit.cpp test/unit-renderer.cpp include/inja)

  coveralls_setup("${COVERAGE_SRCS}" OFF) # If we should upload.
endif()


if(BUILD_TESTS)
  enable_testing()

  add_executable(inja_test
    test/unit.cpp
    test/unit-files.cpp
    test/unit-renderer.cpp
  )
  target_link_libraries(inja_test PRIVATE inja)

  add_test(inja_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inja_test)
endif()


if(BUILD_BENCHMARK)
  add_executable(inja_benchmark test/benchmark.cpp)
  target_link_libraries(inja_benchmark PRIVATE inja)
endif()
