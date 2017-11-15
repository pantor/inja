cmake_minimum_required(VERSION 3.1)

##
## HUNTER
##
option(HUNTER_ENABLED "Use hunter to manage dependencies" OFF)
if(HUNTER_ENABLED)
    include("../cmake/HunterGate.cmake")
    HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.19.156.tar.gz"
        SHA1 "8d5e4635b137365e0d1ade4d60accf4e2bb41f0d"
    )
endif()

##
## PROJECT
## name and version
##
project(inja-testing LANGUAGES CXX VERSION ${INJA_VERSION})

##
## TARGETS
##
set(UNITTEST_TARGET_NAME "inja_unit")
file(GLOB TEST_SOURCES "src/*.cpp")
add_executable(${UNITTEST_TARGET_NAME} ${TEST_SOURCES})

##
## INCLUDES
##
if(HUNTER_ENABLED) # Use Hunter to manage dependencies
    # Add Catch framework
    hunter_add_package(Catch)
    find_package(Catch CONFIG REQUIRED)
    # Add JSON package
    hunter_add_package(nlohmann_json)
    find_package(nlohmann_json CONFIG REQUIRED)
    # Add dependencies to target
    target_link_libraries(${UNITTEST_TARGET_NAME} Catch::Catch)
    target_link_libraries(${UNITTEST_TARGET_NAME} nlohmann_json)
else() # Manage dependencies manually
    # Prepare "Catch" library for other executables
    set(CATCH_INCLUDE_DIR "thirdparty/catch")
    add_library(Catch INTERFACE)
    target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})
    # Add dependencies to target
    target_link_libraries(${UNITTEST_TARGET_NAME} Catch)
    target_include_directories(${UNITTEST_TARGET_NAME} PRIVATE "../src" "thirdparty/json")
endif()

##
## Add tests to make
##
add_test(NAME "${UNITTEST_TARGET_NAME}_default"
    COMMAND ${UNITTEST_TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
