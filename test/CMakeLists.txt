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
## TARGETS
##
add_executable(inja_test
  src/unit-files.cpp
  src/unit-renderer.cpp
  src/unit-string-helper.cpp
  src/unit.cpp
)


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
  target_link_libraries(inja_test Catch::Catch nlohmann_json inja)
else() # Manage dependencies manually
  # Prepare "Catch" library for other executables
  add_library(Catch INTERFACE)
  target_include_directories(Catch INTERFACE "thirdparty")

  # Prepare "JSON" library for other executables
  add_library(JSON INTERFACE)
  target_include_directories(JSON INTERFACE "thirdparty")

  # Add dependencies to target
  target_link_libraries(inja_test Catch JSON inja)
endif()


##
## Copy test files to build directory
##
add_custom_command(
  TARGET inja_test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/data
  ${CMAKE_CURRENT_BINARY_DIR}/data
)


##
## Add tests to make
##
add_test(NAME inja_test
  COMMAND inja_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
