cmake_minimum_required(VERSION 3.28)

add_library(inja_module)

target_sources(inja_module
    PUBLIC
        FILE_SET CXX_MODULES FILES
            inja.cppm
)

target_compile_features(inja_module PUBLIC cxx_std_20)

target_include_directories(inja_module PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_library(inja::module ALIAS inja_module)

# Installation
install(TARGETS inja_module
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/inja/modules
)
