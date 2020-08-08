// Copyright (c) 2020 Pantor. All rights reserved.

#include "hayai/hayai.hpp"
#include <inja/inja.hpp>

using json = nlohmann::json;

inja::Environment env;

const std::string test_file_directory {"../test/data/benchmark/"};

json small_data = env.load_json(test_file_directory + "small_data.json");
json large_data = env.load_json(test_file_directory + "large_data.json");
std::string medium_template = env.load_file(test_file_directory + "medium_template.txt");
std::string large_template = env.load_file(test_file_directory + "large_template.txt");


BENCHMARK(SmallDataMediumTemplate, render, 5, 30) { env.render(medium_template, small_data); }
BENCHMARK(LargeDataMediumTemplate, render, 5, 15) { env.render(medium_template, large_data); }
BENCHMARK(LargeDataLargeTemplate, render, 5, 5) { env.render(large_template, large_data); }


int main() {
  hayai::ConsoleOutputter consoleOutputter;

  hayai::Benchmarker::AddOutputter(consoleOutputter);
  hayai::Benchmarker::RunAllTests();
  return 0;
}
