#include "hayai/hayai.hpp"
#include <inja/inja.hpp>


using json = nlohmann::json;


inja::Environment env;

json data = {{"name", "Peter"}};

std::string string_template {"Lorem {{ name }}! Omnis in aut nobis libero enim. Porro optio ratione molestiae necessitatibus numquam architecto soluta. Magnam minus unde quas {{ name }} aspernatur occaecati et voluptas cupiditate. Assumenda ut alias quam voluptate aut saepe ullam dignissimos. \n Sequi aut autem nihil voluptatem tenetur incidunt. Autem commodi animi rerum. {{ lower(name) }} Mollitia eligendi aut sed rerum veniam. Eum et fugit velit sint ratione voluptatem aliquam. Minima sint consectetur natus modi quis. Animi est nesciunt cupiditate nostrum iure. Voluptatem accusamus vel corporis. \n Debitis {{ name }} sunt est debitis distinctio ut. Provident corrupti nihil velit aut tempora corporis corrupti exercitationem. Praesentium cumque ex est itaque."};


BENCHMARK(InjaBenchmarker, render, 10, 100) {
  env.Render(string_template, data);
}

int main() {
  hayai::ConsoleOutputter consoleOutputter;

  hayai::Benchmarker::AddOutputter(consoleOutputter);
  hayai::Benchmarker::RunAllTests();
  return 0;
}
