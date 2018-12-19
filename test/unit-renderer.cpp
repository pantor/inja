#include <iostream>

#include "inja/inja.h"


int main() {
  inja::Environment env;
  inja::json data;
  data["name"] = "Peter";

  std::cout << env.Render("Hello {{ name }}!", data) << std::endl; // , "Hello Peter!"
}
