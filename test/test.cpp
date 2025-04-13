// Copyright (c) 2020 Pantor. All rights reserved.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#define JSON_NO_IO 1

#include <filesystem>

#include "test-files.cpp"
#include "test-functions.cpp"
#include "test-renderer.cpp"
#include "test-units.cpp"

#define xstr(s) str(s)
#define str(s) #s

const std::filesystem::path test_file_directory { std::filesystem::path(xstr(__TEST_DIR__)) / "data" };
