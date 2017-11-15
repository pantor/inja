##
## Prepare "Catch" library for other executables
##
set(CATCH_INCLUDE_DIR "thirdparty/catch")
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

set(UNITTEST_TARGET_NAME "inja_unit")
file(GLOB TEST_SOURCES "src/*.cpp")
add_executable(${UNITTEST_TARGET_NAME} ${TEST_SOURCES})

target_link_libraries(${UNITTEST_TARGET_NAME} Catch)
target_include_directories(${UNITTEST_TARGET_NAME} PRIVATE "../src" "thirdparty/json")

##
## Add tests to make
##
add_test(NAME "${UNITTEST_TARGET_NAME}_default"
    COMMAND ${UNITTEST_TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
