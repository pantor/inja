cmake_minimum_required(VERSION 3.5)

# See https://cmake.org/cmake/help/latest/policy/CMP0077.html
# This allows for setting option variables externally, when this project
# is included in another CMake project.
cmake_policy(SET CMP0077 NEW)

project(inja LANGUAGES CXX VERSION 3.4.0)


option(INJA_USE_EMBEDDED_JSON "Use the shipped json header if not available on the system" ON)
option(INJA_INSTALL "Generate install targets for inja" ON)
option(INJA_EXPORT "Export the current build tree to the package registry" ON)
option(BUILD_TESTING "Build unit tests" ON)
option(INJA_BUILD_TESTS "Build unit tests when BUILD_TESTING is enabled." ON)
option(BUILD_BENCHMARK "Build benchmark" ON)
option(COVERALLS "Generate coveralls data" OFF)


set(INJA_INSTALL_INCLUDE_DIR "include")
set(INJA_PACKAGE_USE_EMBEDDED_JSON OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)


# For using the correct __cplusplus macro
if(MSVC)
  add_compile_options(/utf-8 /Zc:__cplusplus)
endif()


add_library(inja INTERFACE)
add_library(pantor::inja ALIAS inja)


target_include_directories(inja INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${INJA_INSTALL_INCLUDE_DIR}>
)
target_compile_features(inja INTERFACE cxx_std_17)


if(INJA_USE_EMBEDDED_JSON)
  find_package(nlohmann_json QUIET)
  if(NOT nlohmann_json_FOUND)
    set(INJA_PACKAGE_USE_EMBEDDED_JSON ON)
    add_library(nlohmann_json INTERFACE)
    add_library(pantor::nlohmann_json ALIAS nlohmann_json)

    target_include_directories(nlohmann_json INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/include>
      $<INSTALL_INTERFACE:${INJA_INSTALL_INCLUDE_DIR}/inja/json/include>
    )

    target_compile_features(nlohmann_json INTERFACE cxx_std_17)

    install(TARGETS nlohmann_json EXPORT injaTargets)

    set(INJA_SELECTED_JSON_LIBRARY "pantor::nlohmann_json")
  else()
    set(INJA_SELECTED_JSON_LIBRARY "nlohmann_json::nlohmann_json")
  endif()
else()
  # If target already exists, e.g. by git submodules
  if(TARGET nlohmann_json)
    set(INJA_SELECTED_JSON_LIBRARY "nlohmann_json::nlohmann_json")

    install(TARGETS nlohmann_json EXPORT injaTargets)
  else()
    find_package(nlohmann_json REQUIRED)
    set(INJA_SELECTED_JSON_LIBRARY "nlohmann_json::nlohmann_json")
  endif()
endif()


target_link_libraries(inja INTERFACE ${INJA_SELECTED_JSON_LIBRARY})


execute_process(COMMAND scripts/update_single_include.sh WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})


if(BUILD_TESTING AND INJA_BUILD_TESTS)
  enable_testing()

  add_executable(inja_test test/test.cpp)
  target_link_libraries(inja_test PRIVATE inja)
  add_test(inja_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inja_test)

  if(MSVC)
    target_compile_options(inja_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(inja_test PRIVATE -Wall -Wextra -Werror)
  endif()


  add_library(single_inja INTERFACE)
  target_compile_features(single_inja INTERFACE cxx_std_17)
  target_include_directories(single_inja INTERFACE single_include include third_party/include)

  add_executable(single_inja_test test/test.cpp)
  target_link_libraries(single_inja_test PRIVATE single_inja)

  add_test(single_inja_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/single_inja_test)
endif()


if(BUILD_BENCHMARK)
  add_executable(inja_benchmark test/benchmark.cpp)
  target_link_libraries(inja_benchmark PRIVATE inja)
endif()


if(COVERALLS)
  include(Coveralls)
  coveralls_turn_on_coverage()

  file(GLOB_RECURSE COVERAGE_SRCS include/inja/*.hpp)

  # set(COVERAGE_SRCS test/unit.cpp test/unit-renderer.cpp include/inja)

  coveralls_setup("${COVERAGE_SRCS}" OFF) # If we should upload.
endif()


include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/injaConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)


# build tree package config
configure_file(
  cmake/config/injaBuildConfig.cmake.in
  injaConfig.cmake
  @ONLY
)


install(TARGETS inja EXPORT injaTargets)


export(
  EXPORT injaTargets
  NAMESPACE pantor::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/injaTargets.cmake"
)

# build tree package config
configure_file(
  cmake/config/injaBuildConfig.cmake.in
  injaConfig.cmake
  @ONLY
)


if(INJA_INSTALL)
  set(INJA_CONFIG_PATH "lib/cmake/inja")

  # install tree package config
  configure_package_config_file(
    cmake/config/injaConfig.cmake.in
    ${INJA_CONFIG_PATH}/injaConfig.cmake
    INSTALL_DESTINATION ${INJA_CONFIG_PATH}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

  install(
    DIRECTORY include/inja
    DESTINATION ${INJA_INSTALL_INCLUDE_DIR}
    FILES_MATCHING PATTERN "*.hpp"
  )
  if(INJA_USE_EMBEDDED_JSON)
    install(
      DIRECTORY third_party/include/nlohmann
      DESTINATION ${INJA_INSTALL_INCLUDE_DIR}
      FILES_MATCHING PATTERN "*.hpp"
    )
  endif()
  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/${INJA_CONFIG_PATH}/injaConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/injaConfigVersion.cmake"
    DESTINATION ${INJA_CONFIG_PATH}
  )

  install(
    EXPORT injaTargets FILE injaTargets.cmake
    NAMESPACE pantor::
    DESTINATION ${INJA_CONFIG_PATH}
  )
endif()


if(INJA_EXPORT)
  export(PACKAGE inja)
endif()
