// Copyright (c) 2020 Pantor. All rights reserved.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include "inja/inja.hpp"

using json = nlohmann::json;

const std::string test_file_directory {"../test/data/"};

#include "test-files.cpp"
#include "test-functions.cpp"
#include "test-renderer.cpp"
#include "test-units.cpp"
