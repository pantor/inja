cmake_minimum_required(VERSION 3.5)

# See https://cmake.org/cmake/help/latest/policy/CMP0077.html
# This allows for setting option variables externally, when this project
# is included in another CMake project.
cmake_policy(SET CMP0077 NEW)

project(inja
        DESCRIPTION "Template engine for modern C++"
        LANGUAGES CXX
        VERSION 3.5.0)


option(BUILD_TESTING "Build unit tests" ON)
option(COVERALLS "Generate coveralls data" OFF)
option(INJA_BUILD_TESTS "Build unit tests when BUILD_TESTING is enabled." ON)
option(INJA_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
option(INJA_EXPORT "Export the current build tree to the package registry" ON)
option(INJA_INSTALL "Generate install targets for inja" ON)
option(INJA_INSTALL_SINGLE_HEADER "Install the single header instead" OFF)
option(INJA_USE_EMBEDDED_JSON "Use the shipped json header if not available on the system" ON)

include(GNUInstallDirs)

set(INJA_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR})
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
      $<INSTALL_INTERFACE:${INJA_INSTALL_INCLUDE_DIR}/nlohmann>
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

  add_definitions(-D__TEST_DIR__=${CMAKE_CURRENT_SOURCE_DIR}/test)

  add_executable(inja_test test/test.cpp)
  target_link_libraries(inja_test PRIVATE inja)
  target_include_directories(inja_test PRIVATE include third_party/include)
  add_test(inja_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inja_test)

  if(MSVC)
    target_compile_options(inja_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(inja_test PRIVATE -Wall -Wextra -Werror)
  endif()

  if(INJA_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy" REQUIRED)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "-config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")
    set_target_properties(inja_test PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
  endif()


  add_library(single_inja INTERFACE)
  target_compile_features(single_inja INTERFACE cxx_std_17)
  target_include_directories(single_inja INTERFACE single_include)

  add_executable(single_inja_test test/test.cpp)
  target_link_libraries(single_inja_test PRIVATE single_inja)
  target_include_directories(single_inja_test PRIVATE include third_party/include)

  add_test(single_inja_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/single_inja_test)


  add_executable(inja_benchmark test/benchmark.cpp test/test-common.hpp)
  target_link_libraries(inja_benchmark PRIVATE inja)
  target_include_directories(inja_benchmark PRIVATE third_party/include)
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
  set(INJA_CONFIG_PATH "${CMAKE_INSTALL_DATADIR}/cmake/inja")

  # install tree package config
  configure_package_config_file(
    cmake/config/injaConfig.cmake.in
    ${INJA_CONFIG_PATH}/injaConfig.cmake
    INSTALL_DESTINATION ${INJA_CONFIG_PATH}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

  configure_file(${PROJECT_SOURCE_DIR}/cmake/config/inja.pc.in ${CMAKE_BINARY_DIR}/inja.pc @ONLY)
  install(FILES ${CMAKE_BINARY_DIR}/inja.pc
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig
  )

  if(INJA_INSTALL_SINGLE_HEADER)
    install(
      DIRECTORY single_include/inja
      DESTINATION ${INJA_INSTALL_INCLUDE_DIR}
      FILES_MATCHING PATTERN "*.hpp"
    )
  else()
    install(
      DIRECTORY include/inja
      DESTINATION ${INJA_INSTALL_INCLUDE_DIR}
      FILES_MATCHING PATTERN "*.hpp"
    )
  endif()

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
