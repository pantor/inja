#include <iostream>

#include <inja/inja.hpp>


int main() {
  inja::Environment env;
  inja::json data;
  data["name"] = "Peter";

  std::cout << env.Render("Hello {{ lower(name) }}!", data) << std::endl; // , "Hello Peter!"
}
