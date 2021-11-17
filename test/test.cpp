// Copyright (c) 2020 Pantor. All rights reserved.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"

#define JSON_USE_IMPLICIT_CONVERSIONS 0
#define JSON_NO_IO 1
#include "inja/inja.hpp"

const std::string test_file_directory {"../test/data/"};

#include "test-files.cpp"
#include "test-functions.cpp"
#include "test-renderer.cpp"
#include "test-units.cpp"
