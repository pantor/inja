cmake_minimum_required(VERSION 3.1)

##
## PROJECT
## name and version
##
project(inja)

##
## OPTIONS
##
option(BUILD_UNIT_TESTS "Build the unit tests" ON)

##
## CONFIGURATION
##
set(CMAKE_CXX_STANDARD 11)
set(INJA_SOURCE_DIR src/)
set(INJA_HEADER_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dist)

##
## TESTS
## create and configure the unit test target
##
if(BUILD_UNIT_TESTS)
    enable_testing()
    # include_directories(${NLOHMANN_JSON_SOURCE_DIR})
    add_subdirectory(test)
endif()

##
## INSTALL
## install header files, generate and install cmake config files for find_package()
##
install(
    DIRECTORY ${INJA_SOURCE_DIR}
    DESTINATION ${INJA_HEADER_INSTALL_DIR}
)
