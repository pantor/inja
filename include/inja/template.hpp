#ifndef PANTOR_INJA_TEMPLATE_HPP
#define PANTOR_INJA_TEMPLATE_HPP

#include <string>
#include <vector>

#include "bytecode.hpp"


namespace inja {

struct Template {
  std::vector<Bytecode> bytecodes;
  std::string content;
};

using TemplateStorage = std::map<std::string, Template>;

}

#endif // PANTOR_INJA_TEMPLATE_HPP
